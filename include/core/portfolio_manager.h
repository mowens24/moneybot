#ifndef PORTFOLIO_MANAGER_H
#define PORTFOLIO_MANAGER_H

#include "types.h"
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <chrono>
#include <functional>
#include <nlohmann/json.hpp>

namespace moneybot {

class Logger;

struct PortfolioSnapshot {
    double total_value;
    double unrealized_pnl;
    double realized_pnl;
    double day_pnl;
    double total_pnl;
    std::chrono::system_clock::time_point timestamp;
    std::unordered_map<std::string, Balance> balances;
    std::unordered_map<std::string, Position> positions;
};

struct PortfolioMetrics {
    double total_return;
    double daily_return;
    double max_drawdown;
    double sharpe_ratio;
    double win_rate;
    double avg_win;
    double avg_loss;
    double profit_factor;
    int total_trades;
    int winning_trades;
    int losing_trades;
};

class PortfolioManager {
public:
    using PortfolioUpdateCallback = std::function<void(const PortfolioSnapshot&)>;
    using BalanceUpdateCallback = std::function<void(const Balance&)>;
    using PositionUpdateCallback = std::function<void(const Position&)>;

    PortfolioManager(std::shared_ptr<Logger> logger, const nlohmann::json& config);
    ~PortfolioManager() = default;

    // Balance management
    void updateBalance(const Balance& balance);
    void updateBalances(const std::vector<Balance>& balances);
    Balance getBalance(const std::string& asset) const;
    std::vector<Balance> getAllBalances() const;
    double getTotalValue() const;
    double getAvailableBalance(const std::string& asset) const;
    
    // Position management
    void updatePosition(const Position& position);
    void updatePositions(const std::vector<Position>& positions);
    Position getPosition(const std::string& symbol) const;
    std::vector<Position> getAllPositions() const;
    std::vector<Position> getActivePositions() const;
    
    // Trade tracking
    void recordTrade(const Trade& trade);
    void recordOrderFill(const OrderFill& fill);
    std::vector<Trade> getTradeHistory(const std::string& symbol = "") const;
    
    // PnL calculations
    double getUnrealizedPnL() const;
    double getRealizedPnL() const;
    double getDayPnL() const;
    double getTotalPnL() const;
    void resetDayPnL();
    
    // Portfolio analysis
    PortfolioMetrics calculateMetrics() const;
    PortfolioSnapshot getSnapshot() const;
    std::vector<PortfolioSnapshot> getHistorySnapshots(int count = 100) const;
    
    // Risk metrics
    double calculateVaR(double confidence = 0.95) const;
    double calculateMaxDrawdown() const;
    double calculateSharpeRatio() const;
    
    // Callbacks
    void setPortfolioUpdateCallback(PortfolioUpdateCallback callback);
    void setBalanceUpdateCallback(BalanceUpdateCallback callback);
    void setPositionUpdateCallback(PositionUpdateCallback callback);
    
    // Persistence
    void saveSnapshot(const std::string& filename = "") const;
    void loadSnapshot(const std::string& filename);
    
    // Reporting
    nlohmann::json getPortfolioReport() const;
    nlohmann::json getPerformanceReport() const;
    
    // Utilities
    void clear();
    void setBaseCurrency(const std::string& currency);
    std::string getBaseCurrency() const;

private:
    struct TradeRecord {
        Trade trade;
        double pnl;
        std::chrono::system_clock::time_point timestamp;
    };

    // Internal calculations
    double calculatePositionValue(const Position& position) const;
    double calculateUnrealizedPnLForPosition(const Position& position) const;
    void updatePortfolioMetrics();
    void notifyPortfolioUpdate();
    
    // Price handling (simplified - in real system would integrate with market data)
    double getPrice(const std::string& symbol) const;
    void updatePrice(const std::string& symbol, double price);
    
    std::shared_ptr<Logger> logger_;
    
    // Portfolio state
    std::unordered_map<std::string, Balance> balances_;
    std::unordered_map<std::string, Position> positions_;
    std::vector<TradeRecord> trade_history_;
    std::vector<PortfolioSnapshot> history_snapshots_;
    
    // PnL tracking
    double session_start_value_;
    double day_start_value_;
    double total_realized_pnl_;
    std::chrono::system_clock::time_point day_start_time_;
    std::chrono::system_clock::time_point session_start_time_;
    
    // Market data cache (simplified)
    std::unordered_map<std::string, double> price_cache_;
    std::unordered_map<std::string, std::chrono::system_clock::time_point> price_timestamps_;
    
    // Configuration
    std::string base_currency_;
    bool auto_save_snapshots_;
    int max_history_snapshots_;
    
    // Callbacks
    PortfolioUpdateCallback portfolio_callback_;
    BalanceUpdateCallback balance_callback_;
    PositionUpdateCallback position_callback_;
    
    // Thread safety
    mutable std::mutex balances_mutex_;
    mutable std::mutex positions_mutex_;
    mutable std::mutex trades_mutex_;
    mutable std::mutex prices_mutex_;
    mutable std::mutex snapshots_mutex_;
    mutable std::mutex callbacks_mutex_;
};

} // namespace moneybot

#endif // PORTFOLIO_MANAGER_H
