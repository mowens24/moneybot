#include "../../include/core/exchange_manager.h"
#include <random>
#include <cmath>
#include <thread>

namespace moneybot {

ExchangeManager::ExchangeManager(std::shared_ptr<SimpleLogger> logger, ConfigManager& config)
    : logger_(logger), config_(config) {
    
    // Initialize exchange statuses from config
    const auto& cfg = config_.getConfig();
    if (cfg.contains("multi_asset") && cfg["multi_asset"].contains("exchanges")) {
        const auto& exchanges = cfg["multi_asset"]["exchanges"];
        
        if (exchanges.is_array()) {
            for (const auto& exchange : exchanges) {
                if (exchange.contains("name")) {
                    std::string name = exchange["name"];
                    ExchangeStatus status;
                    status.name = name;
                    status.connected = false;
                    status.latency_ms = 0.0;
                    status.last_update = std::chrono::steady_clock::now();
                    status.order_count_24h = 0;
                    status.volume_24h = 0.0;
                    exchanges_[name] = status;
                    
                    logger_->info("Configured exchange: " + name);
                }
            }
        }
    }
    
    logger_->info("ExchangeManager initialized with " + std::to_string(exchanges_.size()) + " exchanges");
}

bool ExchangeManager::connectToExchanges() {
    logger_->info("Connecting to exchanges...");
    
    for (auto& [name, status] : exchanges_) {
        logger_->info("Connecting to " + name + "...");
        
        // Simulate connection (in real implementation, this would be actual API connection)
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        status.connected = true;
        status.latency_ms = 50.0 + (rand() % 50); // Simulate 50-100ms latency
        status.last_update = std::chrono::steady_clock::now();
        
        logger_->info("Connected to " + name + " (latency: " + std::to_string(status.latency_ms) + "ms)");
    }
    
    // Start market data simulation
    simulateMarketData();
    
    logger_->info("All exchanges connected successfully");
    return true;
}

bool ExchangeManager::disconnectFromExchanges() {
    logger_->info("Disconnecting from exchanges...");
    
    stopMarketDataStream();
    
    for (auto& [name, status] : exchanges_) {
        status.connected = false;
        logger_->info("Disconnected from " + name);
    }
    
    return true;
}

bool ExchangeManager::isConnected(const std::string& exchange) const {
    if (exchange.empty()) {
        // Check if any exchange is connected
        for (const auto& [name, status] : exchanges_) {
            if (status.connected) return true;
        }
        return false;
    } else {
        auto it = exchanges_.find(exchange);
        return it != exchanges_.end() && it->second.connected;
    }
}

MarketData ExchangeManager::getMarketData(const std::string& symbol, const std::string& exchange) const {
    std::string key = exchange.empty() ? symbol : symbol + "_" + exchange;
    auto it = market_data_.find(key);
    
    if (it != market_data_.end()) {
        return it->second;
    }
    
    // Return empty market data if not found
    MarketData empty;
    empty.symbol = symbol;
    empty.exchange = exchange;
    return empty;
}

std::vector<MarketData> ExchangeManager::getAllMarketData() const {
    std::vector<MarketData> result;
    for (const auto& [key, data] : market_data_) {
        result.push_back(data);
    }
    return result;
}

std::vector<ExchangeStatus> ExchangeManager::getExchangeStatuses() const {
    std::vector<ExchangeStatus> result;
    for (const auto& [name, status] : exchanges_) {
        result.push_back(status);
    }
    return result;
}

std::vector<std::string> ExchangeManager::getSupportedSymbols() const {
    return {"BTCUSDT", "ETHUSDT", "ADAUSDT", "DOTUSDT", "LINKUSDT"};
}

void ExchangeManager::startMarketDataStream() {
    if (streaming_) return;
    
    streaming_ = true;
    logger_->info("Started market data streaming");
}

void ExchangeManager::stopMarketDataStream() {
    if (!streaming_) return;
    
    streaming_ = false;
    logger_->info("Stopped market data streaming");
}

void ExchangeManager::simulateMarketData() {
    // Simulate real-time market data for major crypto pairs
    std::vector<std::string> symbols = getSupportedSymbols();
    
    // Base prices for simulation
    std::map<std::string, double> base_prices = {
        {"BTCUSDT", 43250.0},
        {"ETHUSDT", 2580.0},
        {"ADAUSDT", 0.45},
        {"DOTUSDT", 6.8},
        {"LINKUSDT", 14.2}
    };
    
    for (const auto& symbol : symbols) {
        for (const auto& [exchange_name, exchange_status] : exchanges_) {
            if (exchange_status.connected) {
                updateMarketData(symbol, exchange_name);
            }
        }
    }
}

void ExchangeManager::updateMarketData(const std::string& symbol, const std::string& exchange) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_real_distribution<> price_change(-0.01, 0.01); // ±1% change
    static std::uniform_real_distribution<> spread(0.0001, 0.001); // 0.01-0.1% spread
    
    // Base prices for realistic simulation
    static std::map<std::string, double> base_prices = {
        {"BTCUSDT", 43250.0},
        {"ETHUSDT", 2580.0},
        {"ADAUSDT", 0.45},
        {"DOTUSDT", 6.8},
        {"LINKUSDT", 14.2}
    };
    
    double base_price = base_prices[symbol];
    double current_price = base_price * (1.0 + price_change(gen));
    double spread_pct = spread(gen);
    
    MarketData data;
    data.symbol = symbol;
    data.exchange = exchange;
    data.last = current_price;
    data.bid = current_price * (1.0 - spread_pct/2);
    data.ask = current_price * (1.0 + spread_pct/2);
    data.volume_24h = 1000000.0 + (rand() % 5000000); // Random volume
    data.timestamp = std::chrono::steady_clock::now();
    
    std::string key = symbol + "_" + exchange;
    market_data_[key] = data;
}

} // namespace moneybot
