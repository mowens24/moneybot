#pragma once

#include <memory>
#include <vector>
#include <string>
#include <imgui.h>
#include "types.h"  // Include types to have Position and Balance

namespace moneybot {

// Forward declarations
class PortfolioManager;

namespace gui {

struct PortfolioMetrics {
    double total_value = 0.0;
    double unrealized_pnl = 0.0;
    double realized_pnl = 0.0;
    double daily_return = 0.0;
    double total_return = 0.0;
    double sharpe_ratio = 0.0;
    double max_drawdown = 0.0;
    double var_95 = 0.0;
};

class PortfolioWindow {
private:
    std::shared_ptr<PortfolioManager> portfolio_manager_;
    
    // UI state
    bool show_positions_table_ = true;
    bool show_balances_table_ = true;
    bool show_metrics_panel_ = true;
    bool show_performance_chart_ = true;
    
    // Table sorting
    int positions_sort_column_ = 0;
    bool positions_sort_ascending_ = true;
    int balances_sort_column_ = 0;
    bool balances_sort_ascending_ = true;
    
    // Chart data
    std::vector<float> pnl_history_;
    std::vector<float> value_history_;
    
public:
    explicit PortfolioWindow(std::shared_ptr<PortfolioManager> portfolio_manager);
    ~PortfolioWindow() = default;
    
    void render();
    void updateData();
    
private:
    void renderPositionsTable();
    void renderBalancesTable();
    void renderMetricsPanel();
    void renderPerformanceChart();
    
    // Helper functions
    void drawPositionRow(const Position& position, int row_id);
    void drawBalanceRow(const Balance& balance, int row_id);
    void updateChartData();
    PortfolioMetrics calculateMetrics() const;
    
    // Formatting helpers
    std::string formatCurrency(double value) const;
    std::string formatPercentage(double value) const;
    ImVec4 getPnLColor(double pnl) const;
};

} // namespace gui
} // namespace moneybot
