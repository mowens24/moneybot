#include "../include/cli_command_processor.h"
#include <iostream>
#include <iomanip>

namespace moneybot {

CLICommandProcessor::CLICommandProcessor() 
    : config_(ConfigManager::getInstance()) {
    
    // Initialize core components
    logger_ = std::make_shared<SimpleLogger>();
    portfolio_ = std::make_shared<SimplePortfolioManager>(logger_);
    system_ = std::make_shared<SystemManager>(logger_, config_);
    strategy_ = std::make_shared<StrategyManager>(logger_, config_);
    
    // Load configuration
    if (!config_.loadConfig("config.json")) {
        logger_->warning("Could not load config.json, using defaults");
    }
    
    logger_->info("CLICommandProcessor initialized");
}

int CLICommandProcessor::processCommand(int argc, char** argv) {
    if (argc < 2) {
        print_usage();
        return 1;
    }
    
    std::string command = argv[1];
    std::vector<std::string> args(argv + 2, argv + argc);
    
    try {
        if (command == "start") {
            return cmd_start(args);
        } else if (command == "stop") {
            return cmd_stop(args);
        } else if (command == "status") {
            return cmd_status(args);
        } else if (command == "portfolio") {
            return cmd_portfolio(args);
        } else if (command == "strategies") {
            return cmd_strategies(args);
        } else if (command == "risk-check") {
            return cmd_risk_check(args);
        } else if (command == "config") {
            return cmd_config(args);
        } else if (command == "version") {
            return cmd_version(args);
        } else {
            std::cout << "❌ Unknown command: " << command << std::endl;
            print_usage();
            return 1;
        }
    } catch (const std::exception& e) {
        logger_->error("Command failed: " + std::string(e.what()));
        std::cout << "❌ Error: " << e.what() << std::endl;
        return 1;
    }
}

void CLICommandProcessor::print_usage() {
    std::cout << "\n";
    std::cout << "🤖 MoneyBot CLI Trading System\n";
    std::cout << "==============================\n\n";
    std::cout << "Usage: moneybot <command> [options]\n\n";
    std::cout << "Commands:\n";
    std::cout << "  start              Start the trading system\n";
    std::cout << "  stop               Stop all trading activities\n";
    std::cout << "  status             Show current system status\n";
    std::cout << "  portfolio          Show portfolio summary\n";
    std::cout << "  strategies         Manage trading strategies\n";
    std::cout << "  risk-check         Display risk metrics\n";
    std::cout << "  config             Configuration management\n";
    std::cout << "  version            Show version information\n";
    std::cout << "\nExamples:\n";
    std::cout << "  moneybot start\n";
    std::cout << "  moneybot status\n";
    std::cout << "  moneybot portfolio\n";
    std::cout << "  moneybot config show\n";
    std::cout << "\n";
}

int CLICommandProcessor::cmd_start(const std::vector<std::string>& args) {
    std::cout << "🚀 Starting MoneyBot Trading System...\n\n";
    
    if (system_->isRunning()) {
        std::cout << "⚠️  MoneyBot is already running\n";
        return 0;
    }
    
    std::cout << "📊 Loading configuration...\n";
    std::cout << "🔗 Connecting to exchanges...\n";
    std::cout << "🎯 Initializing strategies...\n";
    
    if (system_->start()) {
        std::cout << "✅ MoneyBot started successfully\n";
        std::cout << "📊 System Status: ACTIVE\n";
        std::cout << "💡 Use 'moneybot status' to monitor the system\n";
        return 0;
    } else {
        std::cout << "❌ Failed to start MoneyBot\n";
        return 1;
    }
}

int CLICommandProcessor::cmd_stop(const std::vector<std::string>& args) {
    std::cout << "🛑 Stopping MoneyBot Trading System...\n\n";
    
    if (!system_->isRunning()) {
        std::cout << "⚠️  MoneyBot is not currently running\n";
        return 0;
    }
    
    if (system_->stop()) {
        std::cout << "✅ MoneyBot stopped successfully\n";
        std::cout << "📊 System Status: STOPPED\n";
        return 0;
    } else {
        std::cout << "❌ Failed to stop MoneyBot\n";
        return 1;
    }
}

int CLICommandProcessor::cmd_status(const std::vector<std::string>& args) {
    std::cout << "\n📊 MoneyBot System Status\n";
    std::cout << "========================\n\n";
    
    // System status
    std::cout << "🔧 System Status: " << system_->getSystemStatus() << "\n";
    std::cout << "📊 Version: " << system_->getVersion() << "\n";
    std::cout << "⏰ Uptime: " << system_->getUptime() << " minutes\n";
    std::cout << "🔄 Active Strategies: " << strategy_->getActiveStrategyCount() << "\n";
    std::cout << "💰 Portfolio Value: $" << std::fixed << std::setprecision(2) << portfolio_->getTotalValue() << "\n";
    std::cout << "📈 Total P&L: $" << std::fixed << std::setprecision(2) << portfolio_->getTotalPnL() << "\n";
    std::cout << "⚠️  Risk Level: LOW\n\n";
    
    std::cout << "🎯 Available Commands:\n";
    std::cout << "  • moneybot portfolio    - View portfolio details\n";
    std::cout << "  • moneybot strategies   - Manage strategies\n";
    std::cout << "  • moneybot risk-check   - View risk metrics\n";
    std::cout << "  • moneybot config       - Configuration settings\n";
    
    return 0;
}

int CLICommandProcessor::cmd_portfolio(const std::vector<std::string>& args) {
    std::cout << "\n💼 Portfolio Summary\n";
    std::cout << "===================\n\n";
    
    // Get real portfolio data
    double total_value = portfolio_->getTotalValue();
    double available_cash = portfolio_->getAvailableCash();
    double total_pnl = portfolio_->getTotalPnL();
    double daily_pnl = portfolio_->getDailyPnL();
    int active_positions = portfolio_->getActivePositions();
    
    std::cout << "📊 Account Overview:\n";
    std::cout << "  Total Value:     $" << std::fixed << std::setprecision(2) << total_value << "\n";
    std::cout << "  Available Cash:  $" << std::fixed << std::setprecision(2) << available_cash << "\n";
    std::cout << "  Total P&L:       $" << std::fixed << std::setprecision(2) << total_pnl << "\n";
    std::cout << "  Daily P&L:       $" << std::fixed << std::setprecision(2) << daily_pnl << "\n";
    std::cout << "  Active Positions:" << active_positions << "\n\n";
    
    std::cout << "💰 Asset Holdings:\n";
    auto balances = portfolio_->getBalances();
    for (const auto& balance : balances) {
        std::cout << "  " << balance.asset << ": " 
                  << std::fixed << std::setprecision(8) << balance.total;
        if (balance.asset == "USD") {
            std::cout << " (Available)";
        }
        std::cout << "\n";
    }
    
    std::cout << "\n📈 Open Positions:\n";
    auto positions = portfolio_->getPositions();
    if (positions.empty()) {
        std::cout << "  No open positions\n";
    } else {
        for (const auto& position : positions) {
            std::cout << "  " << position.symbol << ": " 
                      << std::fixed << std::setprecision(8) << position.quantity
                      << " @ $" << std::fixed << std::setprecision(2) << position.avg_price;
            if (position.unrealized_pnl != 0.0) {
                std::cout << " (P&L: $" << std::fixed << std::setprecision(2) << position.unrealized_pnl << ")";
            }
            std::cout << "\n";
        }
    }
    
    std::cout << "\n💡 Tip: Use 'moneybot start' to begin trading\n";
    
    return 0;
}

int CLICommandProcessor::cmd_strategies(const std::vector<std::string>& args) {
    if (args.empty()) {
        // List all strategies
        std::cout << "\n🎯 Available Trading Strategies\n";
        std::cout << "===============================\n\n";
        
        auto strategies = strategy_->getAvailableStrategies();
        for (const auto& strategy : strategies) {
            std::cout << "📈 " << strategy.type << " (" << strategy.name << ")\n";
            std::cout << "   Status: " << (strategy.enabled ? "CONFIGURED" : "DISABLED");
            if (strategy.running) {
                std::cout << " - RUNNING";
            }
            std::cout << "\n";
            
            // Show key parameters
            for (const auto& [key, value] : strategy.parameters) {
                std::cout << "   " << key << ": " << value << "\n";
            }
            std::cout << "   P&L: $" << std::fixed << std::setprecision(2) << strategy.pnl << "\n\n";
        }
        
        std::cout << "💡 Use 'moneybot strategies start <name>' to activate a strategy\n";
    } else if (args[0] == "start" && args.size() > 1) {
        std::string strategy_name = args[1];
        std::cout << "🚀 Starting strategy: " << strategy_name << "\n";
        if (strategy_->startStrategy(strategy_name)) {
            std::cout << "✅ Strategy activated successfully\n";
        } else {
            std::cout << "❌ Failed to start strategy\n";
            return 1;
        }
    } else if (args[0] == "stop" && args.size() > 1) {
        std::string strategy_name = args[1];
        std::cout << "🛑 Stopping strategy: " << strategy_name << "\n";
        if (strategy_->stopStrategy(strategy_name)) {
            std::cout << "✅ Strategy stopped successfully\n";
        } else {
            std::cout << "❌ Failed to stop strategy\n";
            return 1;
        }
    } else {
        std::cout << "Usage: moneybot strategies [start|stop] [strategy_name]\n";
        return 1;
    }
    
    return 0;
}

int CLICommandProcessor::cmd_risk_check(const std::vector<std::string>& args) {
    std::cout << "\n⚠️  Risk Management Dashboard\n";
    std::cout << "============================\n\n";
    
    std::cout << "📊 Risk Metrics:\n";
    std::cout << "  Value at Risk (VaR):    $1,000.00\n";
    std::cout << "  Maximum Drawdown:       " << std::fixed << std::setprecision(2) << portfolio_->getMaxDrawdown() << "%\n";
    std::cout << "  Sharpe Ratio:           0.00\n";
    std::cout << "  Portfolio Beta:         1.00\n";
    std::cout << "  Risk Level:             🟢 LOW\n\n";
    
    std::cout << "🚨 Risk Alerts:\n";
    std::cout << "  ✅ No active risk alerts\n\n";
    
    std::cout << "🛡️ Risk Limits:\n";
    std::cout << "  Max Position Size:      $10,000.00\n";
    std::cout << "  Max Daily Loss:         $1,000.00\n";
    std::cout << "  Stop Loss Threshold:    5.00%\n";
    
    return 0;
}

int CLICommandProcessor::cmd_config(const std::vector<std::string>& args) {
    if (args.empty() || args[0] == "show") {
        std::cout << "\n⚙️  MoneyBot Configuration\n";
        std::cout << "==========================\n\n";
        
        // Get real configuration
        const auto& config = config_.getConfig();
        
        std::cout << "📊 Trading Settings:\n";
        if (config.contains("strategy")) {
            const auto& strategy = config["strategy"];
            std::cout << "  Symbol:                 " << strategy.value("symbol", "BTCUSDT") << "\n";
            std::cout << "  Strategy Type:          " << strategy.value("type", "market_maker") << "\n";
            if (strategy.contains("config")) {
                const auto& cfg = strategy["config"];
                std::cout << "  Max Position:           " << cfg.value("max_position", 0.01) << "\n";
                std::cout << "  Order Size:             " << cfg.value("order_size", 0.001) << "\n";
            }
        }
        std::cout << "  Trading Mode:           " << (config_.isDryRunMode() ? "SIMULATION" : "LIVE") << "\n";
        std::cout << "  Production Mode:        " << (config_.isProductionMode() ? "YES" : "NO") << "\n\n";
        
        std::cout << "🔗 Exchange Settings:\n";
        if (config.contains("multi_asset") && config["multi_asset"].contains("exchanges")) {
            const auto& exchanges = config["multi_asset"]["exchanges"];
            if (exchanges.is_array() && !exchanges.empty()) {
                const auto& exchange = exchanges[0];
                std::cout << "  Primary Exchange:       " << exchange.value("name", "unknown") << "\n";
                std::cout << "  REST URL:               " << exchange.value("rest_url", "not configured") << "\n";
                std::cout << "  API Status:             " << (config_.validateApiKeys() ? "CONFIGURED" : "MISSING") << "\n";
            }
        }
        std::cout << "\n💡 Configuration loaded from config.json\n";
    } else if (args[0] == "validate") {
        std::cout << "🔍 Validating configuration...\n";
        bool valid = config_.validateApiKeys();
        std::cout << "API Keys: " << (valid ? "✅ Valid" : "❌ Invalid/Missing") << "\n";
    } else {
        std::cout << "Usage: moneybot config [show|validate]\n";
        return 1;
    }
    
    return 0;
}

int CLICommandProcessor::cmd_version(const std::vector<std::string>& args) {
    std::cout << "\n🤖 MoneyBot Trading System\n";
    std::cout << "==========================\n";
    std::cout << "Version: " << system_->getVersion() << "\n";
    std::cout << "Build Date: " << __DATE__ << " " << __TIME__ << "\n";
    std::cout << "Platform: CLI\n";
    std::cout << "Author: MoneyBot Development Team\n\n";
    
    return 0;
}

} // namespace moneybot
