#include "gui/exchange_window.h"
#include <imgui.h>
#include <chrono>

namespace moneybot {
namespace gui {

ExchangeWindow::ExchangeWindow(std::shared_ptr<ExchangeManager> exchange_manager)
    : BaseWindow("Exchange Management")
    , exchange_manager_(exchange_manager) {
    
    // Initialize with some dummy exchange data
    ExchangeStatus binance_status;
    binance_status.name = "Binance";
    binance_status.is_connected = false;
    binance_status.status = "Disconnected";
    binance_status.latency_ms = 0.0;
    binance_status.active_orders = 0;
    binance_status.daily_volume = 0.0;
    binance_status.last_update = std::chrono::system_clock::now();
    binance_status.health = "Unknown";
    exchange_statuses_.push_back(binance_status);
    
    ExchangeStatus coinbase_status;
    coinbase_status.name = "Coinbase Pro";
    coinbase_status.is_connected = false;
    coinbase_status.status = "Disconnected";
    coinbase_status.latency_ms = 0.0;
    coinbase_status.active_orders = 0;
    coinbase_status.daily_volume = 0.0;
    coinbase_status.last_update = std::chrono::system_clock::now();
    coinbase_status.health = "Unknown";
    exchange_statuses_.push_back(coinbase_status);
}

void ExchangeWindow::render() {
    if (!is_visible_) return;
    
    ImGui::Begin("Exchange Management", &is_visible_);
    
    // Exchange overview at the top
    ImGui::Text("Exchange Overview");
    ImGui::Separator();
    
    // Exchange status section
    if (show_exchange_status_) {
        renderExchangeStatus();
    }
    
    // Exchange metrics section
    if (show_exchange_metrics_) {
        renderExchangeMetrics();
    }
    
    // Connection logs section
    if (show_connection_logs_) {
        renderConnectionLogs();
    }
    
    // Exchange controls
    renderExchangeControls();
    
    ImGui::End();
}

void ExchangeWindow::updateData() {
    // Update exchange statuses from exchange manager
    if (exchange_manager_) {
        // TODO: Implement actual exchange status updates
        // For now, use placeholder data
        for (auto& status : exchange_statuses_) {
            if (status.name == "Binance") {
                status.is_connected = true;
                status.status = "Connected";
                status.latency_ms = 45.2;
                status.active_orders = 3;
                status.daily_volume = 125000.0;
                status.health = "Healthy";
            } else if (status.name == "Coinbase Pro") {
                status.is_connected = false;
                status.status = "Connection Error";
                status.latency_ms = 0.0;
                status.active_orders = 0;
                status.daily_volume = 0.0;
                status.health = "Critical";
            }
            status.last_update = std::chrono::system_clock::now();
        }
    }
    
    // Update connection status
    updateConnectionStatus();
    
    // Update latency and volume data
    updateLatencyData();
    updateVolumeData();
}

void ExchangeWindow::renderExchangeStatus() {
    ImGui::Text("Exchange Status");
    ImGui::Separator();
    
    ImGui::Columns(6, "ExchangeStatus", true);
    ImGui::Text("Exchange");
    ImGui::NextColumn();
    ImGui::Text("Status");
    ImGui::NextColumn();
    ImGui::Text("Latency");
    ImGui::NextColumn();
    ImGui::Text("Orders");
    ImGui::NextColumn();
    ImGui::Text("Volume");
    ImGui::NextColumn();
    ImGui::Text("Health");
    ImGui::NextColumn();
    ImGui::Separator();
    
    for (size_t i = 0; i < exchange_statuses_.size(); ++i) {
        drawExchangeStatusRow(exchange_statuses_[i], i);
    }
    
    ImGui::Columns(1);
    ImGui::Spacing();
}

void ExchangeWindow::renderExchangeMetrics() {
    ImGui::Text("Exchange Metrics");
    ImGui::Separator();
    
    if (exchange_metrics_.empty()) {
        ImGui::Text("No metrics available");
    } else {
        ImGui::Columns(5, "ExchangeMetrics", true);
        ImGui::Text("Exchange");
        ImGui::NextColumn();
        ImGui::Text("Total Volume");
        ImGui::NextColumn();
        ImGui::Text("Orders");
        ImGui::NextColumn();
        ImGui::Text("Success Rate");
        ImGui::NextColumn();
        ImGui::Text("Avg Latency");
        ImGui::NextColumn();
        ImGui::Separator();
        
        for (size_t i = 0; i < exchange_metrics_.size(); ++i) {
            drawExchangeMetricsRow(exchange_metrics_[i], i);
        }
        
        ImGui::Columns(1);
    }
    
    ImGui::Spacing();
}

void ExchangeWindow::renderConnectionLogs() {
    ImGui::Text("Connection Logs");
    ImGui::Separator();
    
    // Log filter
    ImGui::Text("Filter:");
    ImGui::SameLine();
    char filter_buffer[256];
    strncpy(filter_buffer, log_filter_.c_str(), sizeof(filter_buffer));
    if (ImGui::InputText("##LogFilter", filter_buffer, sizeof(filter_buffer))) {
        log_filter_ = std::string(filter_buffer);
    }
    
    ImGui::SameLine();
    ImGui::Checkbox("Auto-scroll", &auto_scroll_logs_);
    
    // Log content
    ImGui::BeginChild("LogScrollRegion", ImVec2(0, 150), true);
    
    if (connection_logs_.empty()) {
        ImGui::Text("No logs available");
    } else {
        for (const auto& log : connection_logs_) {
            if (log_filter_.empty() || log.find(log_filter_) != std::string::npos) {
                ImGui::Text("%s", log.c_str());
            }
        }
        
        if (auto_scroll_logs_) {
            ImGui::SetScrollHereY(1.0f);
        }
    }
    
    ImGui::EndChild();
    ImGui::Spacing();
}

void ExchangeWindow::renderExchangeControls() {
    ImGui::Text("Exchange Controls");
    ImGui::Separator();
    
    if (ImGui::Button("Connect All")) {
        connectAllExchanges();
    }
    ImGui::SameLine();
    if (ImGui::Button("Disconnect All")) {
        disconnectAllExchanges();
    }
    
    ImGui::Spacing();
    
    // Individual exchange controls
    for (const auto& status : exchange_statuses_) {
        drawConnectionButton(status.name, status.is_connected);
        ImGui::SameLine();
    }
}

void ExchangeWindow::drawExchangeStatusRow(const ExchangeStatus& status, int row_id) {
    // Exchange name
    ImGui::Text("%s", status.name.c_str());
    ImGui::NextColumn();
    
    // Status with color coding
    ImVec4 status_color = getStatusColor(status.status);
    ImGui::TextColored(status_color, "%s", status.status.c_str());
    ImGui::NextColumn();
    
    // Latency with color coding
    ImVec4 latency_color = getLatencyColor(status.latency_ms);
    ImGui::TextColored(latency_color, "%.1f ms", status.latency_ms);
    ImGui::NextColumn();
    
    // Active orders
    ImGui::Text("%d", status.active_orders);
    ImGui::NextColumn();
    
    // Daily volume
    ImGui::Text("%.2f", status.daily_volume);
    ImGui::NextColumn();
    
    // Health indicator with color
    ImVec4 health_color = getHealthColor(status.health);
    ImGui::TextColored(health_color, "%s", status.health.c_str());
    ImGui::NextColumn();
}

void ExchangeWindow::drawExchangeMetricsRow(const ExchangeMetrics& metrics, int row_id) {
    ImGui::Text("%s", metrics.exchange_name.c_str());
    ImGui::NextColumn();
    ImGui::Text("%.2f", metrics.total_traded_volume);
    ImGui::NextColumn();
    ImGui::Text("%d/%d", metrics.successful_orders, metrics.total_orders);
    ImGui::NextColumn();
    ImGui::Text("%.1f%%", metrics.success_rate);
    ImGui::NextColumn();
    ImGui::Text("%.1f ms", metrics.avg_latency_ms);
    ImGui::NextColumn();
}

void ExchangeWindow::drawConnectionButton(const std::string& exchange_name, bool is_connected) {
    if (is_connected) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
        std::string button_label = "Disconnect " + exchange_name;
        if (ImGui::Button(button_label.c_str())) {
            disconnectExchange(exchange_name);
        }
        ImGui::PopStyleColor();
    } else {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.8f, 0.2f, 1.0f));
        std::string button_label = "Connect " + exchange_name;
        if (ImGui::Button(button_label.c_str())) {
            connectExchange(exchange_name);
        }
        ImGui::PopStyleColor();
    }
}

