#pragma once

#include "base_window.h"
#include "core/exchange_manager.h"
#include <memory>
#include <vector>
#include <string>
#include <map>
#include <chrono>
#include <imgui.h>

namespace moneybot {
namespace gui {

struct ExchangeStatus {
    std::string name;
    bool is_connected = false;
    std::string status = "Disconnected";
    double latency_ms = 0.0;
    int active_orders = 0;
    double daily_volume = 0.0;
    std::chrono::system_clock::time_point last_update;
    std::string health = "Unknown"; // Healthy, Warning, Critical, Unknown
};

struct ExchangeMetrics {
    std::string exchange_name;
    double total_traded_volume = 0.0;
    int total_orders = 0;
    int successful_orders = 0;
    int failed_orders = 0;
    double success_rate = 0.0;
    double avg_latency_ms = 0.0;
    double uptime_pct = 0.0;
};

class ExchangeWindow : public BaseWindow {
private:
    std::shared_ptr<ExchangeManager> exchange_manager_;
    
    // UI state
    bool show_exchange_status_ = true;
    bool show_exchange_metrics_ = true;
    bool show_latency_chart_ = true;
    bool show_volume_chart_ = true;
    bool show_connection_logs_ = true;
    
    // Data
    std::vector<ExchangeStatus> exchange_statuses_;
    std::vector<ExchangeMetrics> exchange_metrics_;
    std::vector<std::string> connection_logs_;
    
    // Chart data
    std::map<std::string, std::vector<float>> latency_history_;
    std::map<std::string, std::vector<float>> volume_history_;
    
    // Selected exchange for detailed view
    std::string selected_exchange_ = "";
    
    // Log filtering
    std::string log_filter_ = "";
    bool auto_scroll_logs_ = true;
    
public:
    explicit ExchangeWindow(std::shared_ptr<ExchangeManager> exchange_manager);
    ~ExchangeWindow() override = default;
    
    void render() override;
    void updateData();
    
    // Connection management
    void connectExchange(const std::string& exchange_name);
    void disconnectExchange(const std::string& exchange_name);
    void reconnectExchange(const std::string& exchange_name);
    void connectAllExchanges();
    void disconnectAllExchanges();
    
    // Logging
    void addConnectionLog(const std::string& message);
    
private:
    void renderExchangeStatus();
    void renderExchangeMetrics();
    void renderLatencyChart();
    void renderVolumeChart();
    void renderConnectionLogs();
    void renderExchangeControls();
    
    // Helper functions
    void drawExchangeStatusRow(const ExchangeStatus& status, int row_id);
    void drawExchangeMetricsRow(const ExchangeMetrics& metrics, int row_id);
    void updateLatencyData();
    void updateVolumeData();
    void updateConnectionStatus();
    
    // Visualization
    void drawHealthIndicator(const std::string& health, ImVec2 position, float radius);
    void drawConnectionButton(const std::string& exchange_name, bool is_connected);
    
    // Color coding
    ImVec4 getHealthColor(const std::string& health) const;
    ImVec4 getStatusColor(const std::string& status) const;
    ImVec4 getLatencyColor(double latency_ms) const;
    
    // Formatting helpers
    std::string formatLatency(double latency_ms) const;
    std::string formatVolume(double volume) const;
    std::string formatUptime(double uptime_pct) const;
    std::string formatTime(const std::chrono::system_clock::time_point& time) const;
};

} // namespace gui
} // namespace moneybot
