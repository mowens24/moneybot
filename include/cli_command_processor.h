#pragma once

#include <memory>
#include <string>
#include <vector>
#include "simple_logger.h"
#include "config_manager.h"
#include "simple_portfolio_manager.h"
#include "system_manager.h"
#include "strategy_manager.h"

namespace moneybot {

class CLICommandProcessor {
public:
    CLICommandProcessor();
    
    // Main command processing
    int processCommand(int argc, char** argv);
    
private:
    // Core managers
    std::shared_ptr<SimpleLogger> logger_;
    ConfigManager& config_;
    std::shared_ptr<SimplePortfolioManager> portfolio_;
    std::shared_ptr<SystemManager> system_;
    std::shared_ptr<StrategyManager> strategy_;
    
    // Command handlers
    int cmd_start(const std::vector<std::string>& args);
    int cmd_stop(const std::vector<std::string>& args);
    int cmd_status(const std::vector<std::string>& args);
    int cmd_portfolio(const std::vector<std::string>& args);
    int cmd_strategies(const std::vector<std::string>& args);
    int cmd_risk_check(const std::vector<std::string>& args);
    int cmd_config(const std::vector<std::string>& args);
    int cmd_version(const std::vector<std::string>& args);
    
    // Utilities
    void print_usage();
};

} // namespace moneybot
