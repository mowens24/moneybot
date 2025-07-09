#pragma once
#include <memory>
#include <unordered_map>
#include <vector>
#include <mutex>
#include <chrono>
#include <atomic>
#include "order_book.h"
#include "types.h"
#include "logger.h"
#include <nlohmann/json.hpp>

namespace moneybot {

struct ExchangeConfig {
    std::string name;
    std::string rest_url;
    std::string ws_url;
    std::string api_key;
    std::string secret_key;
    double taker_fee = 0.001;
    double maker_fee = 0.001;
    bool enabled = true;
    int max_connections = 5;
    double latency_threshold_ms = 100.0;
};

struct CrossExchangeOrderBook {
    std::string symbol;
    std::unordered_map<std::string, std::shared_ptr<OrderBook>> exchange_books; // exchange_name -> OrderBook
    double best_bid_price = 0.0;
    double best_ask_price = 0.0;
    std::string best_bid_exchange;
    std::string best_ask_exchange;
    double best_bid_size = 0.0;
    double best_ask_size = 0.0;
    std::chrono::steady_clock::time_point last_update;
    double latency_ms = 0.0;
};

struct ArbitrageOpportunity {
    std::string symbol;
    std::string buy_exchange;
    std::string sell_exchange;
    double buy_price;
    double sell_price;
    double profit_bps;
    double max_size;
    double profit_usd;
    double confidence_score; // 0.0 - 1.0 based on book depth and latency
    std::chrono::steady_clock::time_point timestamp;
    std::chrono::milliseconds time_to_execute{0};
};

struct ExchangeBalance {
    std::string asset;
    double available = 0.0;
    double locked = 0.0;
    double total = 0.0;
    std::chrono::steady_clock::time_point last_update;
};

class MultiExchangeGateway {
public:
    explicit MultiExchangeGateway(const std::vector<ExchangeConfig>& configs, 
                                  std::shared_ptr<Logger> logger = nullptr);
    ~MultiExchangeGateway();

    // Lifecycle management
    void start();
    void stop();
    bool isRunning() const { return running_; }

    // Exchange connectivity
    void connectExchange(const std::string& exchange);
    void disconnectExchange(const std::string& exchange);
    bool isExchangeConnected(const std::string& exchange) const;
    std::vector<std::string> getConnectedExchanges() const;
    std::vector<std::string> getAllExchanges() const;

    // Market data aggregation
    CrossExchangeOrderBook getAggregatedOrderBook(const std::string& symbol) const;
    std::vector<std::string> getAvailableSymbols() const;
    std::unordered_map<std::string, CrossExchangeOrderBook> getAllAggregatedBooks() const;
    
    // Price and tick data
    double getLastPrice(const std::string& exchange, const std::string& symbol) const;
    double getBestPrice(const std::string& symbol, bool is_bid) const;
    
    // Trading operations
    std::string placeOrder(const std::string& exchange, const Order& order);
    bool cancelOrder(const std::string& exchange, const std::string& order_id);
    bool cancelAllOrders(const std::string& exchange, const std::string& symbol = "");
    
    // Account information
    ExchangeBalance getBalance(const std::string& exchange, const std::string& asset) const;
    std::unordered_map<std::string, ExchangeBalance> getAllBalances(const std::string& exchange) const;
    double getTotalBalance(const std::string& asset) const; // Across all exchanges
    
    // Arbitrage opportunities
    std::vector<ArbitrageOpportunity> findArbitrageOpportunities(double min_profit_bps = 10.0) const;
    std::vector<ArbitrageOpportunity> findTriangularArbitrage(const std::string& base_asset = "BTC") const;
    
    // Risk and performance metrics
    double getExchangeLatency(const std::string& exchange) const;
    double getAverageSpread(const std::string& symbol) const;
    nlohmann::json getPerformanceMetrics() const;
    
    // Configuration and status
    void updateExchangeConfig(const std::string& exchange, const ExchangeConfig& config);
    ExchangeConfig getExchangeConfig(const std::string& exchange) const;
    nlohmann::json getStatus() const;

    // Callbacks for real-time updates
    void setOrderBookUpdateCallback(std::function<void(const std::string&, const std::string&, const OrderBook&)> callback);
    void setTradeCallback(std::function<void(const std::string&, const Trade&)> callback);
    void setArbitrageCallback(std::function<void(const ArbitrageOpportunity&)> callback);

private:
    // Configuration and state
    std::vector<ExchangeConfig> configs_;
    std::shared_ptr<Logger> logger_;
    std::atomic<bool> running_{false};
    
    // Exchange connectors (one per exchange)
    std::unordered_map<std::string, std::unique_ptr<class ExchangeConnector>> connectors_;
    
    // Aggregated market data
    mutable std::mutex books_mutex_;
    std::unordered_map<std::string, CrossExchangeOrderBook> aggregated_books_;
    
    // Balance tracking
    mutable std::mutex balances_mutex_;
    std::unordered_map<std::string, std::unordered_map<std::string, ExchangeBalance>> balances_; // exchange -> asset -> balance
    
    // Performance tracking
    mutable std::mutex metrics_mutex_;
    std::unordered_map<std::string, double> exchange_latencies_;
    std::unordered_map<std::string, std::chrono::steady_clock::time_point> last_updates_;
    
    // Callbacks
    std::function<void(const std::string&, const std::string&, const OrderBook&)> orderbook_callback_;
    std::function<void(const std::string&, const Trade&)> trade_callback_;
    std::function<void(const ArbitrageOpportunity&)> arbitrage_callback_;
    
    // Threading
    std::vector<std::thread> worker_threads_;
    
    // Internal methods
    void updateAggregatedBook(const std::string& symbol);
    void onOrderBookUpdate(const std::string& exchange, const std::string& symbol, const OrderBook& book);
    void onTradeUpdate(const std::string& exchange, const Trade& trade);
    void scanArbitrageOpportunities();
    void updateBalances(const std::string& exchange);
    void updateLatency(const std::string& exchange, double latency_ms);
    
    // Arbitrage calculation helpers
    double calculateArbitrageProfit(const std::string& symbol, const std::string& buy_exchange, 
                                   const std::string& sell_exchange) const;
    double calculateMaxArbitrageSize(const ArbitrageOpportunity& opp) const;
    double calculateConfidenceScore(const ArbitrageOpportunity& opp) const;
    
    // Utility methods
    void logInfo(const std::string& message) const;
    void logWarning(const std::string& message) const;
    void logError(const std::string& message) const;
    std::string generateOrderId() const;
};

// Exchange-specific connector interface
class ExchangeConnector {
public:
    virtual ~ExchangeConnector() = default;
    
    virtual void connect() = 0;
    virtual void disconnect() = 0;
    virtual bool isConnected() const = 0;
    
    virtual std::string placeOrder(const Order& order) = 0;
    virtual bool cancelOrder(const std::string& order_id) = 0;
    
    virtual OrderBook getOrderBook(const std::string& symbol) const = 0;
    virtual ExchangeBalance getBalance(const std::string& asset) const = 0;
    virtual std::vector<std::string> getAvailableSymbols() const = 0;
    
    virtual void setOrderBookCallback(std::function<void(const std::string&, const OrderBook&)> callback) = 0;
    virtual void setTradeCallback(std::function<void(const Trade&)> callback) = 0;
    
    virtual double getLatency() const = 0;
    virtual std::string getExchangeName() const = 0;
};

// Factory for creating exchange connectors
std::unique_ptr<ExchangeConnector> createExchangeConnector(const ExchangeConfig& config, 
                                                           std::shared_ptr<Logger> logger = nullptr);

} // namespace moneybot
