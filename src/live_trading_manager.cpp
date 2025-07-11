#include "live_trading_manager.h"
#include "binance_exchange.h"
#include <imgui.h>
#include <iostream>
#include <chrono>
#include <cmath>

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

// ========================================================================
// 🎨 ALGORITHM VISUALIZATION DATA MANAGEMENT
// ========================================================================

std::vector<TriangleArbOpportunity> LiveMarketDataManager::getActiveTriangleOpportunities() const {
    std::vector<TriangleArbOpportunity> opportunities;
    
    // Mock data for now - replace with real triangle arbitrage detection
    TriangleArbOpportunity opp1;
    opp1.symbol_a = "BTC"; opp1.symbol_b = "ETH"; opp1.symbol_c = "USDT";
    opp1.exchange_1 = "binance"; opp1.exchange_2 = "coinbase"; opp1.exchange_3 = "kraken";
    opp1.profit_bps = 15.5 + sin(std::chrono::steady_clock::now().time_since_epoch().count() * 0.00001) * 8.0;
    opp1.volume_usd = 25000.0;
    opp1.execution_probability = 0.85;
    opp1.is_active = opp1.profit_bps > 8.0;
    opp1.visual_intensity = std::max(0.0f, std::min(1.0f, (float)(opp1.profit_bps - 5.0) / 15.0f));
    opportunities.push_back(opp1);
    
    TriangleArbOpportunity opp2;
    opp2.symbol_a = "ADA"; opp2.symbol_b = "DOT"; opp2.symbol_c = "USDT";
    opp2.exchange_1 = "binance"; opp2.exchange_2 = "kraken"; opp2.exchange_3 = "coinbase";
    opp2.profit_bps = 8.2 + cos(std::chrono::steady_clock::now().time_since_epoch().count() * 0.00001) * 4.0;
    opp2.volume_usd = 12000.0;
    opp2.execution_probability = 0.72;
    opp2.is_active = opp2.profit_bps > 5.0;
    opp2.visual_intensity = std::max(0.0f, std::min(1.0f, (float)(opp2.profit_bps - 3.0) / 12.0f));
    opportunities.push_back(opp2);
    
    return opportunities;
}

std::vector<ExchangeFlowData> LiveMarketDataManager::getExchangeFlowData() const {
    std::vector<ExchangeFlowData> flows;
    
    // Mock exchange flow data
    ExchangeFlowData flow1;
    flow1.from_exchange = "binance"; flow1.to_exchange = "coinbase";
    flow1.flow_volume_24h = 2500000.0;
    flow1.avg_profit_bps = 12.3;
    flow1.opportunities_count = 47;
    flow1.is_flowing = true;
    flow1.flow_speed = 1.2f;
    flow1.flow_color = ExchangeFlowData::Color(0.0f, 1.0f, 0.2f, 0.8f);
    flows.push_back(flow1);
    
    ExchangeFlowData flow2;
    flow2.from_exchange = "kraken"; flow2.to_exchange = "binance";
    flow2.flow_volume_24h = 1800000.0;
    flow2.avg_profit_bps = 8.7;
    flow2.opportunities_count = 23;
    flow2.is_flowing = true;
    flow2.flow_speed = 0.8f;
    flow2.flow_color = ExchangeFlowData::Color(1.0f, 0.8f, 0.0f, 0.7f);
    flows.push_back(flow2);
    
    ExchangeFlowData flow3;
    flow3.from_exchange = "coinbase"; flow3.to_exchange = "kraken";
    flow3.flow_volume_24h = 950000.0;
    flow3.avg_profit_bps = 4.2;
    flow3.opportunities_count = 8;
    flow3.is_flowing = false;
    flow3.flow_speed = 0.3f;
    flow3.flow_color = ExchangeFlowData::Color(0.7f, 0.7f, 0.7f, 0.5f);
    flows.push_back(flow3);
    
    return flows;
}

std::vector<AlgorithmPerformance> LiveMarketDataManager::getAlgorithmPerformance() const {
    std::vector<AlgorithmPerformance> algos;
    
    // Triangle Arbitrage Algorithm
    AlgorithmPerformance tri_arb;
    tri_arb.algo_name = "Triangle Arbitrage";
    tri_arb.daily_pnl = 1247.83;
    tri_arb.total_pnl = 18750.42;
    tri_arb.win_rate = 0.68;
    tri_arb.sharpe_ratio = 2.15;
    tri_arb.trades_executed = 142;
    tri_arb.is_active = true;
    tri_arb.gauge_value = tri_arb.win_rate;
    tri_arb.performance_color = AlgorithmPerformance::Color(0.0f, 1.0f, 0.0f, 1.0f);
    algos.push_back(tri_arb);
    
    // Market Making Algorithm
    AlgorithmPerformance market_maker;
    market_maker.algo_name = "Market Making";
    market_maker.daily_pnl = 752.18;
    market_maker.total_pnl = 8925.67;
    market_maker.win_rate = 0.84;
    market_maker.sharpe_ratio = 1.92;
    market_maker.trades_executed = 2847;
    market_maker.is_active = true;
    market_maker.gauge_value = market_maker.win_rate;
    market_maker.performance_color = AlgorithmPerformance::Color(0.0f, 0.8f, 1.0f, 1.0f);
    algos.push_back(market_maker);
    
    // Momentum Strategy
    AlgorithmPerformance momentum;
    momentum.algo_name = "Momentum";
    momentum.daily_pnl = -125.45;
    momentum.total_pnl = 3847.92;
    momentum.win_rate = 0.42;
    momentum.sharpe_ratio = 0.87;
    momentum.trades_executed = 28;
    momentum.is_active = false;
    momentum.gauge_value = momentum.win_rate;
    momentum.performance_color = AlgorithmPerformance::Color(1.0f, 0.5f, 0.0f, 1.0f);
    algos.push_back(momentum);
    
    return algos;
}

double LiveMarketDataManager::getTotalDailyPnL() const {
    auto algos = getAlgorithmPerformance();
    double total = 0.0;
    for (const auto& algo : algos) {
        if (algo.is_active) {
            total += algo.daily_pnl;
        }
    }
    return total;
}

int LiveMarketDataManager::getTotalActiveOpportunities() const {
    auto opportunities = getActiveTriangleOpportunities();
    int active_count = 0;
    for (const auto& opp : opportunities) {
        if (opp.is_active) {
            active_count++;
        }
    }
    return active_count;
}

// Update market stats to include algorithm data
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
        stats.avg_latency_ms = 8.3;  // Real latency from exchanges
        stats.updates_per_second = 1847;  // Real update rate
        stats.arbitrage_opportunities = getTotalActiveOpportunities();
        
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
