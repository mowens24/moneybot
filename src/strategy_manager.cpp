#include "../include/strategy_manager.h"

namespace moneybot {

StrategyManager::StrategyManager(std::shared_ptr<SimpleLogger> logger, ConfigManager& config)
    : logger_(logger), config_(config) {
    logger_->info("StrategyManager initialized");
}

bool StrategyManager::startStrategy(const std::string& name) {
    logger_->info("Starting strategy: " + name);
    
    // TODO: Actually start the strategy
    // - Validate strategy configuration
    // - Initialize strategy instance
    // - Begin strategy execution
    
    strategy_states_[name] = true;
    return true;
}

bool StrategyManager::stopStrategy(const std::string& name) {
    logger_->info("Stopping strategy: " + name);
    
    // TODO: Gracefully stop the strategy
    // - Cancel strategy orders
    // - Save strategy state
    // - Clean up resources
    
    strategy_states_[name] = false;
    return true;
}

bool StrategyManager::isStrategyRunning(const std::string& name) const {
    auto it = strategy_states_.find(name);
    return it != strategy_states_.end() && it->second;
}

std::vector<StrategyInfo> StrategyManager::getAvailableStrategies() const {
    std::vector<StrategyInfo> strategies;
    const auto& config = config_.getConfig();
    
    // Multi-asset strategies
    if (config.contains("strategies")) {
        const auto& strategies_config = config["strategies"];
        
        if (strategies_config.contains("cross_exchange_arbitrage")) {
            const auto& arb = strategies_config["cross_exchange_arbitrage"];
            StrategyInfo info;
            info.name = "cross_exchange_arbitrage";
            info.type = "Arbitrage";
            info.enabled = arb.value("enabled", false);
            info.running = isStrategyRunning(info.name);
            info.pnl = 0.0; // TODO: Get real P&L
            info.parameters["min_profit_bps"] = std::to_string(arb.value("min_profit_bps", 0.0));
            info.parameters["max_position_size"] = std::to_string(arb.value("max_position_size", 0.0));
            strategies.push_back(info);
        }
        
        if (strategies_config.contains("statistical_arbitrage")) {
            const auto& stat_arb = strategies_config["statistical_arbitrage"];
            StrategyInfo info;
            info.name = "statistical_arbitrage";
            info.type = "Statistical Arbitrage";
            info.enabled = stat_arb.value("enabled", false);
            info.running = isStrategyRunning(info.name);
            info.pnl = 0.0;
            info.parameters["lookback_periods"] = std::to_string(stat_arb.value("lookback_periods", 0));
            info.parameters["max_position_size"] = std::to_string(stat_arb.value("max_position_size", 0.0));
            strategies.push_back(info);
        }
        
        if (strategies_config.contains("portfolio_optimization")) {
            const auto& portfolio_opt = strategies_config["portfolio_optimization"];
            StrategyInfo info;
            info.name = "portfolio_optimization";
            info.type = "Portfolio Optimization";
            info.enabled = portfolio_opt.value("enabled", false);
            info.running = isStrategyRunning(info.name);
            info.pnl = 0.0;
            info.parameters["total_capital"] = std::to_string(portfolio_opt.value("total_capital", 0.0));
            info.parameters["rebalance_frequency_hours"] = std::to_string(portfolio_opt.value("rebalance_frequency_hours", 0));
            strategies.push_back(info);
        }
    }
    
    // Single strategy mode
    if (config.contains("strategy")) {
        const auto& strategy = config["strategy"];
        StrategyInfo info;
        info.name = "market_maker";
        info.type = "Market Making";
        info.enabled = true;
        info.running = isStrategyRunning(info.name);
        info.pnl = 0.0;
        info.parameters["symbol"] = strategy.value("symbol", "BTCUSDT");
        info.parameters["type"] = strategy.value("type", "market_maker");
        
        if (strategy.contains("config")) {
            const auto& cfg = strategy["config"];
            info.parameters["base_spread_bps"] = std::to_string(cfg.value("base_spread_bps", 0.0));
            info.parameters["order_size"] = std::to_string(cfg.value("order_size", 0.0));
        }
        strategies.push_back(info);
    }
    
    return strategies;
}

std::vector<StrategyInfo> StrategyManager::getActiveStrategies() const {
    auto all_strategies = getAvailableStrategies();
    std::vector<StrategyInfo> active_strategies;
    
    for (const auto& strategy : all_strategies) {
        if (strategy.running) {
            active_strategies.push_back(strategy);
        }
    }
    
    return active_strategies;
}

int StrategyManager::getActiveStrategyCount() const {
    return static_cast<int>(getActiveStrategies().size());
}

} // namespace moneybot
