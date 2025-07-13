#pragma once

#include <memory>
#include <string>
#include <vector>
#include "simple_logger.h"
#include "config_manager.h"

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
    
private:
    std::shared_ptr<SimpleLogger> logger_;
    ConfigManager& config_;
    bool is_running_ = false;
    std::chrono::steady_clock::time_point start_time_;
};

} // namespace moneybot
