#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <functional>
#include <chrono>

// Enhanced market data structure for live trading
struct LiveTickData {
    std::string symbol;
    std::string exchange;
    double last_price;
    double bid_price;
    double ask_price;
    double volume_24h;
    double high_24h;
    double low_24h;
    double price_change_24h;
    double price_change_percent_24h;
    int64_t timestamp;
    int64_t server_time;
    
    LiveTickData() : last_price(0), bid_price(0), ask_price(0), volume_24h(0), 
                     high_24h(0), low_24h(0), price_change_24h(0), 
                     price_change_percent_24h(0), timestamp(0), server_time(0) {}
};

// Order structure for live trading
struct LiveOrder {
    std::string order_id;
    std::string client_order_id;
    std::string symbol;
    std::string side;        // "BUY" or "SELL"
    std::string type;        // "LIMIT", "MARKET", "STOP_LOSS", etc.
    std::string status;      // "NEW", "PARTIALLY_FILLED", "FILLED", "CANCELED", etc.
    double quantity;
    double price;
    double filled_quantity;
    double remaining_quantity;
    double commission;
    std::string commission_asset;
    int64_t timestamp;
    int64_t update_time;
    
    LiveOrder() : quantity(0), price(0), filled_quantity(0), 
                  remaining_quantity(0), commission(0), timestamp(0), update_time(0) {}
};

// Account balance structure
struct AccountBalance {
    std::string asset;
    double free;           // Available for trading
    double locked;         // Locked in orders
    double total;          // free + locked
    
    AccountBalance() : free(0), locked(0), total(0) {}
};

// Base exchange interface for live trading
class LiveExchangeInterface {
public:
    virtual ~LiveExchangeInterface() = default;
    
    // Connection management
    virtual bool connect() = 0;
    virtual bool disconnect() = 0;
    virtual bool isConnected() const = 0;
    virtual std::string getExchangeName() const = 0;
    virtual std::string getStatus() const = 0;
    
    // Market data (WebSocket streams)
    virtual bool subscribeToTickers(const std::vector<std::string>& symbols) = 0;
    virtual bool subscribeToOrderBook(const std::string& symbol, int depth = 20) = 0;
    virtual bool subscribeToTrades(const std::string& symbol) = 0;
    virtual LiveTickData getLatestTick(const std::string& symbol) = 0;
    virtual std::vector<std::string> getSupportedSymbols() = 0;
    
    // Trading operations (REST API)
    virtual std::string placeLimitOrder(const std::string& symbol, const std::string& side, 
                                       double quantity, double price) = 0;
    virtual std::string placeMarketOrder(const std::string& symbol, const std::string& side, 
                                        double quantity) = 0;
    virtual bool cancelOrder(const std::string& symbol, const std::string& order_id) = 0;
    virtual bool cancelAllOrders(const std::string& symbol = "") = 0;
    virtual std::vector<LiveOrder> getOpenOrders(const std::string& symbol = "") = 0;
    virtual LiveOrder getOrderStatus(const std::string& symbol, const std::string& order_id) = 0;
    
    // Account information
    virtual std::vector<AccountBalance> getAccountBalances() = 0;
    virtual AccountBalance getAssetBalance(const std::string& asset) = 0;
    virtual double getAvailableBalance(const std::string& asset) = 0;
    
    // Exchange info and limits
    virtual int getRateLimitRemaining() const = 0;
    virtual double getLatencyMs() const = 0;
    virtual std::map<std::string, double> getTradingFees() = 0;
    virtual std::map<std::string, double> getMinOrderSizes() = 0;
    
    // Risk management callbacks
    virtual void setOrderUpdateCallback(std::function<void(const LiveOrder&)> callback) = 0;
    virtual void setTickerUpdateCallback(std::function<void(const LiveTickData&)> callback) = 0;
    virtual void setErrorCallback(std::function<void(const std::string&, const std::string&)> callback) = 0;
    
protected:
    std::string exchange_name;
    bool connected = false;
    double latency_ms = 0.0;
    int rate_limit_remaining = 1000;
    
    // Callback functions
    std::function<void(const LiveOrder&)> order_update_callback;
    std::function<void(const LiveTickData&)> ticker_update_callback;
    std::function<void(const std::string&, const std::string&)> error_callback;
};

// Arbitrage opportunity structure for live trading
struct LiveArbitrageOpportunity {
    std::string symbol;
    std::string buy_exchange;
    std::string sell_exchange;
    double buy_price;
    double sell_price;
    double profit_bps;
    double max_quantity;
    double confidence_score;
    double execution_time_estimate_ms;
    bool is_executable;
    std::string risk_level;  // "LOW", "MEDIUM", "HIGH"
    
    LiveArbitrageOpportunity() : buy_price(0), sell_price(0), profit_bps(0), 
                                max_quantity(0), confidence_score(0), 
                                execution_time_estimate_ms(0), is_executable(false) {}
};
