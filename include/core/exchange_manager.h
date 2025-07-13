#pragma once

#include <memory>
#include <string>
#include <map>
#include <vector>
#include <chrono>
#include "../simple_logger.h"
#include "../config_manager.h"
#include "../types.h"

namespace moneybot {

struct ExchangeStatus {
    std::string name;
    bool connected;
    double latency_ms;
    std::chrono::steady_clock::time_point last_update;
    int order_count_24h;
    double volume_24h;
};

struct MarketData {
    std::string symbol;
    std::string exchange;
    double bid;
    double ask;
    double last;
    double volume_24h;
    std::chrono::steady_clock::time_point timestamp;
};

class ExchangeManager {
public:
    ExchangeManager(std::shared_ptr<SimpleLogger> logger, ConfigManager& config);
    
    // Connection management
    bool connectToExchanges();
    bool disconnectFromExchanges();
    bool isConnected(const std::string& exchange = "") const;
    
    // Market data
    MarketData getMarketData(const std::string& symbol, const std::string& exchange = "") const;
    std::vector<MarketData> getAllMarketData() const;
    
    // Exchange information
    std::vector<ExchangeStatus> getExchangeStatuses() const;
    std::vector<std::string> getSupportedSymbols() const;
    
    // Real-time updates
    void startMarketDataStream();
    void stopMarketDataStream();
    
private:
    std::shared_ptr<SimpleLogger> logger_;
    ConfigManager& config_;
    std::map<std::string, ExchangeStatus> exchanges_;
    std::map<std::string, MarketData> market_data_; // symbol -> latest data
    bool streaming_ = false;
    
    void simulateMarketData();
    void updateMarketData(const std::string& symbol, const std::string& exchange);
};

} // namespace moneybot
