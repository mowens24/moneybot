#include "market_data_simulator.h"
#include <iostream>
#include <iomanip>
#include <cmath>

namespace moneybot {

MarketDataSimulator::MarketDataSimulator() 
    : running_(false), update_interval_(std::chrono::milliseconds(100)), 
      volatility_(0.02), trend_(0.0), generator_(std::chrono::steady_clock::now().time_since_epoch().count()),
      normal_dist_(0.0, 1.0), uniform_dist_(0.0, 1.0) {
    
    current_news_event_ = {std::chrono::steady_clock::now(), std::chrono::seconds(0), 0.0, false};
    
    // Initialize exchanges first
    exchanges_ = {"binance", "coinbase", "kraken"};
    
    // Then add symbols with base prices
    addSymbol("BTCUSDT", 45000.0);
    addSymbol("ETHUSDT", 3000.0);
    addSymbol("ADAUSDT", 0.85);
    addSymbol("DOTUSDT", 25.0);
    addSymbol("LINKUSDT", 15.0);
}

MarketDataSimulator::~MarketDataSimulator() {
    stop();
}

void MarketDataSimulator::start() {
    if (running_) {
        return;
    }
    
    running_ = true;
    std::cout << "🚀 Starting Market Data Simulator..." << std::endl;
    std::cout << "📊 Symbols: ";
    for (const auto& symbol : symbols_) {
        std::cout << symbol << " ";
    }
    std::cout << std::endl;
    std::cout << "🏢 Exchanges: ";
    for (const auto& exchange : exchanges_) {
        std::cout << exchange << " ";
    }
    std::cout << std::endl;
    
    simulation_thread_ = std::thread(&MarketDataSimulator::simulationLoop, this);
}

void MarketDataSimulator::stop() {
    if (!running_) {
        return;
    }
    
    running_ = false;
    if (simulation_thread_.joinable()) {
        simulation_thread_.join();
    }
    std::cout << "⏹️ Market Data Simulator stopped" << std::endl;
}

void MarketDataSimulator::addSymbol(const std::string& symbol, double base_price) {
    symbols_.push_back(symbol);
    
    // Initialize prices for all exchanges
    for (const auto& exchange : exchanges_) {
        // Add small random variation between exchanges (0.1% - 0.5%)
        double variation = 1.0 + (uniform_dist_(generator_) * 0.004 + 0.001) * (uniform_dist_(generator_) > 0.5 ? 1 : -1);
        symbol_prices_[symbol][exchange] = base_price * variation;
    }
}

void MarketDataSimulator::addExchange(const std::string& exchange_name) {
    exchanges_.push_back(exchange_name);
    
    // Initialize prices for all symbols on this exchange
    for (const auto& symbol : symbols_) {
        if (symbol_prices_[symbol].find(exchange_name) == symbol_prices_[symbol].end()) {
            // Use average price from other exchanges if available
            double avg_price = 0.0;
            int count = 0;
            for (const auto& [ex, price] : symbol_prices_[symbol]) {
                avg_price += price;
                count++;
            }
            if (count > 0) {
                avg_price /= count;
                double variation = 1.0 + (uniform_dist_(generator_) * 0.004 + 0.001) * (uniform_dist_(generator_) > 0.5 ? 1 : -1);
                symbol_prices_[symbol][exchange_name] = avg_price * variation;
            }
        }
    }
}

void MarketDataSimulator::simulateNewsEvent(double impact_percentage, std::chrono::seconds duration) {
    current_news_event_ = {
        std::chrono::steady_clock::now(),
        duration,
        impact_percentage,
        true
    };
    
    std::cout << "📰 NEWS EVENT: " << impact_percentage << "% market impact for " 
              << duration.count() << " seconds" << std::endl;
}

void MarketDataSimulator::simulationLoop() {
    auto last_update = std::chrono::steady_clock::now();
    auto last_status = std::chrono::steady_clock::now();
    
    while (running_) {
        auto now = std::chrono::steady_clock::now();
        
        if (now - last_update >= update_interval_) {
            updatePrices();
            updateOrderBooks();
            last_update = now;
        }
        
        // Print status every 5 seconds
        if (now - last_status >= std::chrono::seconds(5)) {
            std::cout << "📈 Market Update - ";
            for (const auto& symbol : symbols_) {
                if (symbol == "BTCUSDT") { // Show only BTC for brevity
                    auto avg_price = 0.0;
                    int count = 0;
                    for (const auto& [exchange, price] : symbol_prices_[symbol]) {
                        avg_price += price;
                        count++;
                    }
                    if (count > 0) {
                        avg_price /= count;
                        std::cout << symbol << ": $" << std::fixed << std::setprecision(2) << avg_price << " ";
                    }
                }
            }
            std::cout << std::endl;
            last_status = now;
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void MarketDataSimulator::updatePrices() {
    std::lock_guard<std::mutex> lock(data_mutex_);
    
    for (const auto& symbol : symbols_) {
        for (const auto& exchange : exchanges_) {
            double current_price = symbol_prices_[symbol][exchange];
            double new_price = generatePrice(symbol, current_price);
            new_price = applyNewsImpact(new_price, symbol);
            
            symbol_prices_[symbol][exchange] = new_price;
            generateTick(symbol, exchange);
        }
    }
}

void MarketDataSimulator::updateOrderBooks() {
    std::lock_guard<std::mutex> lock(data_mutex_);
    
    // For simulation purposes, we'll skip the order book generation
    // to avoid logger conflicts. In production, this would be properly integrated
    // with the existing order book system.
}

void MarketDataSimulator::generateTick(const std::string& symbol, const std::string& exchange) {
    double price = symbol_prices_[symbol][exchange];
    
    // Generate bid/ask spread (0.01% - 0.1%)
    double spread_bps = 1.0 + uniform_dist_(generator_) * 9.0; // 1-10 bps
    double spread = price * spread_bps / 10000.0;
    
    double bid_price = price - spread / 2.0;
    double ask_price = price + spread / 2.0;
    
    // Generate sizes (realistic trading volumes)
    double base_size = symbol == "BTCUSDT" ? 0.5 : (symbol == "ETHUSDT" ? 5.0 : 100.0);
    double bid_size = base_size * (0.5 + uniform_dist_(generator_));
    double ask_size = base_size * (0.5 + uniform_dist_(generator_));
    
    // Simulate 24h volume
    double volume_24h = price * base_size * 1000 * (0.8 + uniform_dist_(generator_) * 0.4);
    
    MarketDataTick tick = {
        symbol,
        exchange,
        bid_price,
        ask_price,
        bid_size,
        ask_size,
        price,
        volume_24h,
        std::chrono::steady_clock::now()
    };
    
    latest_ticks_[symbol][exchange] = tick;
    
    if (tick_callback_) {
        tick_callback_(tick);
    }
}

std::shared_ptr<OrderBook> MarketDataSimulator::generateOrderBook(const std::string& symbol, 
                                                                  const std::string& exchange, 
                                                                  double mid_price) {
    // For the simulation, we'll create a simplified order book representation
    // Since the existing OrderBook is tightly coupled with database operations,
    // we'll just store the data for retrieval but not use the full OrderBook class
    
    // Return nullptr for now to avoid the logger conflict
    // In a production system, this would be properly integrated
    return nullptr;
}

double MarketDataSimulator::generatePrice(const std::string& symbol, double current_price) {
    // Base random walk with volatility
    double random_change = normal_dist_(generator_) * volatility_ / std::sqrt(252 * 24 * 60 * 60 / 0.1); // Scaled for 100ms updates
    
    // Add trend component
    double trend_change = trend_ * volatility_ / (252 * 24 * 60 * 60 / 0.1);
    
    // Apply mean reversion (prevents prices from drifting too far)
    static std::map<std::string, double> base_prices = {
        {"BTCUSDT", 45000.0}, {"ETHUSDT", 3000.0}, {"ADAUSDT", 0.85}, 
        {"DOTUSDT", 25.0}, {"LINKUSDT", 15.0}
    };
    
    double base_price = base_prices.count(symbol) ? base_prices[symbol] : current_price;
    double reversion_strength = 0.001; // How strong the mean reversion is
    double reversion_change = -reversion_strength * (current_price - base_price) / base_price;
    
    double total_change = random_change + trend_change + reversion_change;
    double new_price = current_price * (1.0 + total_change);
    
    // Ensure price doesn't go negative or too extreme
    return std::max(new_price, current_price * 0.5);
}

double MarketDataSimulator::applyNewsImpact(double price, const std::string& symbol) {
    if (!current_news_event_.active) {
        return price;
    }
    
    auto now = std::chrono::steady_clock::now();
    auto elapsed = now - current_news_event_.start_time;
    
    if (elapsed > current_news_event_.duration) {
        current_news_event_.active = false;
        return price;
    }
    
    // Apply news impact with decay over time
    double progress = static_cast<double>(elapsed.count()) / current_news_event_.duration.count();
    double impact_decay = std::exp(-progress * 3.0); // Exponential decay
    double current_impact = current_news_event_.impact_percentage * impact_decay / 100.0;
    
    return price * (1.0 + current_impact);
}

MarketDataTick MarketDataSimulator::getLatestTick(const std::string& symbol, const std::string& exchange) const {
    std::lock_guard<std::mutex> lock(data_mutex_);
    
    auto symbol_it = latest_ticks_.find(symbol);
    if (symbol_it != latest_ticks_.end()) {
        auto exchange_it = symbol_it->second.find(exchange);
        if (exchange_it != symbol_it->second.end()) {
            return exchange_it->second;
        }
    }
    
    return MarketDataTick{}; // Return empty tick if not found
}

std::shared_ptr<OrderBook> MarketDataSimulator::getOrderBook(const std::string& symbol, const std::string& exchange) const {
    std::lock_guard<std::mutex> lock(data_mutex_);
    
    auto symbol_it = order_books_.find(symbol);
    if (symbol_it != order_books_.end()) {
        auto exchange_it = symbol_it->second.find(exchange);
        if (exchange_it != symbol_it->second.end()) {
            return exchange_it->second;
        }
    }
    
    return nullptr;
}

} // namespace moneybot