void ExchangeWindow::updateConnectionStatus() {
    // TODO: Implement actual connection status updates
}

void ExchangeWindow::updateLatencyData() {
    // TODO: Implement latency data collection
}

void ExchangeWindow::updateVolumeData() {
    // TODO: Implement volume data collection
}

void ExchangeWindow::connectExchange(const std::string& exchange_name) {
    addConnectionLog("Connecting to " + exchange_name + "...");
    // TODO: Implement actual connection logic
}

void ExchangeWindow::disconnectExchange(const std::string& exchange_name) {
    addConnectionLog("Disconnecting from " + exchange_name + "...");
    // TODO: Implement actual disconnection logic
}

void ExchangeWindow::reconnectExchange(const std::string& exchange_name) {
    addConnectionLog("Reconnecting to " + exchange_name + "...");
    // TODO: Implement actual reconnection logic
}

void ExchangeWindow::connectAllExchanges() {
    addConnectionLog("Connecting to all exchanges...");
    for (const auto& status : exchange_statuses_) {
        if (!status.is_connected) {
            connectExchange(status.name);
        }
    }
}

void ExchangeWindow::disconnectAllExchanges() {
    addConnectionLog("Disconnecting from all exchanges...");
    for (const auto& status : exchange_statuses_) {
        if (status.is_connected) {
            disconnectExchange(status.name);
        }
    }
}

