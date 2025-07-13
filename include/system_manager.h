#pragma once

#include <memory>
#include <string>
#include <vector>
#include <chrono>
#include "simple_logger.h"
#include "config_manager.h"
#include "core/exchange_manager.h"

namespace moneybot {

class SystemManager {
public:
    SystemManager(std::shared_ptr<SimpleLogger> logger, ConfigManager& config);
    
    // System operations
    bool start();
    bool stop();
    bool isRunning() const;
    
    // Status information
    std::string getSystemStatus() const;
    std::string getVersion() const;
    int getUptime() const;
    
    // Exchange information
    bool areExchangesConnected() const;
    std::vector<ExchangeStatus> getExchangeStatuses() const;
    
private:
    std::shared_ptr<SimpleLogger> logger_;
    ConfigManager& config_;
    std::shared_ptr<ExchangeManager> exchange_manager_;
    bool is_running_ = false;
    std::chrono::steady_clock::time_point start_time_;
};

} // namespace moneybot
