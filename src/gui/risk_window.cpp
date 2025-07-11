#include "gui/risk_window.h"
#include <imgui.h>
#include <chrono>

namespace moneybot {
namespace gui {

RiskWindow::RiskWindow(std::shared_ptr<RiskManager> risk_manager, 
                       std::shared_ptr<PortfolioManager> portfolio_manager)
    : BaseWindow("Risk Management")
    , risk_manager_(risk_manager)
    , portfolio_manager_(portfolio_manager) {
    
    // Initialize default risk limits
    risk_limits_.push_back({"Max Position Size", 0.0, 100000.0, 0.0, false, "Info"});
    risk_limits_.push_back({"Daily Loss Limit", 0.0, 5000.0, 0.0, false, "Info"});
    risk_limits_.push_back({"Max Drawdown", 0.0, 10.0, 0.0, false, "Info"});
    risk_limits_.push_back({"Leverage Ratio", 0.0, 3.0, 0.0, false, "Info"});
}

void RiskWindow::render() {
    if (!is_visible_) return;
    
    ImGui::Begin("Risk Management", &is_visible_);
    
    // Risk overview at the top
    ImGui::Text("Risk Overview");
    ImGui::Separator();
    
    // Risk metrics section
    if (show_risk_metrics_) {
        renderRiskMetrics();
    }
    
    // Risk limits section
    if (show_risk_limits_) {
        renderRiskLimits();
    }
    
    // Risk alerts section
    if (show_risk_alerts_) {
        renderRiskAlerts();
    }
    
    // Emergency controls
    renderEmergencyControls();
    
    ImGui::End();
}

void RiskWindow::updateData() {
    // Update risk metrics from risk manager
    if (risk_manager_) {
        // TODO: Implement actual risk calculation
        // For now, use placeholder values
        risk_metrics_.portfolio_var_95 = 1250.0;
        risk_metrics_.portfolio_var_99 = 2100.0;
        risk_metrics_.max_drawdown = 3.5;
        risk_metrics_.daily_volatility = 1.8;
        risk_metrics_.correlation_risk = 0.65;
        risk_metrics_.concentration_risk = 0.25;
        risk_metrics_.leverage_ratio = 1.5;
        risk_metrics_.risk_level = "Medium";
    }
    
    // Update risk limits
    updateRiskLimits();
    
    // Check for new alerts
    checkRiskAlerts();
}

void RiskWindow::renderRiskMetrics() {
    ImGui::Text("Risk Metrics");
    ImGui::Separator();
    
    ImGui::Columns(2, "RiskMetrics", true);
    
    // VaR metrics
    ImGui::Text("Portfolio VaR (95%%):");
    ImGui::NextColumn();
    ImGui::Text("$%.2f", risk_metrics_.portfolio_var_95);
    ImGui::NextColumn();
    
    ImGui::Text("Portfolio VaR (99%%):");
    ImGui::NextColumn();
    ImGui::Text("$%.2f", risk_metrics_.portfolio_var_99);
    ImGui::NextColumn();
    
    // Drawdown
    ImGui::Text("Max Drawdown:");
    ImGui::NextColumn();
    ImGui::Text("%.2f%%", risk_metrics_.max_drawdown);
    ImGui::NextColumn();
    
    // Volatility
    ImGui::Text("Daily Volatility:");
    ImGui::NextColumn();
    ImGui::Text("%.2f%%", risk_metrics_.daily_volatility);
    ImGui::NextColumn();
    
    // Risk level
    ImGui::Text("Overall Risk Level:");
    ImGui::NextColumn();
    ImVec4 color = getRiskLevelColor(risk_metrics_.risk_level);
    ImGui::TextColored(color, "%s", risk_metrics_.risk_level.c_str());
    ImGui::NextColumn();
    
    ImGui::Columns(1);
    ImGui::Spacing();
}

void RiskWindow::renderRiskLimits() {
    ImGui::Text("Risk Limits");
    ImGui::Separator();
    
    ImGui::Columns(4, "RiskLimits", true);
    ImGui::Text("Limit");
    ImGui::NextColumn();
    ImGui::Text("Current");
    ImGui::NextColumn();
    ImGui::Text("Limit");
    ImGui::NextColumn();
    ImGui::Text("Utilization");
    ImGui::NextColumn();
    ImGui::Separator();
    
    for (size_t i = 0; i < risk_limits_.size(); ++i) {
        drawRiskLimitRow(risk_limits_[i], i);
    }
    
    ImGui::Columns(1);
    ImGui::Spacing();
}

void RiskWindow::renderRiskAlerts() {
    ImGui::Text("Risk Alerts");
    ImGui::Separator();
    
    if (risk_alerts_.empty()) {
        ImGui::Text("No active alerts");
    } else {
        for (size_t i = 0; i < risk_alerts_.size(); ++i) {
            drawRiskAlertRow(risk_alerts_[i], i);
        }
    }
    
    ImGui::Spacing();
}

void RiskWindow::renderEmergencyControls() {
    ImGui::Text("Emergency Controls");
    ImGui::Separator();
    
    if (emergency_stop_active_) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
        if (ImGui::Button("Cancel Emergency Stop")) {
            cancelEmergencyStop();
        }
        ImGui::PopStyleColor();
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.2f, 1.0f), "EMERGENCY STOP ACTIVE");
    } else {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.3f, 0.3f, 1.0f));
        if (ImGui::Button("Emergency Stop")) {
            triggerEmergencyStop();
        }
        ImGui::PopStyleColor();
    }
}

