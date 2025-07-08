#include "backtest_engine.h"
#include "logger.h"
#include <fstream>
#include <iostream>
#include <cmath>

namespace moneybot {

BacktestEngine::BacktestEngine(const nlohmann::json& config)
    : config_(config) {}

void BacktestEngine::setStrategy(std::shared_ptr<Strategy> strategy) {
    strategy_ = strategy;
}

void BacktestEngine::setLogger(std::shared_ptr<Logger> logger) {
    logger_ = logger;
}

BacktestResult BacktestEngine::run(const std::string& symbol, const std::string& data_path) {
    BacktestResult result;
    std::ifstream file(data_path);
    if (!file.is_open()) {
        if (logger_) logger_->getLogger()->error("Failed to open backtest data: {}", data_path);
        return result;
    }
    
    std::string line;
    double equity = 0.0;
    double peak = 0.0;
    double trough = 0.0;
    std::vector<double> returns;
    OrderBook ob(logger_); // Create order book once, pass logger
    
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        
        try {
            nlohmann::json tick = nlohmann::json::parse(line);
            // Simulate order book update
            ob.update(tick);
            if (strategy_) strategy_->onOrderBookUpdate(ob);
            // Simulate a trade for demonstration
            result.total_trades++;
            // For now, just collect equity curve
            result.equity_curve.push_back(equity);
            if (equity > peak) peak = equity;
            if (equity < trough) trough = equity;
            double dd = (peak > 0) ? (peak - equity) / peak : 0.0;
            if (dd > result.max_drawdown) result.max_drawdown = dd;
        } catch (const std::exception& e) {
            if (logger_) logger_->getLogger()->warn("Failed to parse line: {}", line);
        }
    }
    
    // Compute Sharpe ratio (placeholder)
    if (!returns.empty()) {
        double mean = 0.0, stddev = 0.0;
        for (double r : returns) mean += r;
        mean /= returns.size();
        for (double r : returns) stddev += (r - mean) * (r - mean);
        stddev = std::sqrt(stddev / returns.size());
        result.sharpe_ratio = (stddev > 0) ? mean / stddev : 0.0;
    }
    result.total_pnl = equity;
    // result.total_trades, result.wins, result.losses to be filled by strategy callbacks
    return result;
}

} // namespace moneybot
