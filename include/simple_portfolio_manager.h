#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include <mutex>
#include "types.h"
#include "simple_logger.h"

namespace moneybot {

class SimplePortfolioManager {
public:
    SimplePortfolioManager(std::shared_ptr<SimpleLogger> logger);
    
    // Portfolio operations
    void updateBalance(const std::string& asset, double free, double locked);
    void updatePosition(const std::string& symbol, double quantity, double avg_price);
    void updatePnL(const std::string& symbol, double unrealized_pnl, double realized_pnl);
    
    // Portfolio queries
    std::vector<Balance> getBalances() const;
    std::vector<Position> getPositions() const;
    double getTotalValue() const;
    double getAvailableCash() const;
    double getTotalPnL() const;
    double getDailyPnL() const;
    
    // Risk metrics
    double getDrawdown() const;
    double getMaxDrawdown() const;
    int getActivePositions() const;
    
private:
    std::shared_ptr<SimpleLogger> logger_;
    std::unordered_map<std::string, Balance> balances_;
    std::unordered_map<std::string, Position> positions_;
    
    double total_realized_pnl_ = 0.0;
    double session_start_value_ = 100000.0; // Default starting portfolio
    double peak_value_ = 100000.0;
    
    mutable std::mutex portfolio_mutex_;
};

} // namespace moneybot