void RiskWindow::drawRiskLimitRow(const RiskLimit& limit, int row_id) {
    ImGui::Text("%s", limit.name.c_str());
    ImGui::NextColumn();
    ImGui::Text("%.2f", limit.current_value);
    ImGui::NextColumn();
    ImGui::Text("%.2f", limit.limit_value);
    ImGui::NextColumn();
    
    ImVec4 color = getUtilizationColor(limit.utilization_pct);
    ImGui::TextColored(color, "%.1f%%", limit.utilization_pct);
    ImGui::NextColumn();
}

void RiskWindow::drawRiskAlertRow(const RiskAlert& alert, int row_id) {
    ImVec4 color = getSeverityColor(alert.severity);
    ImGui::TextColored(color, "[%s] %s: %s", 
                      alert.severity.c_str(), 
                      alert.type.c_str(), 
                      alert.message.c_str());
    
    ImGui::SameLine();
    std::string button_id = "Ack##" + std::to_string(row_id);
    if (ImGui::SmallButton(button_id.c_str())) {
        // TODO: Implement acknowledge alert
    }
}

void RiskWindow::updateRiskLimits() {
    // TODO: Update risk limits based on current portfolio state
    // For now, use placeholder calculations
    for (auto& limit : risk_limits_) {
        if (limit.name == "Max Position Size") {
            limit.current_value = 45000.0; // Placeholder
            limit.utilization_pct = (limit.current_value / limit.limit_value) * 100.0;
        } else if (limit.name == "Daily Loss Limit") {
            limit.current_value = 1200.0; // Placeholder
            limit.utilization_pct = (limit.current_value / limit.limit_value) * 100.0;
        }
        // Add more limit calculations as needed
        
        limit.is_breached = limit.utilization_pct > 100.0;
    }
}

void RiskWindow::checkRiskAlerts() {
    // Clear old alerts (for now, just clear all)
    risk_alerts_.clear();
    
    // Check for new alerts based on risk limits
    for (const auto& limit : risk_limits_) {
        if (limit.is_breached) {
            RiskAlert alert;
            alert.type = "Limit Breach";
            alert.message = limit.name + " has been breached";
            alert.severity = "Critical";
            alert.timestamp = std::chrono::system_clock::now();
            alert.acknowledged = false;
            risk_alerts_.push_back(alert);
        }
    }
}

void RiskWindow::acknowledgeAlert(int alert_id) {
    if (alert_id >= 0 && alert_id < static_cast<int>(risk_alerts_.size())) {
        risk_alerts_[alert_id].acknowledged = true;
    }
}

void RiskWindow::clearAllAlerts() {
    risk_alerts_.clear();
}

void RiskWindow::triggerEmergencyStop() {
    emergency_stop_active_ = true;
    // TODO: Implement actual emergency stop logic
}

void RiskWindow::cancelEmergencyStop() {
    emergency_stop_active_ = false;
    // TODO: Implement cancel emergency stop logic
}

ImVec4 RiskWindow::getRiskLevelColor(const std::string& level) const {
    if (level == "Low") return ImVec4(0.0f, 1.0f, 0.0f, 1.0f);      // Green
    if (level == "Medium") return ImVec4(1.0f, 1.0f, 0.0f, 1.0f);   // Yellow
    if (level == "High") return ImVec4(1.0f, 0.5f, 0.0f, 1.0f);     // Orange
    if (level == "Critical") return ImVec4(1.0f, 0.0f, 0.0f, 1.0f); // Red
    return ImVec4(1.0f, 1.0f, 1.0f, 1.0f); // White (default)
}

ImVec4 RiskWindow::getSeverityColor(const std::string& severity) const {
    if (severity == "Info") return ImVec4(0.0f, 0.8f, 1.0f, 1.0f);     // Light blue
    if (severity == "Warning") return ImVec4(1.0f, 1.0f, 0.0f, 1.0f);  // Yellow
    if (severity == "Critical") return ImVec4(1.0f, 0.0f, 0.0f, 1.0f); // Red
    return ImVec4(1.0f, 1.0f, 1.0f, 1.0f); // White (default)
}

ImVec4 RiskWindow::getUtilizationColor(double utilization_pct) const {
    if (utilization_pct < 50.0) return ImVec4(0.0f, 1.0f, 0.0f, 1.0f);     // Green
    if (utilization_pct < 75.0) return ImVec4(1.0f, 1.0f, 0.0f, 1.0f);     // Yellow
    if (utilization_pct < 100.0) return ImVec4(1.0f, 0.5f, 0.0f, 1.0f);    // Orange
    return ImVec4(1.0f, 0.0f, 0.0f, 1.0f); // Red
}

} // namespace gui
} // namespace moneybot
