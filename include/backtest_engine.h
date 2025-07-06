#pragma once
#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include <memory>
#include "strategy.h"
#include "order_book.h"

namespace moneybot {

struct BacktestResult {
    double total_pnl = 0.0;
    double max_drawdown = 0.0;
    int total_trades = 0;
    int wins = 0;
    int losses = 0;
    double sharpe_ratio = 0.0;
    std::vector<double> equity_curve;
};

class BacktestEngine {
public:
    BacktestEngine(const nlohmann::json& config);
    void setStrategy(std::shared_ptr<Strategy> strategy);
    BacktestResult run(const std::string& symbol, const std::string& data_path);
    void setLogger(std::shared_ptr<Logger> logger);
private:
    nlohmann::json config_;
    std::shared_ptr<Strategy> strategy_;
    std::shared_ptr<Logger> logger_;
};

} // namespace moneybot
