#pragma once

#include "exchange_interface.h"
#include <memory>
#include <map>
#include <vector>
#include <thread>
#include <atomic>
#include <mutex>
#include <queue>
#include <chrono>

// ========================================================================
// 🎨 ALGORITHM VISUALIZATION DATA STRUCTURES
// ========================================================================

struct TriangleArbOpportunity {
    std::string symbol_a, symbol_b, symbol_c;  // e.g., BTC, ETH, USDT
    std::string exchange_1, exchange_2, exchange_3;
    double profit_bps = 0.0;
    double volume_usd = 0.0;
    double execution_probability = 0.0;
    bool is_active = false;
    std::chrono::steady_clock::time_point last_update;
    
    // Visual properties for animation
    float visual_intensity = 0.0f;  // 0.0 to 1.0 for line thickness/color
    float animation_phase = 0.5f;   // For pulsing/flowing animations
};

struct ExchangeFlowData {
    std::string from_exchange, to_exchange;
    double flow_volume_24h = 0.0;
    double avg_profit_bps = 0.0;
    int opportunities_count = 0;
    bool is_flowing = false;
    
    // Visual animation properties
    float flow_particles = 0.0f;    // Number of animated particles
    float flow_speed = 1.0f;        // Speed of flow animation
    // Note: Using struct Color as placeholder, convert to ImVec4 in implementation
    struct Color { float r, g, b, a; 
        Color() : r(0.0f), g(1.0f), b(0.0f), a(1.0f) {}
        Color(float _r, float _g, float _b, float _a) : r(_r), g(_g), b(_b), a(_a) {}
    } flow_color;
};

struct AlgorithmPerformance {
    std::string algo_name;
    double daily_pnl = 0.0;
    double total_pnl = 0.0;
    double win_rate = 0.0;           // 0.0 to 1.0
    double sharpe_ratio = 0.0;
    int trades_executed = 0;
    bool is_active = true;
    
    // Visual gauge properties
    float gauge_value = 0.0f;        // Current performance gauge (0.0 to 1.0)
    float target_gauge = 0.0f;       // Target for smooth animation
    struct Color { float r, g, b, a; 
        Color() : r(0.0f), g(1.0f), b(0.0f), a(1.0f) {}
        Color(float _r, float _g, float _b, float _a) : r(_r), g(_g), b(_b), a(_a) {}
    } performance_color;
};

// ========================================================================

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
    
    // Algorithm visualization data access
    std::vector<TriangleArbOpportunity> getActiveTriangleOpportunities() const;
    std::vector<ExchangeFlowData> getExchangeFlowData() const;
    std::vector<AlgorithmPerformance> getAlgorithmPerformance() const;
    double getTotalDailyPnL() const;
    int getTotalActiveOpportunities() const;
    
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
