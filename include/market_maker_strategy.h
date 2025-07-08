#ifndef MARKET_MAKER_STRATEGY_H
#define MARKET_MAKER_STRATEGY_H

#include "strategy.h"
#include "order_book.h"
#include "order_manager.h"
#include "risk_manager.h"
#include "types.h"
#include <memory>
#include <unordered_map>
#include <chrono>
#include <nlohmann/json.hpp>

namespace moneybot {

struct MarketMakerConfig {
    double base_spread_bps = 5.0;          // Base spread in basis points
    double max_spread_bps = 50.0;          // Maximum spread in basis points
    double min_spread_bps = 1.0;           // Minimum spread in basis points
    double order_size = 0.001;             // Base order size
    double max_position = 0.01;            // Maximum position size
    double inventory_skew_factor = 2.0;    // How much to skew based on inventory
    int volatility_window = 100;           // Ticks to calculate volatility
    double volatility_multiplier = 1.5;    // Spread adjustment based on volatility
    int refresh_interval_ms = 1000;        // Order refresh interval
    double min_profit_bps = 0.5;           // Minimum profit target per trade
    double rebalance_threshold = 0.5;      // Position rebalance threshold
    double max_slippage_bps = 10.0;        // Maximum slippage tolerance
    bool aggressive_rebalancing = false;   // Whether to use aggressive rebalancing
    
    MarketMakerConfig() = default;
    MarketMakerConfig(const nlohmann::json& j);
};

class MarketMakerStrategy : public Strategy {
public:
    MarketMakerStrategy(std::shared_ptr<Logger> logger,
                       std::shared_ptr<OrderManager> order_manager,
                       std::shared_ptr<RiskManager> risk_manager,
                       const nlohmann::json& config);
    ~MarketMakerStrategy();
    
    // Strategy interface implementation
    void onOrderBookUpdate(const OrderBook& order_book) override;
    void onTrade(const Trade& trade) override;
    void onOrderAck(const OrderAck& ack) override;
    void onOrderReject(const OrderReject& reject) override;
    void onOrderFill(const OrderFill& fill) override;
    
    void initialize() override;
    void shutdown() override;
    
    std::string getName() const override { return "MarketMaker"; }
    void updateConfig(const nlohmann::json& config) override;

private:
    // Enhanced market making logic
    void calculateQuotes();
    void placeQuotes();
    void cancelStaleOrders();
    void rebalancePosition();
    
    // Advanced spread calculation
    double calculateOptimalSpread() const;
    double calculateInventorySkew() const;
    double calculateVolatility() const;
    std::pair<double, double> calculateOrderSizes() const;
    
    // Order management
    void placeBidOrder(double price, double quantity);
    void placeAskOrder(double price, double quantity);
    void cancelOrder(const std::string& order_id);
    void cancelAllOrders();
    
    // Position management
    double getCurrentPosition();
    double getTargetPosition();
    void updatePosition(double fill_quantity, OrderSide side);
    
    // Risk management
    bool isWithinRiskLimits(double price, double size, bool is_buy) const;
    void updateMetrics();
    void logStrategyState() const;
    
    // Utility functions
    double calculateBidPrice();
    double calculateAskPrice();
    double calculateOrderSize();
    bool shouldPlaceOrders();
    bool shouldRefreshOrders() const;
    bool isOrderStale(const std::string& order_id);
    
    // Configuration
    void loadConfig(const nlohmann::json& config);
    std::string generateClientOrderId();
    
    // Helper methods
    template<typename... Args>
    void safeLog(spdlog::level::level_enum level, const std::string& format, Args&&... args) const {
        if (logger_ && logger_->getLogger()) {
            logger_->getLogger()->log(level, format, std::forward<Args>(args)...);
        }
    }
    
    std::shared_ptr<Logger> logger_;
    std::shared_ptr<OrderManager> order_manager_;
    std::shared_ptr<RiskManager> risk_manager_;
    MarketMakerConfig config_;
    
    // State tracking
    std::string symbol_;
    double current_position_;
    double current_bid_price_;
    double current_ask_price_;
    double mid_price_;
    
    // Enhanced market data tracking
    std::deque<double> recent_prices_;
    std::deque<double> recent_spreads_;
    double current_volatility_;
    double current_spread_bps_;
    
    // Order tracking
    struct ActiveOrder {
        std::string order_id;
        OrderSide side;
        double price;
        double quantity;
        std::chrono::system_clock::time_point timestamp;
    };
    std::unordered_map<std::string, ActiveOrder> active_orders_;
    
    // Thread safety
    mutable std::mutex state_mutex_;
    mutable std::mutex orders_mutex_;
    
    // Timing
    std::chrono::system_clock::time_point last_quote_time_;
    std::chrono::system_clock::time_point last_rebalance_time_;
    
    // Enhanced performance tracking
    double total_pnl_;
    double realized_pnl_;
    double unrealized_pnl_;
    int total_trades_;
    double avg_spread_;
    int spread_samples_;
    bool strategy_active_;
};

} // namespace moneybot

#endif // MARKET_MAKER_STRATEGY_H 