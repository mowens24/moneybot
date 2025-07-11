#include "gui/algorithm_window.h"
#include "strategy/strategy_engine.h"
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <iostream>
#include <cmath>

namespace moneybot {
namespace gui {

AlgorithmWindow::AlgorithmWindow(std::shared_ptr<StrategyEngine> strategy_engine)
    : strategy_engine_(strategy_engine) {
    // Initialize chart data
    strategy_performance_.reserve(100);
    opportunity_profits_.reserve(100);
    
    // Initialize demo strategy metrics
    strategy_metrics_.push_back({
        "Triangle Arbitrage", true, 12.5, 2.3, 68.5, 142, 3.2, 1.85, "Running"
    });
    strategy_metrics_.push_back({
        "Cross Exchange Arbitrage", true, 8.7, 1.8, 72.1, 89, 2.1, 1.92, "Running"
    });
    strategy_metrics_.push_back({
        "Market Making", false, 15.2, 0.0, 65.3, 256, 1.8, 1.67, "Stopped"
    });
    
    // Initialize demo opportunities
    opportunities_.push_back({
        "triangle", "BTC/ETH/USDT", 12.5, 50000, "BTC → ETH → USDT → BTC", true
    });
    opportunities_.push_back({
        "cross_exchange", "BTC/USDT", 8.3, 25000, "Binance → Coinbase", true
    });
    opportunities_.push_back({
        "triangle", "ETH/ADA/USDT", 6.7, 30000, "ETH → ADA → USDT → ETH", false
    });
}

void AlgorithmWindow::render() {
    if (!ImGui::Begin("🤖 Algorithm Dashboard")) {
        ImGui::End();
        return;
    }
    
    // Update data first
    updateData();
    
    // Header
    ImGui::TextColored(ImVec4(0.0f, 1.0f, 1.0f, 1.0f), "AI-Powered Trading Algorithms");
    ImGui::Separator();
    
    // Main content in tabs
    if (ImGui::BeginTabBar("AlgorithmTabs")) {
        
        // Strategy Control tab
        if (ImGui::BeginTabItem("Strategy Control")) {
            renderStrategyControl();
            ImGui::EndTabItem();
        }
        
        // Opportunities tab
        if (ImGui::BeginTabItem("Live Opportunities")) {
            renderOpportunitiesTable();
            ImGui::EndTabItem();
        }
        
        // Performance tab
        if (ImGui::BeginTabItem("Performance")) {
            renderPerformanceCharts();
            ImGui::EndTabItem();
        }
        
        // Triangle Network tab
        if (ImGui::BeginTabItem("Triangle Network")) {
            renderTriangleNetwork();
            ImGui::EndTabItem();
        }
        
        // Cross Exchange Matrix tab
        if (ImGui::BeginTabItem("Cross Exchange")) {
            renderCrossExchangeMatrix();
            ImGui::EndTabItem();
        }
        
        ImGui::EndTabBar();
    }
    
    ImGui::End();
}

void AlgorithmWindow::updateData() {
    if (!strategy_engine_) return;
    
    // Update strategy metrics from real data
    static auto last_update = std::chrono::steady_clock::now();
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - last_update).count();
    
    if (elapsed > 2) {
        // Update performance data
        for (auto& metrics : strategy_metrics_) {
            if (metrics.is_active) {
                // Simulate performance updates
                double change = ((rand() % 200) - 100) / 10000.0; // ±1% change
                metrics.daily_return += change * 100;
                metrics.total_return += change * 100;
                
                // Update trade count occasionally
                if (rand() % 20 == 0) {
                    metrics.total_trades++;
                    // Update win rate
                    if (rand() % 100 < 70) {
                        metrics.win_rate = (metrics.win_rate * (metrics.total_trades - 1) + 100.0) / metrics.total_trades;
                    } else {
                        metrics.win_rate = (metrics.win_rate * (metrics.total_trades - 1)) / metrics.total_trades;
                    }
                }
            }
        }
        
        // Update opportunities
        for (auto& opp : opportunities_) {
            if (opp.is_active) {
                // Simulate profit changes
                double change = ((rand() % 100) - 50) / 1000.0; // ±5% change
                opp.profit_bps += change;
                opp.profit_bps = std::max(0.0, opp.profit_bps); // Don't go negative
            }
        }
        
        last_update = now;
    }
}

void AlgorithmWindow::renderStrategyControl() {
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "🎯 Strategy Control Center");
    
