#pragma once

#include <memory>
#include <thread>
#include <atomic>
#include <random>
#include <chrono>
#include <functional>
#include "types.h"
#include "order_book.h"

namespace moneybot {

struct MarketDataTick {
    std::string symbol;
    std::string exchange;
    double bid_price;
    double ask_price;
    double bid_size;
    double ask_size;
    double last_price;
    double volume_24h;
    std::chrono::steady_clock::time_point timestamp;
};

class MarketDataSimulator {
public:
    explicit MarketDataSimulator();
    ~MarketDataSimulator();
    
    // Start/stop simulation
    void start();
    void stop();
    bool isRunning() const { return running_; }
    
    // Configuration
    void setUpdateInterval(std::chrono::milliseconds interval) { update_interval_ = interval; }
    void addSymbol(const std::string& symbol, double base_price = 50000.0);
    void addExchange(const std::string& exchange_name);
    
    // Callbacks
    void setTickCallback(std::function<void(const MarketDataTick&)> callback) {
        tick_callback_ = callback;
    }
    
    void setOrderBookCallback(std::function<void(const std::string&, const std::string&, std::shared_ptr<OrderBook>)> callback) {
        orderbook_callback_ = callback;
    }
    
    // Market conditions
    void setVolatility(double volatility) { volatility_ = volatility; }
    void setTrendDirection(double trend) { trend_ = trend; } // -1.0 to 1.0
    void simulateNewsEvent(double impact_percentage, std::chrono::seconds duration);
    
    // Get current market data
    MarketDataTick getLatestTick(const std::string& symbol, const std::string& exchange) const;
    std::shared_ptr<OrderBook> getOrderBook(const std::string& symbol, const std::string& exchange) const;
    
private:
    void simulationLoop();
    void updatePrices();
    void updateOrderBooks();
    void generateTick(const std::string& symbol, const std::string& exchange);
    std::shared_ptr<OrderBook> generateOrderBook(const std::string& symbol, const std::string& exchange, double mid_price);
    
    double generatePrice(const std::string& symbol, double current_price);
    double applyNewsImpact(double price, const std::string& symbol);
    
    std::atomic<bool> running_;
    std::thread simulation_thread_;
    std::chrono::milliseconds update_interval_;
    
    // Market data storage
    std::map<std::string, std::map<std::string, double>> symbol_prices_; // symbol -> exchange -> price
    std::map<std::string, std::map<std::string, MarketDataTick>> latest_ticks_; // symbol -> exchange -> tick
    std::map<std::string, std::map<std::string, std::shared_ptr<OrderBook>>> order_books_; // symbol -> exchange -> book
    
    // Simulation parameters
    std::vector<std::string> symbols_;
    std::vector<std::string> exchanges_;
    double volatility_;
    double trend_;
    
    // Random generators
    mutable std::mt19937 generator_;
    mutable std::normal_distribution<double> normal_dist_;
    mutable std::uniform_real_distribution<double> uniform_dist_;
    
    // News event simulation
    struct NewsEvent {
        std::chrono::steady_clock::time_point start_time;
        std::chrono::seconds duration;
        double impact_percentage;
        bool active;
    };
    NewsEvent current_news_event_;
    
    // Callbacks
    std::function<void(const MarketDataTick&)> tick_callback_;
    std::function<void(const std::string&, const std::string&, std::shared_ptr<OrderBook>)> orderbook_callback_;
    
    mutable std::mutex data_mutex_;
};

} // namespace moneybot
