#ifndef MOCK_EXCHANGE_H
#define MOCK_EXCHANGE_H

#include "types.h"
#include "logger.h"
#include <nlohmann/json.hpp>
#include <random>
#include <thread>
#include <atomic>
#include <queue>
#include <mutex>
#include <unordered_map>
#include <vector>
#include <string>
#include <memory>
#include <functional>
#include <chrono>

namespace moneybot {
namespace testing {

// Type aliases for callbacks
using MarketDataCallback = std::function<void(const MarketData&)>;
using OrderUpdateCallback = std::function<void(const OrderUpdate&)>;
using TradeUpdateCallback = std::function<void(const TradeUpdate&)>;

class MockExchange {
public:
    MockExchange(const std::string& name, const nlohmann::json& config);
    ~MockExchange();
    
    // Connection management
    void start();
    void stop();
    bool isConnected() const;
    
    // Order management
    std::string placeOrder(const Order& order);
    bool cancelOrder(const std::string& order_id);
    Order getOrder(const std::string& order_id) const;
    std::vector<Order> getOpenOrders() const;
    std::vector<Order> getOrderHistory(const std::string& symbol = "", int limit = 100) const;
    
    // Balance management
    double getBalance(const std::string& asset) const;
    std::unordered_map<std::string, double> getAllBalances() const;
    
    // Market data
    MarketData getMarketData(const std::string& symbol) const;
    std::vector<MarketData> getAllMarketData() const;
    
    // Subscriptions
    void subscribeToMarketData(const std::string& symbol, MarketDataCallback callback);
    void subscribeToOrderUpdates(OrderUpdateCallback callback);
    void subscribeToTradeUpdates(TradeUpdateCallback callback);
    
    // Test utilities
    void setBalance(const std::string& asset, double balance);
    void setMarketData(const std::string& symbol, const MarketData& data);
    void simulateMarketMovement(const std::string& symbol, double price_change_percent);
    void simulateOrderExecution(const std::string& order_id, double fill_price, double fill_quantity);
    
    // Configuration
    void setLatency(std::chrono::milliseconds latency);
    void setFeeRate(double fee_rate);
    void setSlippageRate(double slippage_rate);
    void setErrorRate(double error_rate);
    
    // Statistics
    nlohmann::json getStatistics() const;
    void reset();

private:
    struct MockOrder {
        std::string order_id;
        std::string symbol;
        OrderSide side;
        OrderType type;
        double quantity;
        double price;
        OrderStatus status;
        double filled_quantity;
        double avg_fill_price;
        std::chrono::system_clock::time_point timestamp;
    };
    
    // Configuration
    std::string exchange_name_;
    nlohmann::json config_;
    std::chrono::milliseconds latency_{50};
    double fee_rate_{0.001};
    double slippage_rate_{0.001};
    double error_rate_{0.01};
    
    // State
    std::atomic<bool> is_connected_{false};
    int next_order_id_;
    
    // Data storage
    std::unordered_map<std::string, MockOrder> pending_orders_;
    std::unordered_map<std::string, MockOrder> executed_orders_;
    std::unordered_map<std::string, double> balances_;
    std::unordered_map<std::string, MarketData> market_data_;
    
    // Callbacks
    std::unordered_map<std::string, MarketDataCallback> market_data_callbacks_;
    OrderUpdateCallback order_update_callback_;
    TradeUpdateCallback trade_update_callback_;
    
    // Threading
    std::thread market_thread_;
    std::thread order_thread_;
    
    // Thread safety
    mutable std::mutex state_mutex_;
    mutable std::mutex orders_mutex_;
    mutable std::mutex balances_mutex_;
    mutable std::mutex market_data_mutex_;
    mutable std::mutex callbacks_mutex_;
    
    // Logger
    std::shared_ptr<Logger> logger_;
    
    // Internal methods
    void marketDataLoop();
    void orderProcessingLoop();
    void updateMarketData();
    void processOrders();
    bool validateOrder(const MockOrder& order) const;
    bool shouldExecuteOrder(const MockOrder& order) const;
    double calculateFillPrice(const MockOrder& order) const;
    void updateBalanceAfterTrade(const MockOrder& order, double fill_quantity, double fill_price);
    bool shouldSimulateError() const;
    Order convertToOrder(const MockOrder& mock_order) const;
    void notifyMarketDataUpdate(const std::string& symbol, const MarketData& data);
    void notifyOrderUpdate(const MockOrder& order);
    void notifyTradeUpdate(const MockOrder& order, double fill_quantity, double fill_price);
    std::string getBaseAsset(const std::string& symbol) const;
    std::string getQuoteAsset(const std::string& symbol) const;
};

} // namespace testing
} // namespace moneybot

#endif // MOCK_EXCHANGE_H
