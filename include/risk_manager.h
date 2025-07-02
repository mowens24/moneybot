#ifndef RISK_MANAGER_H
#define RISK_MANAGER_H

#include "logger.h"
#include "types.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <chrono>
#include <nlohmann/json.hpp>

namespace moneybot {

struct RiskLimits {
    double max_position_size;
    double max_order_size;
    double max_daily_loss;
    double max_drawdown;
    int max_orders_per_minute;
    double min_spread;
    double max_slippage;
    
    RiskLimits() = default;
    RiskLimits(const nlohmann::json& j);
};

class RiskManager {
public:
    RiskManager(std::shared_ptr<Logger> logger, const nlohmann::json& config);
    ~RiskManager() = default;
    
    // Risk checks
    bool checkOrderRisk(const Order& order);
    bool checkPositionRisk(const std::string& symbol, double new_position);
    bool checkDailyLoss(double current_pnl);
    bool checkDrawdown(double current_drawdown);
    bool checkOrderRate(const std::string& symbol);
    
    // Position tracking
    void updatePosition(const std::string& symbol, double quantity, double price);
    void updatePnL(const std::string& symbol, double pnl);
    
    // Limits management
    void setRiskLimits(const RiskLimits& limits);
    RiskLimits getRiskLimits() const;
    
    // Emergency controls
    void emergencyStop();
    void resume();
    bool isEmergencyStopped() const;
    
    // Reporting
    nlohmann::json getRiskReport() const;

private:
    // Internal tracking
    struct PositionInfo {
        double quantity;
        double avg_price;
        double unrealized_pnl;
        std::chrono::system_clock::time_point last_update;
    };
    
    struct OrderRateInfo {
        int order_count;
        std::chrono::system_clock::time_point window_start;
    };
    
    // Risk calculations
    double calculateDrawdown() const;
    double calculateDailyPnL() const;
    
    std::shared_ptr<Logger> logger_;
    RiskLimits limits_;
    
    // State tracking
    std::unordered_map<std::string, PositionInfo> positions_;
    std::unordered_map<std::string, OrderRateInfo> order_rates_;
    double total_realized_pnl_;
    double peak_equity_;
    double current_equity_;
    
    // Emergency state
    bool emergency_stopped_;
    
    // Thread safety
    mutable std::mutex positions_mutex_;
    mutable std::mutex rates_mutex_;
    mutable std::mutex limits_mutex_;
    
    // Timestamps
    std::chrono::system_clock::time_point session_start_;
    std::chrono::system_clock::time_point last_pnl_update_;
};

} // namespace moneybot

#endif // RISK_MANAGER_H 