#pragma once
#include "strategy.h"
#include "multi_exchange_gateway.h"
#include <deque>
#include <memory>
#include <vector>
#include <unordered_map>
#include <chrono>
#include <mutex>

namespace moneybot {

struct CryptoPair {
    std::string base_symbol;        // e.g., "BTCUSDT"
    std::string hedge_symbol;       // e.g., "ETHUSDT"
    double hedge_ratio = 1.0;       // How much hedge per base
    double z_score = 0.0;           // Current z-score
    double correlation = 0.0;       // Current correlation coefficient
    bool in_position = false;
    double entry_z_score = 0.0;
    double entry_price_base = 0.0;
    double entry_price_hedge = 0.0;
    double current_pnl = 0.0;
    double position_size = 0.0;     // Current position size
    std::chrono::steady_clock::time_point last_rebalance;
    std::chrono::steady_clock::time_point position_entry_time;
    
    // Statistical metrics
    double mean_spread = 0.0;
    double std_spread = 0.0;
    double half_life = 0.0;         // Mean reversion half-life in minutes
};

struct StatArbConfig {
    std::vector<CryptoPair> pairs;
    double entry_threshold = 2.0;      // Z-score threshold to enter
    double exit_threshold = 0.5;       // Z-score threshold to exit
    double stop_loss_threshold = 4.0;  // Emergency exit z-score
    int lookback_periods = 200;        // Periods for cointegration
    double max_position_size = 0.01;   // Max position per pair (BTC)
    int rebalance_frequency_ms = 5000; // How often to check positions
    double correlation_threshold = 0.7; // Minimum correlation to trade
    double min_half_life_minutes = 30.0; // Minimum mean reversion speed
    double max_half_life_minutes = 1440.0; // Maximum mean reversion speed (24h)
    double transaction_cost_bps = 10.0;  // Estimated transaction costs
    bool use_kalman_filter = true;      // Use Kalman filter for hedge ratio
    double volatility_threshold = 0.05; // Don't trade in high volatility
};

class StatisticalArbitrageStrategy : public Strategy {
public:
    explicit StatisticalArbitrageStrategy(const StatArbConfig& config, 
                                        std::shared_ptr<MultiExchangeGateway> gateway,
                                        std::shared_ptr<Logger> logger = nullptr);
    ~StatisticalArbitrageStrategy() override = default;

    // Strategy interface
    void onOrderBookUpdate(const OrderBook& order_book) override;
    void onTrade(const Trade& trade) override;
    void onOrderAck(const OrderAck& ack) override;
    void onOrderReject(const OrderReject& reject) override;
    void onOrderFill(const OrderFill& fill) override;
    void initialize() override;
    void shutdown() override;
    std::string getName() const override { return "StatisticalArbitrage"; }
    void updateConfig(const nlohmann::json& config) override;

    // Statistical analysis methods
    void updatePairStatistics();
    double calculateZScore(const CryptoPair& pair) const;
    double calculateCorrelation(const std::string& symbol1, const std::string& symbol2) const;
    double calculateCointegrationRatio(const std::string& symbol1, const std::string& symbol2) const;
    double calculateHalfLife(const CryptoPair& pair) const;
    bool isPairCointegrated(const CryptoPair& pair) const;
    
    // Position management
    void checkEntrySignals();
    void checkExitSignals();
    void executePairTrade(CryptoPair& pair, bool long_base_short_hedge);
    void closePairPosition(CryptoPair& pair);
    void updatePositionPnL(CryptoPair& pair);
    
    // Risk management
    bool isRiskAcceptable(const CryptoPair& pair, double position_size) const;
    double calculateOptimalPositionSize(const CryptoPair& pair) const;
    double calculateStopLossLevel(const CryptoPair& pair) const;
    
    // Performance metrics
    nlohmann::json getPerformanceMetrics() const;
    std::vector<CryptoPair> getActivePairs() const;
    double getTotalPnL() const { return total_pnl_; }
    double getSharpeRatio() const;
    double getMaxDrawdown() const;

private:
    StatArbConfig config_;
    std::shared_ptr<MultiExchangeGateway> gateway_;
    std::shared_ptr<Logger> logger_;
    
    // Price history for statistical calculations (symbol -> price history)
    std::unordered_map<std::string, std::deque<double>> price_history_;
    std::unordered_map<std::string, std::deque<double>> return_history_;
    std::unordered_map<std::string, std::deque<std::chrono::steady_clock::time_point>> time_history_;
    
    // Spread history for each pair
    std::unordered_map<std::string, std::deque<double>> spread_history_; // pair_id -> spread history
    std::unordered_map<std::string, std::deque<double>> zscore_history_; // pair_id -> z-score history
    
    // Active positions
    std::vector<CryptoPair> active_pairs_;
    std::unordered_map<std::string, double> positions_; // symbol -> position size
    
    // Order tracking
    struct PendingOrder {
        std::string order_id;
        std::string pair_id;
        std::string symbol;
        bool is_entry; // true for entry, false for exit
        double expected_price;
        std::chrono::steady_clock::time_point order_time;
    };
    std::unordered_map<std::string, PendingOrder> pending_orders_;
    
    // Performance tracking
    double total_pnl_ = 0.0;
    double realized_pnl_ = 0.0;
    double unrealized_pnl_ = 0.0;
    int trades_executed_ = 0;
    int successful_trades_ = 0;
    std::chrono::steady_clock::time_point last_rebalance_;
    std::chrono::steady_clock::time_point strategy_start_time_;
    
    // Risk metrics
    double max_drawdown_ = 0.0;
    double peak_pnl_ = 0.0;
    std::deque<double> daily_returns_;
    
    // Threading
    mutable std::mutex strategy_mutex_;
    
    // Helper methods
    void updatePriceHistory(const std::string& symbol, double price);
    void updateSpreadHistory(const std::string& pair_id, double spread);
    double getLatestPrice(const std::string& symbol) const;
    double calculateSpread(const CryptoPair& pair) const;
    std::string getPairId(const CryptoPair& pair) const;
    double getMeanSpread(const CryptoPair& pair) const;
    double getSpreadStd(const CryptoPair& pair) const;
    
    // Order management
    std::string generateOrderId() const;
    void onOrderCompleted(const std::string& order_id, bool success);
    void cleanupStaleOrders();
    
    // Statistical calculations
    double calculateMean(const std::deque<double>& data) const;
    double calculateStdDev(const std::deque<double>& data) const;
    double calculateCorrelation(const std::deque<double>& x, const std::deque<double>& y) const;
    
    // Kalman filter for dynamic hedge ratio estimation
    struct KalmanState {
        double x = 1.0;     // hedge ratio estimate
        double P = 1.0;     // error covariance
        double Q = 0.01;    // process noise
        double R = 0.1;     // measurement noise
    };
    std::unordered_map<std::string, KalmanState> kalman_states_;
    double updateKalmanFilter(const std::string& pair_id, double observed_ratio);
    
    // Logging helpers
    void logInfo(const std::string& message) const;
    void logWarning(const std::string& message) const;
    void logError(const std::string& message) const;
    void logPairStatus(const CryptoPair& pair) const;
    void logTradeExecution(const CryptoPair& pair, bool is_entry, bool long_base) const;
};

} // namespace moneybot
