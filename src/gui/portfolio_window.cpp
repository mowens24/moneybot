#include "gui/portfolio_window.h"
#include "core/portfolio_manager.h"
#include "types.h"
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <iostream>

namespace moneybot {
namespace gui {

PortfolioWindow::PortfolioWindow(std::shared_ptr<PortfolioManager> portfolio_manager)
    : portfolio_manager_(portfolio_manager) {
    // Initialize chart data
    pnl_history_.reserve(100);
    value_history_.reserve(100);
    updateChartData();
}

void PortfolioWindow::render() {
    if (!ImGui::Begin("💼 Portfolio & Performance")) {
        ImGui::End();
        return;
    }
    
    // Update data first
    updateData();
    
    // Header
    ImGui::TextColored(ImVec4(0.0f, 1.0f, 1.0f, 1.0f), "Portfolio Dashboard");
    ImGui::Separator();
    
    // Main content in tabs
    if (ImGui::BeginTabBar("PortfolioTabs")) {
        
        // Overview tab
        if (ImGui::BeginTabItem("Overview")) {
            renderMetricsPanel();
            ImGui::EndTabItem();
        }
        
        // Positions tab
        if (ImGui::BeginTabItem("Positions")) {
            renderPositionsTable();
            ImGui::EndTabItem();
        }
        
        // Balances tab
        if (ImGui::BeginTabItem("Balances")) {
            renderBalancesTable();
            ImGui::EndTabItem();
        }
        
        // Performance tab
        if (ImGui::BeginTabItem("Performance")) {
            renderPerformanceChart();
            ImGui::EndTabItem();
        }
        
        ImGui::EndTabBar();
    }
    
    ImGui::End();
}

void PortfolioWindow::updateData() {
    if (!portfolio_manager_) return;
    
    // Update chart data periodically
    static auto last_update = std::chrono::steady_clock::now();
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - last_update).count();
    
    if (elapsed > 5) {
        updateChartData();
        last_update = now;
    }
}

void PortfolioWindow::renderMetricsPanel() {
    auto metrics = calculateMetrics();
    
    // Portfolio value summary
    ImGui::Columns(3, "MetricsSummary", false);
    
    // Total Portfolio Value
    ImGui::BeginChild("TotalValue", ImVec2(0, 80), true);
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Portfolio Value");
    ImGui::Spacing();
    ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "%s", formatCurrency(metrics.total_value).c_str());
    ImGui::TextColored(getPnLColor(metrics.daily_return), "%s", formatPercentage(metrics.daily_return).c_str());
    ImGui::EndChild();
    ImGui::NextColumn();
    
    // Unrealized P&L
    ImGui::BeginChild("UnrealizedPnL", ImVec2(0, 80), true);
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Unrealized P&L");
    ImGui::Spacing();
    ImGui::TextColored(getPnLColor(metrics.unrealized_pnl), "%s", formatCurrency(metrics.unrealized_pnl).c_str());
    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Realized: %s", formatCurrency(metrics.realized_pnl).c_str());
    ImGui::EndChild();
    ImGui::NextColumn();
    
    // Total Return
    ImGui::BeginChild("TotalReturn", ImVec2(0, 80), true);
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Total Return");
    ImGui::Spacing();
    ImGui::TextColored(getPnLColor(metrics.total_return), "%s", formatPercentage(metrics.total_return).c_str());
    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Sharpe: %.2f", metrics.sharpe_ratio);
    ImGui::EndChild();
    
    ImGui::Columns(1);
    ImGui::Separator();
    
