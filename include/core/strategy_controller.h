#pragma once

#include "strategy/strategy_engine.h"
#include "strategy/triangle_arbitrage_strategy.h"
#include "strategy/cross_exchange_arbitrage_strategy.h"
#include "core/exchange_manager.h"
#include "core/portfolio_manager.h"
#include "risk_manager.h"
#include "config_manager.h"
#include "modern_logger.h"
#include "event_manager.h"
#include <memory>
#include <vector>
#include <string>
#include <mutex>
#include <thread>
#include <atomic>

namespace moneybot {

/**
 * @brief High-level controller that manages all trading strategies
 * 
 * This class integrates the StrategyEngine with the main application,
 * providing a unified interface for strategy management, real-time
 * performance monitoring, and risk control.
 */
class StrategyController {
public:
    struct StrategyStatus {
        std::string name;
        std::string type;
        bool is_active;
        bool is_enabled;
        double total_pnl;
        double daily_pnl;
        int total_trades;
        double win_rate;
        double sharpe_ratio;
        double max_drawdown;
        std::chrono::steady_clock::time_point last_update;
        std::string last_error;
    };

    struct PerformanceMetrics {
        double total_portfolio_value;
        double total_pnl;
        double daily_pnl;
        int total_trades;
        double overall_win_rate;
        double overall_sharpe_ratio;
        double max_drawdown;
        double var_95;
        std::vector<StrategyStatus> strategy_statuses;
    };

    StrategyController();
    ~StrategyController();

    // Core lifecycle
    bool initialize(const nlohmann::json& config);
    void start();
    void stop();
    void pause();
    void resume();
    bool isRunning() const;

    // Strategy management
    bool addStrategy(const std::string& name, const std::string& type, const nlohmann::json& config);
    bool removeStrategy(const std::string& name);
    bool enableStrategy(const std::string& name);
    bool disableStrategy(const std::string& name);
    std::vector<std::string> getStrategyNames() const;
    StrategyStatus getStrategyStatus(const std::string& name) const;

    // Performance monitoring
    PerformanceMetrics getPerformanceMetrics() const;
    nlohmann::json getDetailedReport() const;
    void resetMetrics();

    // Risk controls
    void setRiskLimits(const nlohmann::json& limits);
    void emergencyStop();
    bool isEmergencyStopped() const;

    // GUI integration
    void setGUICallbacks(std::function<void(const std::string&)> on_strategy_update,
                        std::function<void(const PerformanceMetrics&)> on_metrics_update);

    // Configuration
    void updateConfiguration(const nlohmann::json& config);
    nlohmann::json getConfiguration() const;

private:
    // Core components
    std::unique_ptr<StrategyEngine> strategy_engine_;
    std::shared_ptr<ExchangeManager> exchange_manager_;
    std::shared_ptr<PortfolioManager> portfolio_manager_;
    std::shared_ptr<RiskManager> risk_manager_;
    ConfigManager* config_manager_;
    ModernLogger* logger_;
    EventManager* event_manager_;

    // State management
    std::atomic<bool> is_running_{false};
    std::atomic<bool> is_paused_{false};
    std::atomic<bool> emergency_stopped_{false};
    mutable std::mutex state_mutex_;

    // Strategy tracking
    std::map<std::string, std::shared_ptr<BaseStrategy>> strategies_;
    std::map<std::string, StrategyStatus> strategy_statuses_;
    mutable std::mutex strategies_mutex_;

    // Performance tracking
    PerformanceMetrics current_metrics_;
    mutable std::mutex metrics_mutex_;

    // Background processing
    std::thread monitor_thread_;
    std::atomic<bool> should_stop_monitor_{false};

    // GUI callbacks
    std::function<void(const std::string&)> on_strategy_update_;
    std::function<void(const PerformanceMetrics&)> on_metrics_update_;

    // Configuration
    nlohmann::json config_;
    mutable std::mutex config_mutex_;

    // Internal methods
    void monitorLoop();
    void updateStrategyStatuses();
    void updatePerformanceMetrics();
    void checkRiskLimits();
    void processStrategyEvents();
    
    // Strategy factory methods
    std::shared_ptr<BaseStrategy> createStrategy(const std::string& type, const nlohmann::json& config);
    void setupStrategyCallbacks(std::shared_ptr<BaseStrategy> strategy);
    
    // Risk management
    void handleRiskViolation(const std::string& strategy_name, const std::string& violation);
    bool checkStrategyLimits(const std::string& strategy_name) const;
    
    // Performance calculation
    double calculateOverallWinRate() const;
    double calculateOverallSharpeRatio() const;
    double calculateMaxDrawdown() const;
    double calculateVaR95() const;
};

} // namespace moneybot
