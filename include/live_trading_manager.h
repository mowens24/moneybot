#pragma once

#include "exchange_interface.h"
#include <memory>
#include <map>
#include <vector>
#include <thread>
#include <atomic>
#include <mutex>
#include <queue>

class LiveMarketDataManager {
private:
    std::map<std::string, std::unique_ptr<LiveExchangeInterface>> exchanges;
    std::map<std::pair<std::string, std::string>, LiveTickData> latest_ticks; // {symbol, exchange} -> tick
    std::atomic<bool> running{false};
    std::thread update_thread;
    mutable std::mutex data_mutex;
    
    // Data aggregation and processing
    void processMarketData();
    void updateArbitrageOpportunities();
    std::vector<LiveArbitrageOpportunity> current_opportunities;
    
    // Configuration
    bool demo_mode;
    double min_arbitrage_profit_bps;
    
public:
    LiveMarketDataManager(bool demo = false);
    ~LiveMarketDataManager();
    
    // Exchange management
    bool addExchange(const std::string& name, std::unique_ptr<LiveExchangeInterface> exchange);
    bool removeExchange(const std::string& name);
    LiveExchangeInterface* getExchange(const std::string& name);
    std::vector<std::string> getAvailableExchanges() const;
    
    // Connection management
    bool connectAll();
    bool disconnectAll();
    std::map<std::string, bool> getConnectionStatus() const;
    std::map<std::string, std::string> getExchangeStatus() const;
    
    // Market data access
    LiveTickData getLatestTick(const std::string& symbol, const std::string& exchange);
    std::map<std::string, LiveTickData> getAllTicks(const std::string& symbol);
    std::vector<std::string> getAvailableSymbols() const;
    
    // Arbitrage opportunities
    std::vector<LiveArbitrageOpportunity> getArbitrageOpportunities(double min_profit_bps = 10.0);
    void setMinArbitrageProfitBps(double min_bps) { min_arbitrage_profit_bps = min_bps; }
    
    // Real-time statistics
    struct MarketStats {
        int total_symbols;
        int connected_exchanges;
        double avg_latency_ms;
        int updates_per_second;
        int arbitrage_opportunities;
        std::string last_update_time;
    };
    MarketStats getMarketStats() const;
    
    // Control
    void start();
    void stop();
    bool isRunning() const { return running; }
    void setDemoMode(bool demo) { demo_mode = demo; }
    bool isDemoMode() const { return demo_mode; }
    
    // Callbacks for GUI updates
    void setTickerUpdateCallback(std::function<void(const LiveTickData&)> callback);
    void setArbitrageUpdateCallback(std::function<void(const std::vector<LiveArbitrageOpportunity>&)> callback);
    void setErrorCallback(std::function<void(const std::string&, const std::string&)> callback);
    
private:
    std::function<void(const LiveTickData&)> ticker_callback;
    std::function<void(const std::vector<LiveArbitrageOpportunity>&)> arbitrage_callback;
    std::function<void(const std::string&, const std::string&)> error_callback;
};

// Order execution engine for live trading
class LiveOrderExecutionEngine {
private:
    LiveMarketDataManager* market_data_manager;
    std::map<std::string, std::unique_ptr<LiveExchangeInterface>> exchanges;
    std::atomic<bool> enabled{false};
    
    // Risk management
    double max_position_size_usd;
    double max_daily_loss_usd;
    double max_order_size_usd;
    std::map<std::string, double> position_sizes; // symbol -> size in USD
    
    // Order tracking
    std::map<std::string, LiveOrder> active_orders;
    std::mutex orders_mutex;
    
public:
    LiveOrderExecutionEngine(LiveMarketDataManager* mdm);
    ~LiveOrderExecutionEngine();
    
    // Configuration
    void setRiskLimits(double max_position, double max_daily_loss, double max_order);
    void enableTrading(bool enable) { enabled = enable; }
    bool isTradingEnabled() const { return enabled; }
    
    // Order execution
    struct ExecutionResult {
        bool success;
        std::string order_id;
        std::string error_message;
        double executed_price;
        double executed_quantity;
        double commission;
    };
    
    ExecutionResult executeLimitOrder(const std::string& exchange, const std::string& symbol, 
                                     const std::string& side, double quantity, double price);
    ExecutionResult executeMarketOrder(const std::string& exchange, const std::string& symbol, 
                                      const std::string& side, double quantity);
    
    // Arbitrage execution
    ExecutionResult executeArbitrage(const LiveArbitrageOpportunity& opportunity);
    
    // Order management
    bool cancelOrder(const std::string& exchange, const std::string& symbol, const std::string& order_id);
    bool cancelAllOrders(const std::string& exchange = "");
    std::vector<LiveOrder> getActiveOrders(const std::string& exchange = "");
    
    // Portfolio tracking
    std::map<std::string, double> getCurrentPositions() const;
    double getTotalPortfolioValue() const;
    double getDailyPnL() const;
    
    // Risk checks
    bool checkRiskLimits(const std::string& symbol, double quantity_usd) const;
    bool isWithinDailyLoss() const;
    
    // Callbacks
    void setOrderUpdateCallback(std::function<void(const LiveOrder&)> callback);
    void setExecutionCallback(std::function<void(const ExecutionResult&)> callback);
    
private:
    std::function<void(const LiveOrder&)> order_update_callback;
    std::function<void(const ExecutionResult&)> execution_callback;
    
    bool validateOrder(const std::string& exchange, const std::string& symbol, 
                      const std::string& side, double quantity, double price = 0);
};
