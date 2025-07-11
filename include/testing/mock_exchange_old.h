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
    double getLatencyMs() const override;
    std::map<std::string, double> getTradingFees() override;
    std::map<std::string, double> getMinOrderSizes() override;
    
    // Risk management callbacks
    void setOrderUpdateCallback(std::function<void(const LiveOrder&)> callback) override;
    void setTickerUpdateCallback(std::function<void(const LiveTickData&)> callback) override;
    void setErrorCallback(std::function<void(const std::string&, const std::string&)> callback) override;
    
    // Mock-specific methods
    void setBalance(const std::string& asset, double amount);
    void setPrice(const std::string& symbol, double price);
    void enablePriceSimulation(bool enable);
    void setLatencySimulation(double min_ms, double max_ms);
    void setOrderFillProbability(double probability);
    void triggerOrderFill(const std::string& order_id);
    void triggerOrderReject(const std::string& order_id, const std::string& reason);
    
    // Test utilities
    void reset();
    int getTotalOrdersPlaced() const;
    int getTotalOrdersFilled() const;
    int getTotalOrdersCancelled() const;
    
private:
    struct MockOrder {
        std::string order_id;
        std::string client_order_id;
        std::string symbol;
        std::string side;
        std::string type;
        std::string status;
        double quantity;
        double price;
        double filled_quantity;
        double remaining_quantity;
        std::chrono::system_clock::time_point created_time;
        std::chrono::system_clock::time_point update_time;
        
        MockOrder(const std::string& id, const std::string& sym, const std::string& s, 
                 const std::string& t, double q, double p)
            : order_id(id), symbol(sym), side(s), type(t), status("NEW"), 
              quantity(q), price(p), filled_quantity(0), remaining_quantity(q) {
            created_time = update_time = std::chrono::system_clock::now();
        }
    };
    
    // Market data simulation
    void simulateMarketData();
    void generateRandomPrice(const std::string& symbol);
    void publishTick(const std::string& symbol, double price);
    
    // Order processing
    std::string generateOrderId();
    void processOrder(const std::string& order_id);
    void fillOrder(const std::string& order_id, double fill_price, double fill_quantity);
    
    // State
    std::string name_;
    std::atomic<bool> connected_;
    std::atomic<bool> simulating_;
    
    // Market data
    std::unordered_map<std::string, double> prices_;
    std::unordered_map<std::string, LiveTickData> ticks_;
    std::vector<std::string> subscribed_symbols_;
    std::vector<std::string> supported_symbols_;
    
    // Account data
    std::unordered_map<std::string, AccountBalance> balances_;
    
    // Orders
    std::unordered_map<std::string, MockOrder> orders_;
    std::queue<std::string> order_processing_queue_;
    
    // Configuration
    double min_latency_ms_;
    double max_latency_ms_;
    double order_fill_probability_;
    bool enable_price_simulation_;
    
    // Statistics
    std::atomic<int> total_orders_placed_;
    std::atomic<int> total_orders_filled_;
    std::atomic<int> total_orders_cancelled_;
    std::atomic<int> next_order_id_;
    
    // Threading
    std::thread market_data_thread_;
    std::thread order_processing_thread_;
    
    // Thread safety
    mutable std::mutex prices_mutex_;
    mutable std::mutex orders_mutex_;
    mutable std::mutex balances_mutex_;
    mutable std::mutex subscriptions_mutex_;
    mutable std::mutex queue_mutex_;
    
    // Random number generation
    std::random_device rd_;
    std::mt19937 gen_;
    std::uniform_real_distribution<> price_change_dist_;
    std::uniform_real_distribution<> latency_dist_;
    std::uniform_real_distribution<> fill_probability_dist_;
};

} // namespace moneybot

#endif // MOCK_EXCHANGE_H