    // Overall status
    int active_strategies = 0;
    for (const auto& metrics : strategy_metrics_) {
        if (metrics.is_active) active_strategies++;
    }
    
    ImGui::Text("Active Strategies: %d / %d", active_strategies, (int)strategy_metrics_.size());
    ImGui::SameLine();
    ImGui::TextColored(active_strategies > 0 ? ImVec4(0.0f, 1.0f, 0.0f, 1.0f) : ImVec4(1.0f, 0.0f, 0.0f, 1.0f), 
                       "●");
    
    // Strategy table
    if (ImGui::BeginTable("StrategyTable", 8, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
        ImGui::TableSetupColumn("Strategy");
        ImGui::TableSetupColumn("Status");
        ImGui::TableSetupColumn("Total Return");
        ImGui::TableSetupColumn("Daily Return");
        ImGui::TableSetupColumn("Win Rate");
        ImGui::TableSetupColumn("Trades");
        ImGui::TableSetupColumn("Sharpe");
        ImGui::TableSetupColumn("Controls");
        ImGui::TableHeadersRow();
        
        int row_id = 0;
        for (const auto& metrics : strategy_metrics_) {
            drawStrategyRow(metrics, row_id++);
        }
        
        ImGui::EndTable();
    }
    
    // Global controls
    ImGui::Separator();
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "⚡ Global Controls");
    
    if (ImGui::Button("🚀 Start All", ImVec2(100, 0))) {
        for (auto& metrics : strategy_metrics_) {
            if (!metrics.is_active) {
                startStrategy(metrics.name);
            }
        }
    }
    ImGui::SameLine();
    
    if (ImGui::Button("⏸️ Pause All", ImVec2(100, 0))) {
        for (auto& metrics : strategy_metrics_) {
            if (metrics.is_active) {
                pauseStrategy(metrics.name);
            }
        }
    }
    ImGui::SameLine();
    
    if (ImGui::Button("🛑 Stop All", ImVec2(100, 0))) {
        for (auto& metrics : strategy_metrics_) {
            if (metrics.is_active) {
                stopStrategy(metrics.name);
            }
        }
    }
}

void AlgorithmWindow::renderOpportunitiesTable() {
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "🎯 Live Arbitrage Opportunities");
    
    // Opportunities summary
    int active_opportunities = 0;
    double total_profit = 0.0;
    for (const auto& opp : opportunities_) {
        if (opp.is_active) {
            active_opportunities++;
            total_profit += opp.profit_bps;
        }
    }
    
    ImGui::Text("Active Opportunities: %d", active_opportunities);
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Total Profit: %s", 
                       formatBasisPoints(total_profit).c_str());
    
    // Opportunities table
    if (ImGui::BeginTable("OpportunitiesTable", 6, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
        ImGui::TableSetupColumn("Type");
        ImGui::TableSetupColumn("Symbol");
        ImGui::TableSetupColumn("Profit (bps)");
        ImGui::TableSetupColumn("Max Volume");
        ImGui::TableSetupColumn("Description");
        ImGui::TableSetupColumn("Status");
        ImGui::TableHeadersRow();
        
        int row_id = 0;
        for (const auto& opp : opportunities_) {
            drawOpportunityRow(opp, row_id++);
        }
        
        ImGui::EndTable();
    }
    
    // Real-time profit chart
    ImGui::Separator();
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "📊 Profit Timeline");
    
    // Update chart data
    opportunity_profits_.push_back(static_cast<float>(total_profit));
    if (opportunity_profits_.size() > 100) {
        opportunity_profits_.erase(opportunity_profits_.begin());
    }
    
    if (!opportunity_profits_.empty()) {
        ImGui::PlotLines("##OpportunityProfit", opportunity_profits_.data(), opportunity_profits_.size(), 
                         0, nullptr, 0.0f, FLT_MAX, ImVec2(0, 150));
    }
}

