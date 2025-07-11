#include "application_state.h"
#include "modern_logger.h"
#include <sstream>

namespace moneybot {

ApplicationStateManager::ApplicationStateManager() 
    : start_time_(std::chrono::steady_clock::now()) {
}

void ApplicationStateManager::setState(AppState state) {
    AppState old_state = current_state_.load();
    
    if (!isStateTransitionValid(old_state, state)) {
        LOG_WARN("Invalid state transition");
        return;
    }
    
    current_state_ = state;
    LOG_INFO("State changed");
    
    notifyStateChange(old_state, state);
}

AppState ApplicationStateManager::getState() const {
    return current_state_.load();
}

std::string ApplicationStateManager::getStateString() const {
    switch (current_state_.load()) {
        case AppState::INITIALIZING: return "INITIALIZING";
        case AppState::CONNECTING: return "CONNECTING";
        case AppState::CONNECTED: return "CONNECTED";
        case AppState::TRADING: return "TRADING";
        case AppState::PAUSED: return "PAUSED";
        case AppState::DISCONNECTING: return "DISCONNECTING";
        case AppState::SHUTDOWN: return "SHUTDOWN";
        case AppState::ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}

bool ApplicationStateManager::isStateTransitionValid(AppState from, AppState to) const {
    // Define valid state transitions
    switch (from) {
        case AppState::INITIALIZING:
            return to == AppState::CONNECTING || to == AppState::ERROR || to == AppState::SHUTDOWN;
        case AppState::CONNECTING:
            return to == AppState::CONNECTED || to == AppState::ERROR || to == AppState::SHUTDOWN;
        case AppState::CONNECTED:
            return to == AppState::TRADING || to == AppState::PAUSED || 
                   to == AppState::DISCONNECTING || to == AppState::ERROR;
        case AppState::TRADING:
            return to == AppState::PAUSED || to == AppState::DISCONNECTING || to == AppState::ERROR;
        case AppState::PAUSED:
            return to == AppState::TRADING || to == AppState::DISCONNECTING || to == AppState::ERROR;
        case AppState::DISCONNECTING:
            return to == AppState::SHUTDOWN || to == AppState::ERROR;
        case AppState::ERROR:
            return to == AppState::SHUTDOWN || to == AppState::INITIALIZING;
        case AppState::SHUTDOWN:
            return false; // No transitions from shutdown
        default:
            return false;
    }
}

void ApplicationStateManager::setTradingMode(TradingMode mode) {
    trading_mode_ = mode;
    LOG_INFO("Trading mode set");
}

TradingMode ApplicationStateManager::getTradingMode() const {
    return trading_mode_.load();
}

std::string ApplicationStateManager::getTradingModeString() const {
    switch (trading_mode_.load()) {
        case TradingMode::DEMO: return "DEMO";
        case TradingMode::PAPER: return "PAPER";
        case TradingMode::LIVE: return "LIVE";
        default: return "UNKNOWN";
    }
}

void ApplicationStateManager::setConfig(const std::string& key, const std::string& value) {
    std::lock_guard<std::mutex> lock(state_mutex_);
    config_[key] = value;
}

std::string ApplicationStateManager::getConfig(const std::string& key, const std::string& default_value) const {
    std::lock_guard<std::mutex> lock(state_mutex_);
    auto it = config_.find(key);
    return it != config_.end() ? it->second : default_value;
}

bool ApplicationStateManager::hasConfig(const std::string& key) const {
    std::lock_guard<std::mutex> lock(state_mutex_);
    return config_.find(key) != config_.end();
}

void ApplicationStateManager::setExchangeConnected(const std::string& exchange, bool connected) {
    std::lock_guard<std::mutex> lock(state_mutex_);
    exchange_connections_[exchange] = connected;
    LOG_INFO("Exchange connection status changed");
}

bool ApplicationStateManager::isExchangeConnected(const std::string& exchange) const {
    std::lock_guard<std::mutex> lock(state_mutex_);
    auto it = exchange_connections_.find(exchange);
    return it != exchange_connections_.end() && it->second;
}

size_t ApplicationStateManager::getConnectedExchangeCount() const {
    std::lock_guard<std::mutex> lock(state_mutex_);
    size_t count = 0;
    for (const auto& pair : exchange_connections_) {
        if (pair.second) count++;
    }
    return count;
}

void ApplicationStateManager::setError(const std::string& error_message) {
    std::lock_guard<std::mutex> lock(state_mutex_);
    last_error_ = error_message;
    has_error_ = true;
    LOG_ERROR("Application error: " + error_message);
}

std::string ApplicationStateManager::getLastError() const {
    std::lock_guard<std::mutex> lock(state_mutex_);
    return last_error_;
}

bool ApplicationStateManager::hasError() const {
    return has_error_.load();
}

void ApplicationStateManager::clearError() {
    std::lock_guard<std::mutex> lock(state_mutex_);
    last_error_.clear();
    has_error_ = false;
}

void ApplicationStateManager::incrementOrderCount() {
    order_count_++;
}

void ApplicationStateManager::incrementTradeCount() {
    trade_count_++;
}

void ApplicationStateManager::updatePnL(double pnl) {
    total_pnl_ += pnl;
}

size_t ApplicationStateManager::getOrderCount() const {
    return order_count_.load();
}

size_t ApplicationStateManager::getTradeCount() const {
    return trade_count_.load();
}

double ApplicationStateManager::getTotalPnL() const {
    return total_pnl_.load();
}

void ApplicationStateManager::addStateChangeCallback(const std::string& name, StateChangeCallback callback) {
    std::lock_guard<std::mutex> lock(state_mutex_);
    state_callbacks_[name] = callback;
}

void ApplicationStateManager::removeStateChangeCallback(const std::string& name) {
    std::lock_guard<std::mutex> lock(state_mutex_);
    state_callbacks_.erase(name);
}

void ApplicationStateManager::requestShutdown() {
    shutdown_requested_ = true;
    LOG_INFO("Shutdown requested");
}

bool ApplicationStateManager::isShutdownRequested() const {
    return shutdown_requested_.load();
}

void ApplicationStateManager::setShutdownComplete() {
    shutdown_complete_ = true;
    LOG_INFO("Shutdown complete");
}

std::chrono::steady_clock::time_point ApplicationStateManager::getStartTime() const {
    return start_time_;
}

std::chrono::seconds ApplicationStateManager::getUptime() const {
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::seconds>(now - start_time_);
}

void ApplicationStateManager::notifyStateChange(AppState old_state, AppState new_state) {
    std::lock_guard<std::mutex> lock(state_mutex_);
    for (const auto& pair : state_callbacks_) {
        try {
            pair.second(old_state, new_state);
        } catch (const std::exception& e) {
            LOG_ERROR("Error in state change callback");
        }
    }
}

} // namespace moneybot
