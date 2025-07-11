#include "core/exchange_manager.h"
#include "binance_exchange.h"
#include "modern_logger.h"
#include "config_manager.h"
#include "event_manager.h"
#include <thread>
#include <chrono>
#include <algorithm>
#include <iostream>
#include <sstream>

namespace moneybot {

ExchangeManager::ExchangeManager(std::shared_ptr<ConfigManager> config_manager,
                               std::shared_ptr<ModernLogger> logger,
                               std::shared_ptr<EventManager> event_manager)
    : config_manager_(config_manager)
    , logger_(logger)
    , event_manager_(event_manager) {
    
    LOG_INFO("ExchangeManager initialized");
}

ExchangeManager::~ExchangeManager() {
    stopHealthMonitoring();
    disconnectAllExchanges();
    LOG_INFO("ExchangeManager destroyed");
}

bool ExchangeManager::addExchange(const std::string& name, std::unique_ptr<ExchangeInterface> exchange) {
    std::lock_guard<std::mutex> lock(exchanges_mutex_);
    
    if (exchanges_.find(name) != exchanges_.end()) {
        LOG_WARN("Exchange " + name + " already exists");
        return false;
    }
    
    exchanges_[name] = std::move(exchange);
    
    // Initialize health status
    {
        std::lock_guard<std::mutex> health_lock(health_mutex_);
        ExchangeHealth health;
        health.exchange_name = name;
        health.status = ExchangeHealthStatus::DISCONNECTED;
        health.last_check = std::chrono::system_clock::now();
        health.status_message = "Not connected";
        health_status_[name] = health;
    }
    
    // Initialize metrics
    {
        std::lock_guard<std::mutex> metrics_lock(metrics_mutex_);
        ExchangeMetrics metrics;
        metrics.exchange_name = name;
        metrics.session_start = std::chrono::system_clock::now();
        metrics_[name] = metrics;
    }
    
    LOG_INFO("Added exchange: " + name);
    return true;
}

bool ExchangeManager::removeExchange(const std::string& name) {
    std::lock_guard<std::mutex> lock(exchanges_mutex_);
    
    auto it = exchanges_.find(name);
    if (it == exchanges_.end()) {
        LOG_WARN("Exchange " + name + " not found");
        return false;
    }
    
    // Disconnect if connected
    if (it->second && it->second->isConnected()) {
        it->second->disconnect();
    }
    
    exchanges_.erase(it);
    
    // Remove from health status
    {
        std::lock_guard<std::mutex> health_lock(health_mutex_);
        health_status_.erase(name);
    }
    
    // Remove from metrics
    {
        std::lock_guard<std::mutex> metrics_lock(metrics_mutex_);
        metrics_.erase(name);
    }
    
    LOG_INFO("Removed exchange: " + name);
    return true;
}

ExchangeInterface* ExchangeManager::getExchange(const std::string& name) {
    std::lock_guard<std::mutex> lock(exchanges_mutex_);
    
    auto it = exchanges_.find(name);
    if (it != exchanges_.end()) {
        return it->second.get();
    }
    return nullptr;
}

const ExchangeInterface* ExchangeManager::getExchange(const std::string& name) const {
    std::lock_guard<std::mutex> lock(exchanges_mutex_);
    
    auto it = exchanges_.find(name);
    if (it != exchanges_.end()) {
        return it->second.get();
    }
    return nullptr;
}

bool ExchangeManager::connectExchange(const std::string& name) {
    auto exchange = getExchange(name);
    if (!exchange) {
        LOG_ERROR("Exchange " + name + " not found");
        return false;
    }
    
    LOG_INFO("Connecting to exchange: " + name);
    
    bool connected = exchange->connect();
    if (connected) {
        LOG_INFO("Successfully connected to exchange: " + name);
        
        // Update health status
        std::lock_guard<std::mutex> health_lock(health_mutex_);
        auto& health = health_status_[name];
        health.status = ExchangeHealthStatus::HEALTHY;
        health.last_check = std::chrono::system_clock::now();
        health.status_message = "Connected";
        
        // Notify callbacks
        notifyHealthCallbacks(name, health);
    } else {
        LOG_ERROR("Failed to connect to exchange: " + name);
        
        // Update health status
        std::lock_guard<std::mutex> health_lock(health_mutex_);
        auto& health = health_status_[name];
        health.status = ExchangeHealthStatus::CRITICAL;
        health.last_check = std::chrono::system_clock::now();
        health.status_message = "Connection failed";
        health.failed_requests++;
        
        // Notify callbacks
        notifyHealthCallbacks(name, health);
    }
    
    return connected;
}

bool ExchangeManager::disconnectExchange(const std::string& name) {
    auto exchange = getExchange(name);
    if (!exchange) {
        LOG_ERROR("Exchange " + name + " not found");
        return false;
    }
    
    LOG_INFO("Disconnecting from exchange: " + name);
    
    bool disconnected = exchange->disconnect();
    if (disconnected) {
        LOG_INFO("Successfully disconnected from exchange: " + name);
    } else {
        LOG_WARN("Failed to cleanly disconnect from exchange: " + name);
    }
    
    // Update health status
    std::lock_guard<std::mutex> health_lock(health_mutex_);
    auto& health = health_status_[name];
    health.status = ExchangeHealthStatus::DISCONNECTED;
    health.last_check = std::chrono::system_clock::now();
    health.status_message = "Disconnected";
    
    // Notify callbacks
    notifyHealthCallbacks(name, health);
    
    return disconnected;
}

bool ExchangeManager::reconnectExchange(const std::string& name) {
    LOG_INFO("Reconnecting to exchange: " + name);
    
    disconnectExchange(name);
    std::this_thread::sleep_for(std::chrono::seconds(1)); // Brief pause
    return connectExchange(name);
}

bool ExchangeManager::connectAllExchanges() {
    std::vector<std::string> exchange_names = getExchangeNames();
    bool all_connected = true;
    
    LOG_INFO("Connecting to all exchanges");
    
    for (const auto& name : exchange_names) {
        if (!connectExchange(name)) {
            all_connected = false;
        }
    }
    
    if (all_connected) {
        LOG_INFO("All exchanges connected successfully");
    } else {
        LOG_WARN("Some exchanges failed to connect");
    }
    
    return all_connected;
}

void ExchangeManager::disconnectAllExchanges() {
    std::vector<std::string> exchange_names = getExchangeNames();
    
    LOG_INFO("Disconnecting from all exchanges");
    
    for (const auto& name : exchange_names) {
        disconnectExchange(name);
    }
    
    LOG_INFO("All exchanges disconnected");
}

std::vector<std::string> ExchangeManager::getExchangeNames() const {
    std::lock_guard<std::mutex> lock(exchanges_mutex_);
    
    std::vector<std::string> names;
    names.reserve(exchanges_.size());
    
    for (const auto& pair : exchanges_) {
        names.push_back(pair.first);
    }
    
    return names;
}

std::vector<std::string> ExchangeManager::getConnectedExchanges() const {
    std::lock_guard<std::mutex> lock(exchanges_mutex_);
    
    std::vector<std::string> connected;
    
    for (const auto& pair : exchanges_) {
        if (pair.second && pair.second->isConnected()) {
            connected.push_back(pair.first);
        }
    }
    
    return connected;
}

std::vector<std::string> ExchangeManager::getDisconnectedExchanges() const {
    std::lock_guard<std::mutex> lock(exchanges_mutex_);
    
    std::vector<std::string> disconnected;
    
    for (const auto& pair : exchanges_) {
        if (!pair.second || !pair.second->isConnected()) {
            disconnected.push_back(pair.first);
        }
    }
    
    return disconnected;
}

bool ExchangeManager::isExchangeConnected(const std::string& name) const {
    auto exchange = getExchange(name);
    return exchange && exchange->isConnected();
}

ExchangeHealth ExchangeManager::getExchangeHealth(const std::string& name) const {
    std::lock_guard<std::mutex> lock(health_mutex_);
    
    auto it = health_status_.find(name);
    if (it != health_status_.end()) {
        return it->second;
    }
    
    // Return default health status if not found
    ExchangeHealth health;
    health.exchange_name = name;
    health.status = ExchangeHealthStatus::UNKNOWN;
    health.status_message = "Exchange not found";
    return health;
}

std::map<std::string, ExchangeHealth> ExchangeManager::getAllExchangeHealth() const {
    std::lock_guard<std::mutex> lock(health_mutex_);
    return health_status_;
}

void ExchangeManager::startHealthMonitoring() {
    if (monitoring_active_) {
        LOG_WARN("Health monitoring already active");
        return;
    }
    
    monitoring_active_ = true;
    health_monitor_thread_ = std::thread(&ExchangeManager::healthMonitorLoop, this);
    
    LOG_INFO("Health monitoring started");
}

void ExchangeManager::stopHealthMonitoring() {
    if (!monitoring_active_) {
        return;
    }
    
    monitoring_active_ = false;
    
    if (health_monitor_thread_.joinable()) {
        health_monitor_thread_.join();
    }
    
    LOG_INFO("Health monitoring stopped");
}

bool ExchangeManager::isHealthMonitoringActive() const {
    return monitoring_active_;
}

ExchangeMetrics ExchangeManager::getExchangeMetrics(const std::string& name) const {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    
    auto it = metrics_.find(name);
    if (it != metrics_.end()) {
        return it->second;
    }
    
    // Return default metrics if not found
    ExchangeMetrics metrics;
    metrics.exchange_name = name;
    return metrics;
}

std::map<std::string, ExchangeMetrics> ExchangeManager::getAllExchangeMetrics() const {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    return metrics_;
}

void ExchangeManager::updateMetrics(const std::string& exchange_name, const ExchangeMetrics& metrics) {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    metrics_[exchange_name] = metrics;
    
    // Notify callbacks
    notifyMetricsCallbacks(exchange_name, metrics);
}

bool ExchangeManager::loadExchangesFromConfig() {
    if (!config_manager_) {
        LOG_ERROR("Config manager not available");
        return false;
    }
    
    LOG_INFO("Loading exchanges from configuration");
    
    try {
        const auto& config = config_manager_->getConfig();
        
        if (!config.contains("multi_asset") || !config["multi_asset"].contains("exchanges")) {
            LOG_WARN("No exchanges configured");
            return true; // Not an error, just no exchanges configured
        }
        
        const auto& exchanges_config = config["multi_asset"]["exchanges"];
        
        for (const auto& exchange_config : exchanges_config) {
            if (!exchange_config.contains("name") || !exchange_config.contains("enabled")) {
                LOG_WARN("Invalid exchange configuration");
                continue;
            }
            
            std::string name = exchange_config["name"];
            bool enabled = exchange_config["enabled"];
            
            if (!enabled) {
                LOG_INFO("Exchange " + name + " is disabled, skipping");
                continue;
            }
            
            auto exchange = createExchange(name, exchange_config);
            if (exchange) {
                addExchange(name, std::move(exchange));
                LOG_INFO("Loaded exchange: " + name);
            } else {
                LOG_ERROR("Failed to create exchange: " + name);
            }
        }
        
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("Error loading exchanges from config: " + std::string(e.what()));
        return false;
    }
}

void ExchangeManager::setHealthCheckInterval(std::chrono::seconds interval) {
    health_check_interval_ = interval;
    LOG_INFO("Health check interval set to " + std::to_string(interval.count()) + " seconds");
}

void ExchangeManager::addHealthCallback(HealthCallback callback) {
    health_callbacks_.push_back(callback);
}

void ExchangeManager::addMetricsCallback(MetricsCallback callback) {
    metrics_callbacks_.push_back(callback);
}

void ExchangeManager::emergencyDisconnectAll(const std::string& reason) {
    LOG_CRITICAL("Emergency disconnect triggered: " + reason);
    
    // TODO: Set emergency stop flag
    disconnectAllExchanges();
    
    // Notify all callbacks about emergency state
    for (const auto& pair : health_status_) {
        auto health = pair.second;
        health.status = ExchangeHealthStatus::CRITICAL;
        health.status_message = "Emergency stop: " + reason;
        notifyHealthCallbacks(pair.first, health);
    }
}

bool ExchangeManager::isEmergencyStopActive() const {
    // TODO: Implement emergency stop state tracking
    return false;
}

void ExchangeManager::healthMonitorLoop() {
    LOG_INFO("Health monitor loop started");
    
    while (monitoring_active_) {
        auto exchange_names = getExchangeNames();
        
        for (const auto& name : exchange_names) {
            if (!monitoring_active_) break;
            checkExchangeHealth(name);
        }
        
        // Sleep for the specified interval
        auto sleep_duration = health_check_interval_;
        auto start_time = std::chrono::steady_clock::now();
        
        while (monitoring_active_ && 
               (std::chrono::steady_clock::now() - start_time) < sleep_duration) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
    
    LOG_INFO("Health monitor loop stopped");
}

void ExchangeManager::checkExchangeHealth(const std::string& exchange_name) {
    auto exchange = getExchange(exchange_name);
    if (!exchange) {
        return;
    }
    
    ExchangeHealth health;
    health.exchange_name = exchange_name;
    health.last_check = std::chrono::system_clock::now();
    
    // Check if exchange is connected
    if (!exchange->isConnected()) {
        health.status = ExchangeHealthStatus::DISCONNECTED;
        health.status_message = "Not connected";
    } else {
        // TODO: Implement more sophisticated health checks
        // For now, assume connected = healthy
        health.status = ExchangeHealthStatus::HEALTHY;
        health.status_message = "Connected and responsive";
        health.uptime_percentage = 100.0; // TODO: Calculate actual uptime
    }
    
    updateExchangeHealth(exchange_name, health);
}

void ExchangeManager::updateExchangeHealth(const std::string& exchange_name, const ExchangeHealth& health) {
    bool health_changed = false;
    
    {
        std::lock_guard<std::mutex> lock(health_mutex_);
        auto& current_health = health_status_[exchange_name];
        
        if (current_health.status != health.status) {
            health_changed = true;
        }
        
        current_health = health;
    }
    
    if (health_changed) {
        LOG_INFO("Health status changed for " + exchange_name + ": " + 
                healthStatusToString(health.status));
        notifyHealthCallbacks(exchange_name, health);
    }
}

void ExchangeManager::notifyHealthCallbacks(const std::string& exchange_name, const ExchangeHealth& health) {
    for (const auto& callback : health_callbacks_) {
        try {
            callback(exchange_name, health);
        } catch (const std::exception& e) {
            LOG_ERROR("Error in health callback: " + std::string(e.what()));
        }
    }
}

void ExchangeManager::notifyMetricsCallbacks(const std::string& exchange_name, const ExchangeMetrics& metrics) {
    for (const auto& callback : metrics_callbacks_) {
        try {
            callback(exchange_name, metrics);
        } catch (const std::exception& e) {
            LOG_ERROR("Error in metrics callback: " + std::string(e.what()));
        }
    }
}

std::unique_ptr<ExchangeInterface> ExchangeManager::createExchange(const std::string& exchange_type, 
                                                                  const nlohmann::json& config) {
    if (exchange_type == "binance") {
        auto exchange = std::make_unique<BinanceExchange>();
        // TODO: Initialize exchange with config
        return std::move(exchange);
    }
    
    // TODO: Add support for other exchanges
    LOG_ERROR("Unknown exchange type: " + exchange_type);
    return nullptr;
}

ExchangeHealthStatus ExchangeManager::determineHealthStatus(const ExchangeHealth& health) const {
    // TODO: Implement more sophisticated health determination logic
    return health.status;
}

std::string ExchangeManager::healthStatusToString(ExchangeHealthStatus status) const {
    switch (status) {
        case ExchangeHealthStatus::HEALTHY: return "Healthy";
        case ExchangeHealthStatus::WARNING: return "Warning";
        case ExchangeHealthStatus::CRITICAL: return "Critical";
        case ExchangeHealthStatus::DISCONNECTED: return "Disconnected";
        case ExchangeHealthStatus::UNKNOWN: return "Unknown";
        default: return "Unknown";
    }
}

} // namespace moneybot