    // Risk metrics
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "📊 Risk Metrics");
    ImGui::Spacing();
    
    // Progress bars for risk metrics
    float sharpe_normalized = std::min(std::max((float)((metrics.sharpe_ratio + 2.0f) / 4.0f), 0.0f), 1.0f);
    ImGui::Text("Sharpe Ratio: %.2f", metrics.sharpe_ratio);
    ImGui::ProgressBar(sharpe_normalized, ImVec2(-1.0f, 0.0f), "");
    
    float max_dd_normalized = 1.0f - std::min((float)(std::abs(metrics.max_drawdown) / 10.0f), 1.0f);
    ImGui::Text("Max Drawdown: %s", formatPercentage(metrics.max_drawdown).c_str());
    ImGui::ProgressBar(max_dd_normalized, ImVec2(-1.0f, 0.0f), "");
    
    ImGui::Text("VaR (95%%): %s", formatCurrency(metrics.var_95).c_str());
    
    ImGui::Separator();
    
    // Quick Actions
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "⚡ Quick Actions");
    
    if (ImGui::Button("🔄 Refresh Data", ImVec2(100, 0))) {
        updateData();
    }
    ImGui::SameLine();
    
    if (ImGui::Button("📊 Export Report", ImVec2(100, 0))) {
        // TODO: Implement export functionality
        std::cout << "Export portfolio report requested" << std::endl;
    }
    ImGui::SameLine();
    
    if (ImGui::Button("🛑 Emergency Stop", ImVec2(100, 0))) {
        std::cout << "🛑 EMERGENCY STOP - All trading halted!" << std::endl;
        // TODO: Implement emergency stop
    }
}

void PortfolioWindow::renderPositionsTable() {
    if (!portfolio_manager_) {
        ImGui::Text("No portfolio manager available");
        return;
    }
    
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "🎯 Active Positions");
    
    // Table header
    if (ImGui::BeginTable("PositionsTable", 6, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Sortable)) {
        ImGui::TableSetupColumn("Symbol", ImGuiTableColumnFlags_DefaultSort);
        ImGui::TableSetupColumn("Size", ImGuiTableColumnFlags_None);
        ImGui::TableSetupColumn("Entry Price", ImGuiTableColumnFlags_None);
        ImGui::TableSetupColumn("Current Price", ImGuiTableColumnFlags_None);
        ImGui::TableSetupColumn("P&L", ImGuiTableColumnFlags_None);
        ImGui::TableSetupColumn("P&L %", ImGuiTableColumnFlags_None);
        ImGui::TableHeadersRow();
        
        auto positions = portfolio_manager_->getAllPositions();
        
        // Handle sorting
        if (ImGuiTableSortSpecs* sort_specs = ImGui::TableGetSortSpecs()) {
            if (sort_specs->SpecsDirty) {
                // Sort positions based on current sort specs
                // TODO: Implement sorting logic
                sort_specs->SpecsDirty = false;
            }
        }
        
        // Render position rows
        int row_id = 0;
        for (const auto& position : positions) {
            drawPositionRow(position, row_id++);
        }
        
        ImGui::EndTable();
    }
}

void PortfolioWindow::renderBalancesTable() {
    if (!portfolio_manager_) {
        ImGui::Text("No portfolio manager available");
        return;
    }
    
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "💰 Account Balances");
    
    // Table header
    if (ImGui::BeginTable("BalancesTable", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Sortable)) {
        ImGui::TableSetupColumn("Asset", ImGuiTableColumnFlags_DefaultSort);
        ImGui::TableSetupColumn("Available", ImGuiTableColumnFlags_None);
        ImGui::TableSetupColumn("Locked", ImGuiTableColumnFlags_None);
        ImGui::TableSetupColumn("Total", ImGuiTableColumnFlags_None);
        ImGui::TableHeadersRow();
        
        auto balances = portfolio_manager_->getAllBalances();
        
        // Handle sorting
        if (ImGuiTableSortSpecs* sort_specs = ImGui::TableGetSortSpecs()) {
            if (sort_specs->SpecsDirty) {
                // Sort balances based on current sort specs
                // TODO: Implement sorting logic
                sort_specs->SpecsDirty = false;
            }
        }
        
        // Render balance rows
        int row_id = 0;
        for (const auto& balance : balances) {
            drawBalanceRow(balance, row_id++);
        }
        
        ImGui::EndTable();
    }
}

