#pragma once
#include <imgui.h>
#include <vector>
#include <string>
#include "market_data_simulator.h"

namespace moneybot {

// Advanced order book visualization widget
class AdvancedOrderBookWidget {
public:
    struct OrderBookLevel {
        double price;
        double size;
        double cumulative_size;
        int order_count;
    };
    
    static void RenderOrderBook(const std::string& symbol, 
                               MarketDataSimulator* simulator,
                               int depth = 20);
    
    static void RenderDepthChart(const std::vector<OrderBookLevel>& bids,
                                const std::vector<OrderBookLevel>& asks,
                                double mid_price);
    
    static void RenderOrderFlow(const std::string& symbol);
    
private:
    static std::vector<OrderBookLevel> generateMockOrderBook(double mid_price, bool is_bid, int depth);
    static void drawPriceLevel(const OrderBookLevel& level, bool is_bid, double max_size, double mid_price);
};

} // namespace moneybot