void AlgorithmWindow::renderPerformanceCharts() {
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "📈 Strategy Performance");
    
    // Strategy selection
    ImGui::Text("Selected Strategy:");
    ImGui::SameLine();
    if (ImGui::BeginCombo("##StrategySelect", selected_strategy_.c_str())) {
        for (const auto& metrics : strategy_metrics_) {
            bool is_selected = (selected_strategy_ == metrics.name);
            if (ImGui::Selectable(metrics.name.c_str(), is_selected)) {
                selected_strategy_ = metrics.name;
            }
            if (is_selected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
    
    // Performance gauges
    ImGui::Columns(3, "PerformanceGauges", false);
    
    for (const auto& metrics : strategy_metrics_) {
        if (selected_strategy_.empty() || selected_strategy_ == metrics.name) {
            drawPerformanceGauge(metrics.name, metrics.total_return);
            if (selected_strategy_.empty()) ImGui::NextColumn();
        }
    }
    
    ImGui::Columns(1);
    
    // Historical performance chart
    ImGui::Separator();
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "📊 Historical Performance");
    
    // Update chart data
    if (!selected_strategy_.empty()) {
        auto it = std::find_if(strategy_metrics_.begin(), strategy_metrics_.end(),
                              [this](const StrategyMetrics& m) { return m.name == selected_strategy_; });
        if (it != strategy_metrics_.end()) {
            strategy_performance_.push_back(static_cast<float>(it->total_return));
            if (strategy_performance_.size() > 100) {
                strategy_performance_.erase(strategy_performance_.begin());
            }
        }
    }
    
    if (!strategy_performance_.empty()) {
        ImGui::PlotLines("##StrategyPerformance", strategy_performance_.data(), strategy_performance_.size(), 
                         0, nullptr, FLT_MAX, FLT_MAX, ImVec2(0, 200));
    }
}

void AlgorithmWindow::renderTriangleNetwork() {
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "🔺 Triangle Arbitrage Network");
    
    // Network visualization
    ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
    ImVec2 canvas_size = ImGui::GetContentRegionAvail();
    canvas_size.y = 400; // Fixed height for network
    
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    draw_list->AddRectFilled(canvas_pos, ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y), 
                            IM_COL32(20, 20, 20, 255));
    draw_list->AddRect(canvas_pos, ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y), 
                      IM_COL32(255, 255, 255, 255));
    
    // Draw triangle nodes
    ImVec2 center = ImVec2(canvas_pos.x + canvas_size.x * 0.5f, canvas_pos.y + canvas_size.y * 0.5f);
    float radius = 120.0f;
    
    // BTC, ETH, USDT triangle
    ImVec2 btc_pos = ImVec2(center.x, center.y - radius);
    ImVec2 eth_pos = ImVec2(center.x - radius * 0.866f, center.y + radius * 0.5f);
    ImVec2 usdt_pos = ImVec2(center.x + radius * 0.866f, center.y + radius * 0.5f);
    
    drawTriangleNode("BTC", btc_pos, 30.0f);
    drawTriangleNode("ETH", eth_pos, 30.0f);
    drawTriangleNode("USDT", usdt_pos, 30.0f);
    
    // Draw edges with profit indicators
    drawTriangleEdge(btc_pos, eth_pos, 8.5, true);
    drawTriangleEdge(eth_pos, usdt_pos, 12.3, true);
    drawTriangleEdge(usdt_pos, btc_pos, 6.2, false);
    
    ImGui::Dummy(canvas_size);
    
    // Legend
    ImGui::Separator();
    ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "🟢 Profitable Edge");
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "🔴 Unprofitable Edge");
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "🟡 Node Asset");
}

void AlgorithmWindow::renderCrossExchangeMatrix() {
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "🔄 Cross Exchange Matrix");
    
    drawExchangeMatrix();
}

