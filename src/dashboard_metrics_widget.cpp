#include "dashboard_metrics_widget.h"
#include <imgui.h>
#include <iomanip>
#include <sstream>

namespace moneybot {

void RenderMetricsWidget(const nlohmann::json& metrics, const nlohmann::json& status) {
    ImGui::BeginChild("Performance Metrics", ImVec2(0, 200), true);
    
    ImGui::Text("Performance Metrics");
    ImGui::Separator();
    
    if (!metrics.is_null()) {
        double pnl = metrics.value("pnl", 0.0);
        ImGui::Text("P&L: "); ImGui::SameLine();
        ImGui::TextColored(pnl >= 0 ? ImVec4(0.2f, 0.8f, 0.2f, 1.0f) : ImVec4(0.8f, 0.2f, 0.2f, 1.0f), 
                          "%.2f", pnl);
        ImGui::Text("Position: %.4f", metrics.value("position", 0.0));
        ImGui::Text("Fill Rate: %.2f%%", metrics.value("fill_rate", 0.0) * 100.0);
        ImGui::Text("Orders/sec: %.1f", metrics.value("orders_per_sec", 0.0));
    }
    
    ImGui::Separator();
    ImGui::Text("Status");
    
    if (!status.is_null()) {
        ImGui::Text("Trading: %s", status.value("trading_enabled", false) ? "Enabled" : "Disabled");
        ImGui::Text("Risk Check: %s", status.value("risk_check_passed", false) ? "Passed" : "Failed");
        ImGui::Text("Connection: %s", status.value("connected", false) ? "Connected" : "Disconnected");
        ImGui::Text("Latency: %.2fms", status.value("latency_ms", 0.0));
        if (status.contains("uptime_seconds")) {
            ImGui::Text("Uptime: %llds", status.value("uptime_seconds", 0ll));
        }
    }
    
    ImGui::EndChild();
}

} // namespace moneybot