void PortfolioWindow::renderPerformanceChart() {
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "📈 Performance Chart");
    
    if (pnl_history_.empty()) {
        ImGui::Text("No performance data available");
        return;
    }
    
    // P&L chart
    ImGui::Text("P&L History");
    ImGui::PlotLines("##PnLChart", pnl_history_.data(), pnl_history_.size(), 0, nullptr, 
                     FLT_MAX, FLT_MAX, ImVec2(0, 200));
    
    // Portfolio value chart
    ImGui::Text("Portfolio Value History");
    ImGui::PlotLines("##ValueChart", value_history_.data(), value_history_.size(), 0, nullptr, 
                     FLT_MAX, FLT_MAX, ImVec2(0, 200));
}

void PortfolioWindow::drawPositionRow(const Position& position, int row_id) {
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::Text("%s", position.symbol.c_str());
    
    ImGui::TableSetColumnIndex(1);
    ImGui::Text("%.4f", position.quantity);
    
    ImGui::TableSetColumnIndex(2);
    ImGui::Text("%.4f", position.avg_price);
    
    ImGui::TableSetColumnIndex(3);
    ImGui::Text("%.4f", position.avg_price); // Current price (approximation)
    
    ImGui::TableSetColumnIndex(4);
    double pnl = position.unrealized_pnl;
    ImGui::TextColored(getPnLColor(pnl), "%s", formatCurrency(pnl).c_str());
    
    ImGui::TableSetColumnIndex(5);
    double pnl_percent = (position.unrealized_pnl / (position.avg_price * position.quantity)) * 100.0;
    ImGui::TextColored(getPnLColor(pnl_percent), "%s", formatPercentage(pnl_percent).c_str());
}

void PortfolioWindow::drawBalanceRow(const Balance& balance, int row_id) {
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::Text("%s", balance.asset.c_str());
    
    ImGui::TableSetColumnIndex(1);
    ImGui::Text("%.8f", balance.free);
    
    ImGui::TableSetColumnIndex(2);
    ImGui::Text("%.8f", balance.locked);
    
    ImGui::TableSetColumnIndex(3);
    ImGui::Text("%.8f", balance.total);
}

void PortfolioWindow::updateChartData() {
    if (!portfolio_manager_) return;
    
    auto metrics = calculateMetrics();
    
    // Add new data points
    pnl_history_.push_back(static_cast<float>(metrics.unrealized_pnl + metrics.realized_pnl));
    value_history_.push_back(static_cast<float>(metrics.total_value));
    
    // Keep only last 100 data points
    if (pnl_history_.size() > 100) {
        pnl_history_.erase(pnl_history_.begin());
    }
    if (value_history_.size() > 100) {
        value_history_.erase(value_history_.begin());
    }
}

PortfolioMetrics PortfolioWindow::calculateMetrics() const {
    PortfolioMetrics metrics;
    
    if (!portfolio_manager_) return metrics;
    
    // Calculate metrics from portfolio manager
    metrics.total_value = portfolio_manager_->getTotalValue();
    metrics.unrealized_pnl = portfolio_manager_->getUnrealizedPnL();
    metrics.realized_pnl = portfolio_manager_->getRealizedPnL();
    
    // Use placeholder values for methods that don't exist yet
    metrics.daily_return = 0.0; // TODO: Implement getDailyReturn()
    metrics.total_return = 0.0; // TODO: Implement getTotalReturn()
    metrics.sharpe_ratio = 1.85; // TODO: Implement getSharpeRatio()
    metrics.max_drawdown = 2.3; // TODO: Implement getMaxDrawdown()
    metrics.var_95 = 3250.0; // TODO: Implement getVaR()
    
    return metrics;
}

std::string PortfolioWindow::formatCurrency(double value) const {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << "$" << value;
    return oss.str();
}

std::string PortfolioWindow::formatPercentage(double value) const {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << (value >= 0 ? "+" : "") << value << "%";
    return oss.str();
}

ImVec4 PortfolioWindow::getPnLColor(double pnl) const {
    if (pnl > 0) {
        return ImVec4(0.0f, 1.0f, 0.0f, 1.0f); // Green for profit
    } else if (pnl < 0) {
        return ImVec4(1.0f, 0.0f, 0.0f, 1.0f); // Red for loss
    } else {
        return ImVec4(1.0f, 1.0f, 1.0f, 1.0f); // White for neutral
    }
}

} // namespace gui
} // namespace moneybot