void AlgorithmWindow::drawStrategyRow(const StrategyMetrics& metrics, int row_id) {
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::Text("%s", metrics.name.c_str());
    
    ImGui::TableSetColumnIndex(1);
    ImGui::TextColored(getStrategyStatusColor(metrics.status), "%s", metrics.status.c_str());
    
    ImGui::TableSetColumnIndex(2);
    ImGui::TextColored(getProfitColor(metrics.total_return), "%s", formatReturn(metrics.total_return).c_str());
    
    ImGui::TableSetColumnIndex(3);
    ImGui::TextColored(getProfitColor(metrics.daily_return), "%s", formatReturn(metrics.daily_return).c_str());
    
    ImGui::TableSetColumnIndex(4);
    ImGui::Text("%.1f%%", metrics.win_rate);
    
    ImGui::TableSetColumnIndex(5);
    ImGui::Text("%d", metrics.total_trades);
    
    ImGui::TableSetColumnIndex(6);
    ImGui::Text("%.2f", metrics.sharpe_ratio);
    
    ImGui::TableSetColumnIndex(7);
    ImGui::PushID(row_id);
    if (metrics.is_active) {
        if (ImGui::Button("Stop")) {
            stopStrategy(metrics.name);
        }
        ImGui::SameLine();
        if (ImGui::Button("Pause")) {
            pauseStrategy(metrics.name);
        }
    } else {
        if (ImGui::Button("Start")) {
            startStrategy(metrics.name);
        }
    }
    ImGui::PopID();
}

void AlgorithmWindow::drawOpportunityRow(const ArbitrageOpportunity& opp, int row_id) {
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::Text("%s", opp.type.c_str());
    
    ImGui::TableSetColumnIndex(1);
    ImGui::Text("%s", opp.symbol.c_str());
    
    ImGui::TableSetColumnIndex(2);
    ImGui::TextColored(getProfitColor(opp.profit_bps), "%s", formatBasisPoints(opp.profit_bps).c_str());
    
    ImGui::TableSetColumnIndex(3);
    ImGui::Text("$%.0f", opp.max_volume);
    
    ImGui::TableSetColumnIndex(4);
    ImGui::Text("%s", opp.description.c_str());
    
    ImGui::TableSetColumnIndex(5);
    ImGui::TextColored(opp.is_active ? ImVec4(0.0f, 1.0f, 0.0f, 1.0f) : ImVec4(1.0f, 0.0f, 0.0f, 1.0f), 
                       "%s", opp.is_active ? "Active" : "Inactive");
}

void AlgorithmWindow::drawTriangleNode(const std::string& asset, ImVec2 position, float radius) {
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    draw_list->AddCircleFilled(position, radius, IM_COL32(255, 255, 0, 255));
    draw_list->AddCircle(position, radius, IM_COL32(255, 255, 255, 255), 0, 2.0f);
    
    // Add text
    ImVec2 text_size = ImGui::CalcTextSize(asset.c_str());
    ImVec2 text_pos = ImVec2(position.x - text_size.x * 0.5f, position.y - text_size.y * 0.5f);
    draw_list->AddText(text_pos, IM_COL32(0, 0, 0, 255), asset.c_str());
}

void AlgorithmWindow::drawTriangleEdge(ImVec2 from, ImVec2 to, double profit_bps, bool highlight) {
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImU32 color = highlight ? IM_COL32(0, 255, 0, 255) : IM_COL32(255, 0, 0, 255);
    float thickness = highlight ? 3.0f : 1.0f;
    
    draw_list->AddLine(from, to, color, thickness);
    
    // Add profit label
    ImVec2 mid_point = ImVec2((from.x + to.x) * 0.5f, (from.y + to.y) * 0.5f);
    std::string profit_str = formatBasisPoints(profit_bps);
    ImVec2 text_size = ImGui::CalcTextSize(profit_str.c_str());
    ImVec2 text_pos = ImVec2(mid_point.x - text_size.x * 0.5f, mid_point.y - text_size.y * 0.5f);
    draw_list->AddText(text_pos, IM_COL32(255, 255, 255, 255), profit_str.c_str());
}

