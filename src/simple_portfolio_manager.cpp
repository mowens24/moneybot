#include "../include/simple_portfolio_manager.h"
#include <algorithm>
#include <numeric>

namespace moneybot {

SimplePortfolioManager::SimplePortfolioManager(std::shared_ptr<SimpleLogger> logger)
    : logger_(logger) {
    
    // Initialize with default balances
    updateBalance("USD", 100000.0, 0.0);
    updateBalance("BTC", 0.0, 0.0);
    updateBalance("ETH", 0.0, 0.0);
    
    logger_->info("SimplePortfolioManager initialized with default balances");
}

void SimplePortfolioManager::updateBalance(const std::string& asset, double free, double locked) {
    std::lock_guard<std::mutex> lock(portfolio_mutex_);
    
    Balance& balance = balances_[asset];
    balance.asset = asset;
    balance.free = free;
    balance.locked = locked;
    balance.total = free + locked;
    
    logger_->debug("Balance updated: " + asset + " = " + std::to_string(balance.total));
}

void SimplePortfolioManager::updatePosition(const std::string& symbol, double quantity, double avg_price) {
    std::lock_guard<std::mutex> lock(portfolio_mutex_);
    
    Position& position = positions_[symbol];
    position.symbol = symbol;
    position.quantity = quantity;
    position.avg_price = avg_price;
    
    logger_->debug("Position updated: " + symbol + " = " + std::to_string(quantity));
}

void SimplePortfolioManager::updatePnL(const std::string& symbol, double unrealized_pnl, double realized_pnl) {
    std::lock_guard<std::mutex> lock(portfolio_mutex_);
    
    if (positions_.find(symbol) != positions_.end()) {
        positions_[symbol].unrealized_pnl = unrealized_pnl;
        positions_[symbol].realized_pnl = realized_pnl;
        
        // Update total realized PnL
        total_realized_pnl_ += realized_pnl;
    }
}

std::vector<Balance> SimplePortfolioManager::getBalances() const {
    std::lock_guard<std::mutex> lock(portfolio_mutex_);
    
    std::vector<Balance> result;
    for (const auto& [asset, balance] : balances_) {
        if (balance.total > 0.0) {  // Only return non-zero balances
            result.push_back(balance);
        }
    }
    return result;
}

std::vector<Position> SimplePortfolioManager::getPositions() const {
    std::lock_guard<std::mutex> lock(portfolio_mutex_);
    
    std::vector<Position> result;
    for (const auto& [symbol, position] : positions_) {
        if (std::abs(position.quantity) > 1e-8) {  // Only return non-zero positions
            result.push_back(position);
        }
    }
    return result;
}

double SimplePortfolioManager::getTotalValue() const {
    std::lock_guard<std::mutex> lock(portfolio_mutex_);
    
    double total = 0.0;
    
    // Sum USD balances
    auto usd_it = balances_.find("USD");
    if (usd_it != balances_.end()) {
        total += usd_it->second.total;
    }
    
    // For simplicity, assume other assets have USD value
    // In real implementation, you'd get market prices
    for (const auto& [symbol, position] : positions_) {
        if (std::abs(position.quantity) > 1e-8) {
            // Rough estimate - in reality you'd use real market prices
            total += position.quantity * position.avg_price;
        }
    }
    
    return total;
}

double SimplePortfolioManager::getAvailableCash() const {
    std::lock_guard<std::mutex> lock(portfolio_mutex_);
    
    auto usd_it = balances_.find("USD");
    if (usd_it != balances_.end()) {
        return usd_it->second.free;
    }
    return 0.0;
}

double SimplePortfolioManager::getTotalPnL() const {
    std::lock_guard<std::mutex> lock(portfolio_mutex_);
    
    double total_unrealized = 0.0;
    for (const auto& [symbol, position] : positions_) {
        total_unrealized += position.unrealized_pnl;
    }
    
    return total_realized_pnl_ + total_unrealized;
}

double SimplePortfolioManager::getDailyPnL() const {
    // For simplicity, return total PnL
    // In real implementation, you'd track daily boundaries
    return getTotalPnL();
}

double SimplePortfolioManager::getDrawdown() const {
    double current_value = getTotalValue();
    if (peak_value_ <= 0) return 0.0;
    return (current_value - peak_value_) / peak_value_ * 100.0;
}

double SimplePortfolioManager::getMaxDrawdown() const {
    return getDrawdown(); // Simplified
}

int SimplePortfolioManager::getActivePositions() const {
    std::lock_guard<std::mutex> lock(portfolio_mutex_);
    
    int count = 0;
    for (const auto& [symbol, position] : positions_) {
        if (std::abs(position.quantity) > 1e-8) {
            count++;
        }
    }
    return count;
}

} // namespace moneybot
