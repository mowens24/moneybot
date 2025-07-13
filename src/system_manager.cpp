#include "../include/system_manager.h"
#include <chrono>

namespace moneybot {

SystemManager::SystemManager(std::shared_ptr<SimpleLogger> logger, ConfigManager& config)
    : logger_(logger), config_(config) {
    
    // Initialize exchange manager
    exchange_manager_ = std::make_shared<ExchangeManager>(logger_, config_);
    
    logger_->info("SystemManager initialized");
}

bool SystemManager::start() {
    if (is_running_) {
        logger_->warning("System already running");
        return false;
    }
    
    logger_->info("Starting MoneyBot trading system");
    
    // Connect to exchanges
    if (!exchange_manager_->connectToExchanges()) {
        logger_->error("Failed to connect to exchanges");
        return false;
    }
    
    // Start market data streaming
    exchange_manager_->startMarketDataStream();
    
    // TODO: Initialize other trading components
    // - Strategy controllers
    // - Risk managers
    // - Order managers
    
    is_running_ = true;
    start_time_ = std::chrono::steady_clock::now();
    
    logger_->info("MoneyBot trading system started successfully");
    return true;
}

bool SystemManager::stop() {
    if (!is_running_) {
        logger_->warning("System not running");
        return false;
    }
    
    logger_->info("Stopping MoneyBot trading system");
    
    // Stop market data streaming
    exchange_manager_->stopMarketDataStream();
    
    // Disconnect from exchanges
    exchange_manager_->disconnectFromExchanges();
    
    // TODO: Graceful shutdown of other trading components
    // - Cancel all orders
    // - Close positions safely
    // - Save state
    
    is_running_ = false;
    
    logger_->info("MoneyBot trading system stopped successfully");
    return true;
}

bool SystemManager::isRunning() const {
    return is_running_;
}

std::string SystemManager::getSystemStatus() const {
    return is_running_ ? "🟢 RUNNING" : "🔴 STOPPED";
}

std::string SystemManager::getVersion() const {
    return "1.0.0";
}

int SystemManager::getUptime() const {
    if (!is_running_) return 0;
    
    auto now = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::minutes>(now - start_time_);
    return static_cast<int>(duration.count());
}

bool SystemManager::areExchangesConnected() const {
    return exchange_manager_->isConnected();
}

std::vector<ExchangeStatus> SystemManager::getExchangeStatuses() const {
    return exchange_manager_->getExchangeStatuses();
}

} // namespace moneybot
