#include "event_manager.h"
#include "modern_logger.h"
#include <algorithm>

namespace moneybot {

EventManager::~EventManager() {
    stop();
}

void EventManager::subscribe(const std::string& event_type, const std::string& listener_name, EventListener listener) {
    std::lock_guard<std::mutex> lock(mutex_);
    typed_listeners_[event_type][listener_name] = listener;
    LOG_DEBUG("Subscribed listener to event type");
}

void EventManager::subscribe(const std::string& listener_name, EventListener listener) {
    std::lock_guard<std::mutex> lock(mutex_);
    global_listeners_[listener_name] = listener;
    LOG_DEBUG("Subscribed global listener");
}

void EventManager::unsubscribe(const std::string& event_type, const std::string& listener_name) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = typed_listeners_.find(event_type);
    if (it != typed_listeners_.end()) {
        it->second.erase(listener_name);
        if (it->second.empty()) {
            typed_listeners_.erase(it);
        }
        LOG_DEBUG("Unsubscribed listener from event type");
    }
}

void EventManager::unsubscribe(const std::string& listener_name) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Remove from global listeners
    global_listeners_.erase(listener_name);
    
    // Remove from typed listeners
    for (auto& type_pair : typed_listeners_) {
        type_pair.second.erase(listener_name);
    }
    
    // Clean up empty event types
    for (auto it = typed_listeners_.begin(); it != typed_listeners_.end();) {
        if (it->second.empty()) {
            it = typed_listeners_.erase(it);
        } else {
            ++it;
        }
    }
    
    LOG_DEBUG("Unsubscribed listener from all events");
}

void EventManager::publish(std::shared_ptr<Event> event) {
    if (!event) return;
    
    // Apply filter if set
    if (event_filter_ && !event_filter_(event)) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    event_queue_.push(event);
    cv_.notify_one();
}

void EventManager::publishSync(std::shared_ptr<Event> event) {
    if (!event) return;
    
    // Apply filter if set
    if (event_filter_ && !event_filter_(event)) {
        return;
    }
    
    notifyListeners(event, true);
}

void EventManager::start() {
    if (running_.exchange(true)) {
        return; // Already running
    }
    
    shutdown_requested_ = false;
    worker_thread_ = std::thread(&EventManager::processEvents, this);
    LOG_INFO("Event manager started");
}

void EventManager::stop() {
    if (!running_.exchange(false)) {
        return; // Already stopped
    }
    
    shutdown_requested_ = true;
    cv_.notify_all();
    
    if (worker_thread_.joinable()) {
        worker_thread_.join();
    }
    
    LOG_INFO("Event manager stopped");
}

bool EventManager::isRunning() const {
    return running_.load();
}

size_t EventManager::getQueueSize() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return event_queue_.size();
}

size_t EventManager::getTotalEventsProcessed() const {
    return total_events_processed_.load();
}

size_t EventManager::getSubscriberCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    size_t count = global_listeners_.size();
    for (const auto& type_pair : typed_listeners_) {
        count += type_pair.second.size();
    }
    return count;
}

size_t EventManager::getSubscriberCount(const std::string& event_type) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = typed_listeners_.find(event_type);
    return it != typed_listeners_.end() ? it->second.size() : 0;
}

void EventManager::setEventFilter(std::function<bool(std::shared_ptr<Event>)> filter) {
    event_filter_ = filter;
}

void EventManager::clearEventFilter() {
    event_filter_ = nullptr;
}

void EventManager::processEvents() {
    while (running_ && !shutdown_requested_) {
        std::unique_lock<std::mutex> lock(mutex_);
        
        cv_.wait(lock, [this] { 
            return !event_queue_.empty() || shutdown_requested_; 
        });
        
        while (!event_queue_.empty() && !shutdown_requested_) {
            auto event = event_queue_.front();
            event_queue_.pop();
            lock.unlock();
            
            notifyListeners(event);
            total_events_processed_++;
            
            lock.lock();
        }
    }
}

void EventManager::notifyListeners(std::shared_ptr<Event> event, bool sync) {
    if (!event) return;
    
    std::string event_type = event->getType();
    
    // Notify global listeners
    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (const auto& pair : global_listeners_) {
            try {
                pair.second(event);
            } catch (const std::exception& e) {
                LOG_ERROR("Error in global event listener");
            }
        }
        
        // Notify typed listeners
        auto it = typed_listeners_.find(event_type);
        if (it != typed_listeners_.end()) {
            for (const auto& pair : it->second) {
                try {
                    pair.second(event);
                } catch (const std::exception& e) {
                    LOG_ERROR("Error in event listener");
                }
            }
        }
    }
}

// Helper functions for creating events
namespace events {

std::shared_ptr<MarketDataEvent> createMarketDataEvent(const std::string& symbol, double price, double volume) {
    auto event = std::make_shared<MarketDataEvent>();
    event->symbol = symbol;
    event->price = price;
    event->volume = volume;
    event->source = "market_data";
    return event;
}

std::shared_ptr<OrderEvent> createOrderEvent(const std::string& order_id, const std::string& symbol, 
                                             OrderEvent::OrderEventType type, double quantity, double price) {
    auto event = std::make_shared<OrderEvent>();
    event->order_id = order_id;
    event->symbol = symbol;
    event->event_type = type;
    event->quantity = quantity;
    event->price = price;
    event->source = "order_manager";
    return event;
}

std::shared_ptr<ConnectionEvent> createConnectionEvent(const std::string& exchange, 
                                                      ConnectionEvent::ConnectionEventType type, 
                                                      const std::string& message) {
    auto event = std::make_shared<ConnectionEvent>();
    event->exchange = exchange;
    event->event_type = type;
    event->message = message;
    event->source = "exchange_manager";
    return event;
}

std::shared_ptr<ErrorEvent> createErrorEvent(ErrorEvent::ErrorLevel level, const std::string& component, 
                                             const std::string& message, const std::string& details) {
    auto event = std::make_shared<ErrorEvent>();
    event->level = level;
    event->component = component;
    event->message = message;
    event->details = details;
    event->source = component;
    return event;
}

} // namespace events

} // namespace moneybot