void AlgorithmWindow::drawExchangeMatrix() {
    std::vector<std::string> exchanges = {"Binance", "Coinbase", "Kraken", "FTX"};
    
    if (ImGui::BeginTable("ExchangeMatrix", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
        ImGui::TableSetupColumn("");
        for (const auto& exchange : exchanges) {
            ImGui::TableSetupColumn(exchange.c_str());
        }
        ImGui::TableHeadersRow();
        
        for (size_t i = 0; i < exchanges.size(); ++i) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%s", exchanges[i].c_str());
            
            for (size_t j = 0; j < exchanges.size(); ++j) {
                ImGui::TableSetColumnIndex(j + 1);
                if (i == j) {
                    ImGui::Text("-");
                } else {
                    double profit = ((rand() % 200) - 100) / 100.0; // Random profit
                    ImGui::TextColored(getProfitColor(profit), "%.1f bps", profit);
                }
            }
        }
        
        ImGui::EndTable();
    }
}

void AlgorithmWindow::drawPerformanceGauge(const std::string& strategy_name, double performance) {
    ImGui::Text("%s", strategy_name.c_str());
    
    // Simple progress bar as gauge
    float normalized = (performance + 50.0f) / 100.0f; // Normalize to 0-1
    normalized = std::max(0.0f, std::min(1.0f, normalized));
    
    ImGui::ProgressBar(normalized, ImVec2(-1.0f, 0.0f), formatReturn(performance).c_str());
}

void AlgorithmWindow::startStrategy(const std::string& strategy_name) {
    std::cout << "Starting strategy: " << strategy_name << std::endl;
    for (auto& metrics : strategy_metrics_) {
        if (metrics.name == strategy_name) {
            metrics.is_active = true;
            metrics.status = "Running";
            break;
        }
    }
    
    if (strategy_engine_) {
        // TODO: Actually start the strategy
        // strategy_engine_->startStrategy(strategy_name);
    }
}

void AlgorithmWindow::stopStrategy(const std::string& strategy_name) {
    std::cout << "Stopping strategy: " << strategy_name << std::endl;
    for (auto& metrics : strategy_metrics_) {
        if (metrics.name == strategy_name) {
            metrics.is_active = false;
            metrics.status = "Stopped";
            break;
        }
    }
    
    if (strategy_engine_) {
        // TODO: Actually stop the strategy
        // strategy_engine_->stopStrategy(strategy_name);
    }
}

void AlgorithmWindow::pauseStrategy(const std::string& strategy_name) {
    std::cout << "Pausing strategy: " << strategy_name << std::endl;
    for (auto& metrics : strategy_metrics_) {
        if (metrics.name == strategy_name) {
            metrics.is_active = false;
            metrics.status = "Paused";
            break;
        }
    }
    
    if (strategy_engine_) {
        // TODO: Actually pause the strategy
        // strategy_engine_->pauseStrategy(strategy_name);
    }
}

ImVec4 AlgorithmWindow::getProfitColor(double profit_bps) const {
    if (profit_bps > 0) {
        return ImVec4(0.0f, 1.0f, 0.0f, 1.0f); // Green for profit
    } else if (profit_bps < 0) {
        return ImVec4(1.0f, 0.0f, 0.0f, 1.0f); // Red for loss
    } else {
        return ImVec4(1.0f, 1.0f, 1.0f, 1.0f); // White for neutral
    }
}

ImVec4 AlgorithmWindow::getStrategyStatusColor(const std::string& status) const {
    if (status == "Running") {
        return ImVec4(0.0f, 1.0f, 0.0f, 1.0f); // Green
    } else if (status == "Paused") {
        return ImVec4(1.0f, 1.0f, 0.0f, 1.0f); // Yellow
    } else if (status == "Stopped") {
        return ImVec4(1.0f, 0.0f, 0.0f, 1.0f); // Red
    } else {
        return ImVec4(1.0f, 1.0f, 1.0f, 1.0f); // White
    }
}

std::string AlgorithmWindow::formatBasisPoints(double bps) const {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(1) << bps << " bps";
    return oss.str();
}

std::string AlgorithmWindow::formatReturn(double return_pct) const {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << (return_pct >= 0 ? "+" : "") << return_pct << "%";
    return oss.str();
}

} // namespace gui
} // namespace moneybot
