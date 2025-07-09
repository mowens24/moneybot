#pragma once

#include "exchange_interface.h"
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <thread>
#include <atomic>
#include <queue>

class BinanceExchange : public LiveExchangeInterface {
private:
    // API configuration
    std::string api_key;
    std::string secret_key;
    std::string base_url;
    bool use_testnet;
    
    // HTTP client
    CURL* curl_handle;
    
    // Polling for market data (simplified approach)
    std::thread polling_thread;
    std::atomic<bool> polling_active{false};
    
    // Data storage
    std::map<std::string, LiveTickData> tickers;
    std::map<std::string, std::vector<LiveOrder>> order_book_cache;
    std::mutex data_mutex;
    
    // Rate limiting
    std::atomic<int> requests_per_minute{0};
    std::chrono::steady_clock::time_point last_rate_limit_reset;
    
    // HTTP request helpers
    struct HTTPResponse {
        std::string data;
        long response_code;
        std::string error;
    };
    
    HTTPResponse makeRequest(const std::string& endpoint, const std::string& params = "", 
                           const std::string& method = "GET", bool sign = false);
    std::string signRequest(const std::string& params);
    std::string generateSignature(const std::string& query_string);
    
    // Market data polling
    void pollMarketData();
    
public:
    BinanceExchange(const std::string& api_key = "", const std::string& secret_key = "", bool testnet = false);
    ~BinanceExchange();
    
    // Connection management
    bool connect() override;
    bool disconnect() override;
    bool isConnected() const override;
    std::string getStatus() const override;
    std::string getExchangeName() const override;
    
    // Market data subscriptions
    bool subscribeToTickers(const std::vector<std::string>& symbols) override;
    bool subscribeToOrderBook(const std::string& symbol, int depth = 20) override;
    bool subscribeToTrades(const std::string& symbol) override;
    std::vector<std::string> getSupportedSymbols() override;
    
    // Market data access
    LiveTickData getLatestTick(const std::string& symbol) override;
    std::vector<std::string> getAvailableSymbols() const;
    void setTickCallback(std::function<void(const LiveTickData&)> callback) {
        tick_callback = callback;
    }
    
    // Order management
    std::string placeLimitOrder(const std::string& symbol, const std::string& side, 
                               double quantity, double price) override;
    std::string placeMarketOrder(const std::string& symbol, const std::string& side, 
                                double quantity) override;
    bool placeOrder(const LiveOrder& order);  // Additional helper method
    bool cancelOrder(const std::string& symbol, const std::string& order_id) override;
    bool cancelAllOrders(const std::string& symbol = "") override;
    std::vector<LiveOrder> getActiveOrders(const std::string& symbol = "");  // Additional helper method
    std::vector<LiveOrder> getOpenOrders(const std::string& symbol = "") override;
    LiveOrder getOrderStatus(const std::string& symbol, const std::string& order_id) override;
    
    // Account management
    std::map<std::string, double> getBalances();  // Additional helper method
    std::vector<AccountBalance> getAccountBalances() override;
    AccountBalance getAssetBalance(const std::string& asset) override;
    double getAvailableBalance(const std::string& asset) override;
    
    // Performance metrics
    int getRateLimitRemaining() const override;
    double getLatencyMs() const override;
    std::map<std::string, double> getTradingFees() override;
    std::map<std::string, double> getMinOrderSizes() override;
    
    // Callbacks
    void setOrderUpdateCallback(std::function<void(const LiveOrder&)> callback) override;
    void setTickerUpdateCallback(std::function<void(const LiveTickData&)> callback) override;
    void setErrorCallback(std::function<void(const std::string&, const std::string&)> callback) override;
    
private:
    std::function<void(const LiveTickData&)> tick_callback;
    std::function<void(const LiveOrder&)> order_update_callback;
    std::function<void(const std::string&, const std::string&)> error_callback;
};
