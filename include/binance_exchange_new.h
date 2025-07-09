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
    
    // Market data
    LiveTickData getLatestTick(const std::string& symbol) override;
    std::vector<std::string> getAvailableSymbols() const override;
    void setTickCallback(std::function<void(const LiveTickData&)> callback) override {
        tick_callback = callback;
    }
    
    // Order management (placeholder implementations)
    bool placeOrder(const LiveOrder& order) override;
    bool cancelOrder(const std::string& symbol, const std::string& order_id) override;
    std::vector<LiveOrder> getActiveOrders(const std::string& symbol = "") override;
    
    // Account management
    std::map<std::string, double> getBalances() override;
    
private:
    std::function<void(const LiveTickData&)> tick_callback;
};
