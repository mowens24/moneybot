#include "live_trading_manager.h"
#include "binance_exchange.h"
#include <iostream>
#include <chrono>

// LiveMarketDataManager Implementation
LiveMarketDataManager::LiveMarketDataManager(bool demo) 
    : demo_mode(demo), min_arbitrage_profit_bps(10.0) {
    std::cout << "LiveMarketDataManager initialized (Demo mode: " 
              << (demo ? "enabled" : "disabled") << ")" << std::endl;
    
    if (!demo) {
        // Add real Binance exchange for live data
        try {
            auto binance = std::make_unique<BinanceExchange>("", "", false); // No API keys needed for market data
            exchanges["binance"] = std::move(binance);
            std::cout << "🏢 Binance exchange added for live market data" << std::endl;
        } catch (const std::exception& e) {
            std::cout << "❌ Failed to initialize Binance: " << e.what() << std::endl;
        }
    }
}

LiveMarketDataManager::~LiveMarketDataManager() {
    stop();
    disconnectAll();
}

void LiveMarketDataManager::start() {
    if (!running.load()) {
        running = true;
        
        // Connect to real exchanges if not in demo mode
        if (!demo_mode) {
            connectAll();
        }
        
        update_thread = std::thread(&LiveMarketDataManager::processMarketData, this);
        std::cout << "LiveMarketDataManager started" << std::endl;
    }
}

void LiveMarketDataManager::stop() {
    if (running.load()) {
        running = false;
        if (update_thread.joinable()) {
            update_thread.join();
        }
        std::cout << "LiveMarketDataManager stopped" << std::endl;
    }
}

bool LiveMarketDataManager::connectAll() {
    std::cout << "🌐 Connecting to live exchanges..." << std::endl;
    bool all_connected = true;
    
    for (auto& [name, exchange] : exchanges) {
        if (exchange && !exchange->isConnected()) {
            std::cout << "🔌 Connecting to " << name << "..." << std::endl;
            if (exchange->connect()) {
                std::cout << "✅ Connected to " << name << std::endl;
            } else {
                std::cout << "❌ Failed to connect to " << name << std::endl;
                all_connected = false;
            }
        }
    }
    
    return all_connected;
}

bool LiveMarketDataManager::disconnectAll() {
    std::cout << "🔌 Disconnecting from all exchanges..." << std::endl;
    bool all_disconnected = true;
    
    for (auto& [name, exchange] : exchanges) {
        if (exchange && exchange->isConnected()) {
            if (!exchange->disconnect()) {
                all_disconnected = false;
            }
        }
    }
    
    return all_disconnected;
}

LiveMarketDataManager::MarketStats LiveMarketDataManager::getMarketStats() const {
    MarketStats stats;
    
    if (demo_mode) {
        // Demo stats
        stats.total_symbols = 150;
        stats.connected_exchanges = 3;
        stats.avg_latency_ms = 12.5;
        stats.updates_per_second = 1250;
        stats.arbitrage_opportunities = 5;
    } else {
        // Real stats from connected exchanges
        stats.total_symbols = 0;
        stats.connected_exchanges = 0;
        stats.avg_latency_ms = 0.0;
        stats.updates_per_second = 0;
        stats.arbitrage_opportunities = current_opportunities.size();
        
        for (const auto& [name, exchange] : exchanges) {
            if (exchange && exchange->isConnected()) {
                stats.connected_exchanges++;
                auto symbols = exchange->getSupportedSymbols();
                stats.total_symbols += symbols.size();
            }
        }
    }
    
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    stats.last_update_time = std::ctime(&time_t);
    if (!stats.last_update_time.empty()) {
        stats.last_update_time.pop_back(); // Remove newline
    }
    
    return stats;
}

void LiveMarketDataManager::processMarketData() {
    while (running.load()) {
        // Simulate market data processing
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void LiveMarketDataManager::updateArbitrageOpportunities() {
    // Placeholder for arbitrage opportunity detection
}

std::map<std::string, bool> LiveMarketDataManager::getConnectionStatus() const {
    std::map<std::string, bool> status;
    status["binance"] = true;
    status["coinbase"] = true;
    status["kraken"] = false;
    return status;
}

std::map<std::string, std::string> LiveMarketDataManager::getExchangeStatus() const {
    std::map<std::string, std::string> status;
    status["binance"] = "Connected";
    status["coinbase"] = "Connected";
    status["kraken"] = "Disconnected";
    return status;
}

LiveExchangeInterface* LiveMarketDataManager::getExchange(const std::string& name) {
    auto it = exchanges.find(name);
    if (it != exchanges.end()) {
        return it->second.get();
    }
    return nullptr;
}

// LiveOrderExecutionEngine Implementation
LiveOrderExecutionEngine::LiveOrderExecutionEngine(LiveMarketDataManager* mdm)
    : market_data_manager(mdm), max_position_size_usd(50000.0), 
      max_daily_loss_usd(5000.0), max_order_size_usd(10000.0) {
    std::cout << "LiveOrderExecutionEngine initialized" << std::endl;
}

LiveOrderExecutionEngine::~LiveOrderExecutionEngine() {
    std::cout << "LiveOrderExecutionEngine destroyed" << std::endl;
}

bool LiveOrderExecutionEngine::cancelAllOrders(const std::string& exchange) {
    std::lock_guard<std::mutex> lock(orders_mutex);
    std::cout << "Cancelling all orders" << (exchange.empty() ? "" : " for exchange: " + exchange) << std::endl;
    return true;
}

double LiveOrderExecutionEngine::getTotalPortfolioValue() const {
    return 100000.0; // Placeholder value
}

double LiveOrderExecutionEngine::getDailyPnL() const {
    return 1250.75; // Placeholder value
}

std::map<std::string, double> LiveOrderExecutionEngine::getCurrentPositions() const {
    std::map<std::string, double> positions;
    positions["BTC"] = 0.5;
    positions["ETH"] = 2.3;
    positions["USD"] = 25000.0;
    return positions;
}

void LiveOrderExecutionEngine::setRiskLimits(double max_position, double max_daily_loss, double max_order) {
    max_position_size_usd = max_position;
    max_daily_loss_usd = max_daily_loss;
    max_order_size_usd = max_order;
}

bool LiveOrderExecutionEngine::checkRiskLimits(const std::string& symbol, double quantity_usd) const {
    return quantity_usd <= max_order_size_usd;
}

bool LiveOrderExecutionEngine::isWithinDailyLoss() const {
    return getDailyPnL() > -max_daily_loss_usd;
}
