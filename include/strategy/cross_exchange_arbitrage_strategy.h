#ifndef CROSS_EXCHANGE_ARBITRAGE_STRATEGY_H
#define CROSS_EXCHANGE_ARBITRAGE_STRATEGY_H

#include "strategy/strategy_engine.h"
#include "core/exchange_manager.h"
#include "risk_manager.h"
#include "logger.h"
#include <unordered_map>
#include <vector>
#include <chrono>

namespace moneybot {
namespace strategy {

struct ExchangePrice {
    std::string exchange;
    double bid;
    double ask;
    double quantity;
    std::chrono::system_clock::time_point timestamp;
};

struct ArbitrageOpportunity {
    std::string symbol;
    std::string buy_exchange;
    std::string sell_exchange;
    double buy_price;
    double sell_price;
    double quantity;
    double profit;
    double profit_percentage;
    std::chrono::system_clock::time_point timestamp;
};

class CrossExchangeArbitrageStrategy : public BaseStrategy {
public:
    CrossExchangeArbitrageStrategy(
        const std::string& name,
        const nlohmann::json& config,
        std::shared_ptr<Logger> logger,
        std::shared_ptr<RiskManager> risk_manager,
        std::shared_ptr<ExchangeManager> exchange_manager
    );
    
    bool initialize() override;
    void start() override;
    void stop() override;
    void pause() override;
    void resume() override;
    
    // These methods would be called by the strategy engine
    void onTick(const MarketTick& tick);
    void onOrderUpdate(const OrderUpdate& update);
    void onTradeUpdate(const TradeUpdate& update);
    
    // BaseStrategy interface implementation
    void onTick(const std::string& symbol, const Trade& tick) override;
    void onOrderFill(const OrderFill& fill) override;
    void onOrderReject(const OrderReject& reject) override;
    void onBalanceUpdate(const Balance& balance) override;
    void onPositionUpdate(const Position& position) override;
    
    nlohmann::json getMetrics() const;
    nlohmann::json getConfig() const;
    void updateConfig(const nlohmann::json& config);

private:
    // Configuration
    double min_profit_percentage_;
    double max_position_size_;
    double max_order_size_;
    int max_opportunities_per_minute_;
    std::chrono::milliseconds max_latency_;
    std::vector<std::string> monitored_exchanges_;
    std::vector<std::string> monitored_symbols_;
    
    // State
    std::unordered_map<std::string, std::unordered_map<std::string, ExchangePrice>> prices_; // symbol -> exchange -> price
    std::vector<ArbitrageOpportunity> opportunities_;
    std::vector<ArbitrageOpportunity> executed_opportunities_;
    
    // Performance tracking
    struct PerformanceMetrics {
        int opportunities_detected = 0;
        int opportunities_executed = 0;
        double total_profit = 0.0;
        double total_fees = 0.0;
        double net_profit = 0.0;
        double win_rate = 0.0;
        double avg_profit_per_trade = 0.0;
        double max_profit = 0.0;
        double max_loss = 0.0;
        int winning_trades = 0;
        int losing_trades = 0;
        std::chrono::system_clock::time_point last_opportunity;
        std::chrono::system_clock::time_point last_execution;
    };
    PerformanceMetrics metrics_;
    
    // Rate limiting
    std::chrono::system_clock::time_point last_execution_time_;
    int executions_this_minute_;
    std::chrono::system_clock::time_point minute_start_;
    
    // Thread safety
    mutable std::mutex prices_mutex_;
    mutable std::mutex opportunities_mutex_;
    mutable std::mutex metrics_mutex_;
    
    // Core methods
    void updatePrice(const std::string& symbol, const std::string& exchange, const MarketTick& tick);
    void detectArbitrageOpportunities(const std::string& symbol);
    bool validateOpportunity(const ArbitrageOpportunity& opportunity) const;
    void executeArbitrageOpportunity(const ArbitrageOpportunity& opportunity);
    bool checkRateLimits() const;
    double calculateFees(const std::string& exchange, double quantity, double price) const;
    bool isPriceStale(const ExchangePrice& price) const;
    void cleanupStaleData();
    void updateMetrics(const ArbitrageOpportunity& opportunity, double actual_profit);
    
    // Helper methods
    std::string getExchangeFromTick(const MarketTick& tick) const;
    bool isExchangeMonitored(const std::string& exchange) const;
    bool isSymbolMonitored(const std::string& symbol) const;
    double getMinQuantity(const std::string& symbol, const std::string& exchange) const;
    double getMaxQuantity(const std::string& symbol, const std::string& exchange) const;
    
    // Dependencies
    std::shared_ptr<ExchangeManager> exchange_manager_;
};

} // namespace strategy
} // namespace moneybot

#endif // CROSS_EXCHANGE_ARBITRAGE_STRATEGY_H
