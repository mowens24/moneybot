#include "../include/system_manager.h"
#include <chrono>

namespace moneybot {

SystemManager::SystemManager(std::shared_ptr<SimpleLogger> logger, ConfigManager& config)
    : logger_(logger), config_(config) {
    logger_->info("SystemManager initialized");
}

bool SystemManager::start() {
    if (is_running_) {
        logger_->warning("System already running");
        return false;
    }
    
    logger_->info("Starting MoneyBot trading system");
    
    // TODO: Initialize actual trading components
    // - Exchange connections
    // - Strategy controllers
    // - Risk managers
    
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
    
    // TODO: Graceful shutdown of trading components
    // - Cancel all orders
    // - Close positions safely
    // - Disconnect from exchanges
    
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

} // namespace moneybot
