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
    double spread_bps;           // Spread in basis points
    double order_size;           // Base order size
    double max_position;         // Maximum position size
    double rebalance_threshold;  // Position rebalance threshold
    int refresh_interval_ms;     // Order refresh interval
    double min_spread_bps;       // Minimum spread to place orders
    double max_slippage_bps;     // Maximum slippage tolerance
    bool aggressive_rebalancing; // Whether to use aggressive rebalancing
    
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
    // Market making logic
    void calculateQuotes();
    void placeQuotes();
    void cancelStaleOrders();
    void rebalancePosition();
    
    // Order management
    void placeBidOrder(double price, double quantity);
    void placeAskOrder(double price, double quantity);
    void cancelOrder(const std::string& order_id);
    void cancelAllOrders();
    
    // Position management
    double getCurrentPosition();
    double getTargetPosition();
    void updatePosition(double fill_quantity, OrderSide side);
    
    // Utility functions
    double calculateBidPrice();
    double calculateAskPrice();
    double calculateOrderSize();
    bool shouldPlaceOrders();
    bool isOrderStale(const std::string& order_id);
    
    // Configuration
    void loadConfig(const nlohmann::json& config);
    std::string generateClientOrderId();
    
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
    
    // Performance tracking
    double total_pnl_;
    int total_trades_;
    double avg_spread_;
    int spread_samples_;
};

} // namespace moneybot

#endif // MARKET_MAKER_STRATEGY_H 