void ExchangeWindow::addConnectionLog(const std::string& message) {
    auto now = std::chrono::system_clock::now();
    std::time_t time_t = std::chrono::system_clock::to_time_t(now);
    std::string timestamp = std::ctime(&time_t);
    timestamp.pop_back(); // Remove newline
    
    std::string log_entry = "[" + timestamp + "] " + message;
    connection_logs_.push_back(log_entry);
    
    // Keep only last 100 log entries
    if (connection_logs_.size() > 100) {
        connection_logs_.erase(connection_logs_.begin());
    }
}

ImVec4 ExchangeWindow::getHealthColor(const std::string& health) const {
    if (health == "Healthy") return ImVec4(0.0f, 1.0f, 0.0f, 1.0f);      // Green
    if (health == "Warning") return ImVec4(1.0f, 1.0f, 0.0f, 1.0f);      // Yellow
    if (health == "Critical") return ImVec4(1.0f, 0.0f, 0.0f, 1.0f);     // Red
    return ImVec4(0.5f, 0.5f, 0.5f, 1.0f); // Gray (Unknown)
}

ImVec4 ExchangeWindow::getStatusColor(const std::string& status) const {
    if (status == "Connected") return ImVec4(0.0f, 1.0f, 0.0f, 1.0f);         // Green
    if (status == "Connecting") return ImVec4(1.0f, 1.0f, 0.0f, 1.0f);        // Yellow
    if (status == "Disconnected") return ImVec4(0.5f, 0.5f, 0.5f, 1.0f);      // Gray
    if (status == "Connection Error") return ImVec4(1.0f, 0.0f, 0.0f, 1.0f);  // Red
    return ImVec4(1.0f, 1.0f, 1.0f, 1.0f); // White (default)
}

ImVec4 ExchangeWindow::getLatencyColor(double latency_ms) const {
    if (latency_ms < 50.0) return ImVec4(0.0f, 1.0f, 0.0f, 1.0f);      // Green
    if (latency_ms < 100.0) return ImVec4(1.0f, 1.0f, 0.0f, 1.0f);     // Yellow
    if (latency_ms < 200.0) return ImVec4(1.0f, 0.5f, 0.0f, 1.0f);     // Orange
    return ImVec4(1.0f, 0.0f, 0.0f, 1.0f); // Red
}

} // namespace gui
} // namespace moneybot
