#pragma once

#include <memory>
#include <vector>
#include <string>
#include <map>
#include <imgui.h>

namespace moneybot {

// Forward declarations
class StrategyEngine;
class Strategy;

namespace gui {

struct StrategyMetrics {
    std::string name;
    bool is_active = false;
    double total_return = 0.0;
    double daily_return = 0.0;
    double win_rate = 0.0;
    int total_trades = 0;
    double max_drawdown = 0.0;
    double sharpe_ratio = 0.0;
    std::string status = "Stopped";
};

struct ArbitrageOpportunity {
    std::string type; // "triangle" or "cross_exchange"
    std::string symbol;
    double profit_bps = 0.0;
    double max_volume = 0.0;
    std::string description;
    bool is_active = false;
};

class AlgorithmWindow {
private:
    std::shared_ptr<StrategyEngine> strategy_engine_;
    
    // UI state
    bool show_strategy_control_ = true;
    bool show_opportunities_ = true;
    bool show_performance_ = true;
    bool show_triangle_network_ = true;
    bool show_cross_exchange_matrix_ = true;
    
    // Data
    std::vector<StrategyMetrics> strategy_metrics_;
    std::vector<ArbitrageOpportunity> opportunities_;
    
    // Chart data
    std::vector<float> strategy_performance_;
    std::vector<float> opportunity_profits_;
    
    // Selected strategy for detailed view
    std::string selected_strategy_ = "";
    
public:
    explicit AlgorithmWindow(std::shared_ptr<StrategyEngine> strategy_engine);
    ~AlgorithmWindow() = default;
    
    void render();
    void updateData();
    
private:
    void renderStrategyControl();
    void renderOpportunitiesTable();
    void renderPerformanceCharts();
    void renderTriangleNetwork();
    void renderCrossExchangeMatrix();
    
    // Strategy control functions
    void startStrategy(const std::string& strategy_name);
    void stopStrategy(const std::string& strategy_name);
    void pauseStrategy(const std::string& strategy_name);
    
    // Visualization functions
    void drawTriangleNode(const std::string& asset, ImVec2 position, float radius);
    void drawTriangleEdge(ImVec2 from, ImVec2 to, double profit_bps, bool highlight = false);
    void drawExchangeMatrix();
    void drawPerformanceGauge(const std::string& strategy_name, double performance);
    
    // Helper functions
    void drawStrategyRow(const StrategyMetrics& metrics, int row_id);
    void drawOpportunityRow(const ArbitrageOpportunity& opp, int row_id);
    ImVec4 getProfitColor(double profit_bps) const;
    ImVec4 getStrategyStatusColor(const std::string& status) const;
    
    // Formatting helpers
    std::string formatBasisPoints(double bps) const;
    std::string formatReturn(double return_pct) const;
};

} // namespace gui
} // namespace moneybot
