#pragma once

#include "base_window.h"
#include "risk_manager.h"
#include "core/portfolio_manager.h"
#include <memory>
#include <vector>
#include <string>
#include <map>
#include <chrono>
#include <imgui.h>

namespace moneybot {
class PortfolioManager;
namespace gui {

struct RiskMetrics {
    double portfolio_var_95 = 0.0;
    double portfolio_var_99 = 0.0;
    double max_drawdown = 0.0;
    double daily_volatility = 0.0;
    double correlation_risk = 0.0;
    double concentration_risk = 0.0;
    double leverage_ratio = 0.0;
    std::string risk_level = "Low"; // Low, Medium, High, Critical
};

struct RiskLimit {
    std::string name;
    double current_value = 0.0;
    double limit_value = 0.0;
    double utilization_pct = 0.0;
    bool is_breached = false;
    std::string severity = "Info"; // Info, Warning, Critical
};

struct RiskAlert {
    std::string type;
    std::string message;
    std::string severity;
    std::chrono::system_clock::time_point timestamp;
    bool acknowledged = false;
};

class RiskWindow : public BaseWindow {
private:
    std::shared_ptr<RiskManager> risk_manager_;
    std::shared_ptr<PortfolioManager> portfolio_manager_;
    
    // UI state
    bool show_risk_metrics_ = true;
    bool show_risk_limits_ = true;
    bool show_risk_alerts_ = true;
    bool show_risk_heatmap_ = true;
    bool show_correlation_matrix_ = true;
    
    // Data
    RiskMetrics risk_metrics_;
    std::vector<RiskLimit> risk_limits_;
    std::vector<RiskAlert> risk_alerts_;
    std::map<std::string, std::map<std::string, double>> correlation_matrix_;
    
    // Chart data
    std::vector<float> var_history_;
    std::vector<float> drawdown_history_;
    
    // Emergency controls
    bool emergency_stop_active_ = false;
    
public:
    RiskWindow(std::shared_ptr<RiskManager> risk_manager, 
               std::shared_ptr<PortfolioManager> portfolio_manager);
    ~RiskWindow() override = default;
    
    void render() override;
    void updateData();
    
    // Alert management
    void acknowledgeAlert(int alert_id);
    void clearAllAlerts();
    
private:
    void renderRiskMetrics();
    void renderRiskLimits();
    void renderRiskAlerts();
    void renderRiskHeatmap();
    void renderCorrelationMatrix();
    void renderEmergencyControls();
    
    // Risk visualization
    void drawRiskGauge(const std::string& label, double value, double max_value, 
                      ImVec4 color, ImVec2 size);
    void drawRiskMeter(double risk_level);
    void drawHeatmapCell(const std::string& asset, double risk_value, 
                        ImVec2 position, ImVec2 size);
    
    // Helper functions
    void drawRiskLimitRow(const RiskLimit& limit, int row_id);
    void drawRiskAlertRow(const RiskAlert& alert, int row_id);
    void updateRiskMetrics();
    void updateRiskLimits();
    void checkRiskAlerts();
    
    // Color coding
    ImVec4 getRiskLevelColor(const std::string& level) const;
    ImVec4 getSeverityColor(const std::string& severity) const;
    ImVec4 getUtilizationColor(double utilization_pct) const;
    
    // Formatting helpers
    std::string formatRiskValue(double value) const;
    std::string formatPercentage(double value) const;
    std::string formatTime(const std::chrono::system_clock::time_point& time) const;
    
    // Emergency actions
    void triggerEmergencyStop();
    void cancelEmergencyStop();
};

} // namespace gui
} // namespace moneybot
