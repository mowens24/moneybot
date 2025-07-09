#include "live_trading_manager.h"
#include "binance_exchange.h"
#include <iostream>
#include <chrono>
#include <thread>

int main() {
    std::cout << "🚀 Testing Live Binance US Data Connection..." << std::endl;
    
    try {
        // Create and test Binance exchange directly
        auto binance = std::make_unique<BinanceExchange>("", "", false);
        
        std::cout << "🔌 Attempting to connect to Binance US..." << std::endl;
        if (binance->connect()) {
            std::cout << "✅ Connected successfully!" << std::endl;
            
            // Wait for some live data
            std::cout << "📊 Waiting for live data (30 seconds)..." << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(30));
            
            // Try to get latest tick for BTC
            auto btc_tick = binance->getLatestTick("BTCUSDT");
            if (!btc_tick.symbol.empty()) {
                std::cout << "🎯 Successfully retrieved live data for " << btc_tick.symbol << std::endl;
                std::cout << "   Last Price: $" << btc_tick.last_price << std::endl;
                std::cout << "   Bid: $" << btc_tick.bid_price << std::endl;
                std::cout << "   Ask: $" << btc_tick.ask_price << std::endl;
            } else {
                std::cout << "❌ No data received for BTCUSDT" << std::endl;
            }
            
            binance->disconnect();
        } else {
            std::cout << "❌ Failed to connect to Binance US" << std::endl;
            return 1;
        }
        
    } catch (const std::exception& e) {
        std::cout << "❌ Exception: " << e.what() << std::endl;
        return 1;
    }
    
    std::cout << "✅ Live data test completed successfully!" << std::endl;
    return 0;
}
