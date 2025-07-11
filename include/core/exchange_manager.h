#pragma once

#include <memory>
#include <map>
#include <vector>
#include <string>
#include <chrono>
#include <functional>
#include <mutex>
#include <atomic>
#include <thread>
#include <nlohmann/json.hpp>
#include "exchange_interface.h"

namespace moneybot {

// Forward declarations
class ConfigManager;
class ModernLogger;
class EventManager;

using ExchangeInterface = LiveExchangeInterface;

enum class ExchangeHealthStatus {
    HEALTHY,
    WARNING,
    CRITICAL,
    UNKNOWN,
    DISCONNECTED
};

struct ExchangeHealth {
    std::string exchange_name;
    ExchangeHealthStatus status = ExchangeHealthStatus::UNKNOWN;
    double latency_ms = 0.0;
    double uptime_percentage = 0.0;
    int failed_requests = 0;
    int total_requests = 0;
    std::chrono::system_clock::time_point last_check;
    std::string status_message;
    
    double getSuccessRate() const {
        if (total_requests == 0) return 0.0;
        return (double)(total_requests - failed_requests) / total_requests * 100.0;
    }
};

struct ExchangeMetrics {
    std::string exchange_name;
    int active_orders = 0;
    double daily_volume = 0.0;
    double total_volume = 0.0;
    int total_trades = 0;
    double avg_latency_ms = 0.0;
    std::chrono::system_clock::time_point session_start;
};

using HealthCallback = std::function<void(const std::string&, const ExchangeHealth&)>;
using MetricsCallback = std::function<void(const std::string&, const ExchangeMetrics&)>;

class ExchangeManager {
private:
    std::map<std::string, std::unique_ptr<ExchangeInterface>> exchanges_;
    std::map<std::string, ExchangeHealth> health_status_;
    std::map<std::string, ExchangeMetrics> metrics_;
    
    std::shared_ptr<ConfigManager> config_manager_;
    std::shared_ptr<ModernLogger> logger_;
    std::shared_ptr<EventManager> event_manager_;
    
    // Health monitoring
    std::atomic<bool> monitoring_active_{false};
    std::thread health_monitor_thread_;
    std::chrono::seconds health_check_interval_{30};
    
    // Callbacks
    std::vector<HealthCallback> health_callbacks_;
    std::vector<MetricsCallback> metrics_callbacks_;
    
    mutable std::mutex exchanges_mutex_;
    mutable std::mutex health_mutex_;
    mutable std::mutex metrics_mutex_;
    
public:
    ExchangeManager(std::shared_ptr<ConfigManager> config_manager,
                   std::shared_ptr<ModernLogger> logger,
                   std::shared_ptr<EventManager> event_manager);
    ~ExchangeManager();
    
    // Exchange management
    bool addExchange(const std::string& name, std::unique_ptr<ExchangeInterface> exchange);
    bool removeExchange(const std::string& name);
    ExchangeInterface* getExchange(const std::string& name);
    const ExchangeInterface* getExchange(const std::string& name) const;
    
    // Connection management
    bool connectExchange(const std::string& name);
    bool disconnectExchange(const std::string& name);
    bool reconnectExchange(const std::string& name);
    bool connectAllExchanges();
    void disconnectAllExchanges();
    
    // Status queries
    std::vector<std::string> getExchangeNames() const;
    std::vector<std::string> getConnectedExchanges() const;
    std::vector<std::string> getDisconnectedExchanges() const;
    bool isExchangeConnected(const std::string& name) const;
    
    // Health monitoring
    ExchangeHealth getExchangeHealth(const std::string& name) const;
    std::map<std::string, ExchangeHealth> getAllExchangeHealth() const;
    void startHealthMonitoring();
    void stopHealthMonitoring();
    bool isHealthMonitoringActive() const;
    
    // Metrics
    ExchangeMetrics getExchangeMetrics(const std::string& name) const;
    std::map<std::string, ExchangeMetrics> getAllExchangeMetrics() const;
    void updateMetrics(const std::string& exchange_name, const ExchangeMetrics& metrics);
    
    // Configuration
    bool loadExchangesFromConfig();
    void setHealthCheckInterval(std::chrono::seconds interval);
    
    // Callbacks
    void addHealthCallback(HealthCallback callback);
    void addMetricsCallback(MetricsCallback callback);
    
    // Emergency controls
    void emergencyDisconnectAll(const std::string& reason);
    bool isEmergencyStopActive() const;
    
private:
    void healthMonitorLoop();
    void checkExchangeHealth(const std::string& exchange_name);
    void updateExchangeHealth(const std::string& exchange_name, const ExchangeHealth& health);
    void notifyHealthCallbacks(const std::string& exchange_name, const ExchangeHealth& health);
    void notifyMetricsCallbacks(const std::string& exchange_name, const ExchangeMetrics& metrics);
    
    // Factory methods
    std::unique_ptr<ExchangeInterface> createExchange(const std::string& exchange_type, 
                                                     const nlohmann::json& config);
    
    // Utility functions
    ExchangeHealthStatus determineHealthStatus(const ExchangeHealth& health) const;
    std::string healthStatusToString(ExchangeHealthStatus status) const;
};

} // namespace moneybot
