#pragma once

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <mutex>
#include <queue>
#include <thread>
#include <atomic>
#include <condition_variable>
#include <chrono>

namespace moneybot {

// Base event class
class Event {
public:
    virtual ~Event() = default;
    virtual std::string getType() const = 0;
    virtual std::string toString() const = 0;
    
    std::chrono::steady_clock::time_point timestamp = std::chrono::steady_clock::now();
    std::string source;
};

// Market data event
class MarketDataEvent : public Event {
public:
    std::string symbol;
    double price;
    double volume;
    
    std::string getType() const override { return "MarketData"; }
    std::string toString() const override {
        return "MarketDataEvent{symbol=" + symbol + ", price=" + std::to_string(price) + "}";
    }
};

// Order event
class OrderEvent : public Event {
public:
    enum class OrderEventType { PLACED, FILLED, CANCELLED, REJECTED };
    
    std::string order_id;
    std::string symbol;
    OrderEventType event_type;
    double quantity;
    double price;
    
    std::string getType() const override { return "Order"; }
    std::string toString() const override {
        return "OrderEvent{id=" + order_id + ", symbol=" + symbol + ", type=" + std::to_string(static_cast<int>(event_type)) + "}";
    }
};

// Connection event
class ConnectionEvent : public Event {
public:
    enum class ConnectionEventType { CONNECTED, DISCONNECTED, ERROR };
    
    std::string exchange;
    ConnectionEventType event_type;
    std::string message;
    
    std::string getType() const override { return "Connection"; }
    std::string toString() const override {
        return "ConnectionEvent{exchange=" + exchange + ", type=" + std::to_string(static_cast<int>(event_type)) + "}";
    }
};

// Error event
class ErrorEvent : public Event {
public:
    enum class ErrorLevel { WARNING, ERROR, CRITICAL };
    
    ErrorLevel level;
    std::string component;
    std::string message;
    std::string details;
    
    std::string getType() const override { return "Error"; }
    std::string toString() const override {
        return "ErrorEvent{component=" + component + ", message=" + message + "}";
    }
};

// Event listener interface
using EventListener = std::function<void(std::shared_ptr<Event>)>;

// Event manager class
class EventManager {
public:
    static EventManager& getInstance() {
        static EventManager instance;
        return instance;
    }

    // Subscribe to events
    void subscribe(const std::string& event_type, const std::string& listener_name, EventListener listener);
    void subscribe(const std::string& listener_name, EventListener listener); // Subscribe to all events
    
    // Unsubscribe
    void unsubscribe(const std::string& event_type, const std::string& listener_name);
    void unsubscribe(const std::string& listener_name); // Unsubscribe from all events
    
    // Publish events
    void publish(std::shared_ptr<Event> event);
    
    // Synchronous event handling (for critical events)
    void publishSync(std::shared_ptr<Event> event);
    
    // Event queue management
    void start();
    void stop();
    bool isRunning() const;
    
    // Statistics
    size_t getQueueSize() const;
    size_t getTotalEventsProcessed() const;
    size_t getSubscriberCount() const;
    size_t getSubscriberCount(const std::string& event_type) const;
    
    // Event filtering
    void setEventFilter(std::function<bool(std::shared_ptr<Event>)> filter);
    void clearEventFilter();

private:
    EventManager() = default;
    ~EventManager();
    EventManager(const EventManager&) = delete;
    EventManager& operator=(const EventManager&) = delete;

    void processEvents();
    void notifyListeners(std::shared_ptr<Event> event, bool sync = false);

    mutable std::mutex mutex_;
    std::unordered_map<std::string, std::unordered_map<std::string, EventListener>> typed_listeners_;
    std::unordered_map<std::string, EventListener> global_listeners_;
    
    std::queue<std::shared_ptr<Event>> event_queue_;
    std::condition_variable cv_;
    std::thread worker_thread_;
    std::atomic<bool> running_{false};
    std::atomic<bool> shutdown_requested_{false};
    
    std::atomic<size_t> total_events_processed_{0};
    std::function<bool(std::shared_ptr<Event>)> event_filter_;
};

// Convenience macros for publishing events
#define PUBLISH_EVENT(event) moneybot::EventManager::getInstance().publish(event)
#define PUBLISH_EVENT_SYNC(event) moneybot::EventManager::getInstance().publishSync(event)

// Helper functions for creating events
namespace events {
    std::shared_ptr<MarketDataEvent> createMarketDataEvent(const std::string& symbol, double price, double volume);
    std::shared_ptr<OrderEvent> createOrderEvent(const std::string& order_id, const std::string& symbol, 
                                                 OrderEvent::OrderEventType type, double quantity, double price);
    std::shared_ptr<ConnectionEvent> createConnectionEvent(const std::string& exchange, 
                                                          ConnectionEvent::ConnectionEventType type, 
                                                          const std::string& message = "");
    std::shared_ptr<ErrorEvent> createErrorEvent(ErrorEvent::ErrorLevel level, const std::string& component, 
                                                 const std::string& message, const std::string& details = "");
}

} // namespace moneybot
