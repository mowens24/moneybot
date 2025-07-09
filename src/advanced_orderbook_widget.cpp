#include "advanced_orderbook_widget.h"
#include <algorithm>
#include <random>
#include <cmath>

namespace moneybot {

void AdvancedOrderBookWidget::RenderOrderBook(const std::string& symbol, 
                                             MarketDataSimulator* simulator,
                                             int depth) {
    if (!simulator) return;
    
    auto tick = simulator->getLatestTick(symbol, "binance");
    if (tick.symbol.empty()) return;
    
    double mid_price = (tick.bid_price + tick.ask_price) / 2.0;
    
    // Generate realistic order book data
    auto bids = generateMockOrderBook(mid_price, true, depth);
    auto asks = generateMockOrderBook(mid_price, false, depth);
    
    ImGui::BeginChild("OrderBookWidget", ImVec2(0, 400), true);
    ImGui::TextColored(ImVec4(0.0f, 1.0f, 1.0f, 1.0f), "📖 Order Book: %s", symbol.c_str());
    ImGui::Separator();
    
    // Order book table
    if (ImGui::BeginTable("OrderBook", 6, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
        ImGui::TableSetupColumn("Cum. Size");
        ImGui::TableSetupColumn("Size");
        ImGui::TableSetupColumn("Bid");
        ImGui::TableSetupColumn("Ask");
        ImGui::TableSetupColumn("Size");
        ImGui::TableSetupColumn("Cum. Size");
        ImGui::TableHeadersRow();
        
        // Display order book levels
        for (int i = 0; i < std::min(depth, (int)std::min(bids.size(), asks.size())); ++i) {
            ImGui::TableNextRow();
            
            // Bid side
            ImGui::TableSetColumnIndex(0);
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "%.1f", bids[i].cumulative_size);
            ImGui::TableSetColumnIndex(1);
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "%.2f", bids[i].size);
            ImGui::TableSetColumnIndex(2);
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "%.2f", bids[i].price);
            
            // Ask side
            ImGui::TableSetColumnIndex(3);
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "%.2f", asks[i].price);
            ImGui::TableSetColumnIndex(4);
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "%.2f", asks[i].size);
            ImGui::TableSetColumnIndex(5);
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "%.1f", asks[i].cumulative_size);
        }
        ImGui::EndTable();
    }
    
    ImGui::Separator();
    
    // Market stats
    ImGui::Text("💰 Spread: $%.2f (%.2f bps)", 
               tick.ask_price - tick.bid_price,
               ((tick.ask_price - tick.bid_price) / mid_price) * 10000.0);
    ImGui::Text("📊 Mid Price: $%.2f", mid_price);
    ImGui::Text("📈 Bid Depth: %.1f | Ask Depth: %.1f", bids[depth-1].cumulative_size, asks[depth-1].cumulative_size);
    
    ImGui::EndChild();
}

void AdvancedOrderBookWidget::RenderDepthChart(const std::vector<OrderBookLevel>& bids,
                                              const std::vector<OrderBookLevel>& asks,
                                              double mid_price) {
    ImGui::BeginChild("DepthChart", ImVec2(0, 200), true);
    ImGui::Text("📊 Market Depth Visualization");
    
    if (!bids.empty() && !asks.empty()) {
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
        ImVec2 canvas_size = ImGui::GetContentRegionAvail();
        canvas_size.y = 150; // Fixed height for chart
        
        if (canvas_size.x > 0 && canvas_size.y > 0) {
            // Draw background
            draw_list->AddRectFilled(canvas_pos, 
                                   ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y),
                                   IM_COL32(20, 20, 30, 255));
            
            // Draw depth curves
            double max_cum_size = std::max(bids.back().cumulative_size, asks.back().cumulative_size);
            
            // Bid curve (green)
            for (size_t i = 1; i < bids.size(); ++i) {
                float x1 = canvas_pos.x + (i - 1) * canvas_size.x / (2 * bids.size());
                float y1 = canvas_pos.y + canvas_size.y - (bids[i-1].cumulative_size / max_cum_size) * canvas_size.y;
                float x2 = canvas_pos.x + i * canvas_size.x / (2 * bids.size());
                float y2 = canvas_pos.y + canvas_size.y - (bids[i].cumulative_size / max_cum_size) * canvas_size.y;
                draw_list->AddLine(ImVec2(x1, y1), ImVec2(x2, y2), IM_COL32(0, 255, 0, 200), 2.0f);
            }
            
            // Ask curve (red)
            for (size_t i = 1; i < asks.size(); ++i) {
                float x1 = canvas_pos.x + canvas_size.x/2 + (i - 1) * canvas_size.x / (2 * asks.size());
                float y1 = canvas_pos.y + canvas_size.y - (asks[i-1].cumulative_size / max_cum_size) * canvas_size.y;
                float x2 = canvas_pos.x + canvas_size.x/2 + i * canvas_size.x / (2 * asks.size());
                float y2 = canvas_pos.y + canvas_size.y - (asks[i].cumulative_size / max_cum_size) * canvas_size.y;
                draw_list->AddLine(ImVec2(x1, y1), ImVec2(x2, y2), IM_COL32(255, 0, 0, 200), 2.0f);
            }
            
            // Draw mid-price line
            float mid_x = canvas_pos.x + canvas_size.x / 2;
            draw_list->AddLine(ImVec2(mid_x, canvas_pos.y), 
                             ImVec2(mid_x, canvas_pos.y + canvas_size.y),
                             IM_COL32(255, 255, 0, 150), 1.0f);
        }
        
        ImGui::Dummy(canvas_size);
    }
    
    ImGui::EndChild();
}

std::vector<AdvancedOrderBookWidget::OrderBookLevel> 
AdvancedOrderBookWidget::generateMockOrderBook(double mid_price, bool is_bid, int depth) {
    std::vector<OrderBookLevel> levels;
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::normal_distribution<> size_dist(50.0, 20.0);
    
    double cumulative = 0.0;
    for (int i = 0; i < depth; ++i) {
        OrderBookLevel level;
        
        // Price calculation
        double tick_size = mid_price < 100 ? 0.01 : (mid_price < 10000 ? 0.1 : 1.0);
        double price_offset = (i + 1) * tick_size;
        level.price = is_bid ? mid_price - price_offset : mid_price + price_offset;
        
        // Size calculation (larger sizes closer to mid)
        level.size = std::max(1.0, size_dist(gen) * (1.0 - (double)i / depth));
        cumulative += level.size;
        level.cumulative_size = cumulative;
        level.order_count = std::max(1, (int)(level.size / 10));
        
        levels.push_back(level);
    }
    
    return levels;
}

} // namespace moneybot
