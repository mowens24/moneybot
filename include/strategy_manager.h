#pragma once

#include <memory>
#include <string>
#include <vector>
#include "simple_logger.h"
#include "config_manager.h"

namespace moneybot {

struct StrategyInfo {
    std::string name;
    std::string type;
    bool enabled;
    bool running;
    double pnl;
    std::map<std::string, std::string> parameters;
};

class StrategyManager {
public:
    StrategyManager(std::shared_ptr<SimpleLogger> logger, ConfigManager& config);
    
    // Strategy operations
    bool startStrategy(const std::string& name);
    bool stopStrategy(const std::string& name);
    bool isStrategyRunning(const std::string& name) const;
    
    // Strategy information
    std::vector<StrategyInfo> getAvailableStrategies() const;
    std::vector<StrategyInfo> getActiveStrategies() const;
    int getActiveStrategyCount() const;
    
private:
    std::shared_ptr<SimpleLogger> logger_;
    ConfigManager& config_;
    std::map<std::string, bool> strategy_states_;
    
    StrategyInfo createStrategyInfo(const std::string& name, const nlohmann::json& config) const;
};

} // namespace moneybot
