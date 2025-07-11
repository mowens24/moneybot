#ifndef STRATEGY_ENGINE_H
#define STRATEGY_ENGINE_H

#include <memory>
#include <vector>
#include <string>
#include <unordered_map>
#include <mutex>
#include <thread>
#include <atomic>
#include <functional>
#include <chrono>
#include <nlohmann/json.hpp>
#include "types.h"
#include "core/exchange_manager.h"

namespace moneybot {

class Logger;
class RiskManager;
class PortfolioManager;

// Forward declarations
class Strategy;

enum class StrategyStatus {
    STOPPED,
    STARTING,
    RUNNING,
    STOPPING,
    ERROR
};

struct StrategyMetrics {
    int total_trades = 0;
    int winning_trades = 0;
    int losing_trades = 0;
    double total_pnl = 0.0;
    double max_drawdown = 0.0;
    double win_rate = 0.0;
    double avg_win = 0.0;
    double avg_loss = 0.0;
    double sharpe_ratio = 0.0;
    std::chrono::system_clock::time_point last_trade_time;
    std::chrono::system_clock::time_point strategy_start_time;
};

// Base Strategy class
class BaseStrategy {
public:
    BaseStrategy(const std::string& name, const nlohmann::json& config,
             std::shared_ptr<Logger> logger);
    virtual ~BaseStrategy() = default;

    // Strategy lifecycle
    virtual bool initialize() = 0;
    virtual void start() = 0;
    virtual void stop() = 0;
    virtual void pause() = 0;
    virtual void resume() = 0;
    
    // Strategy execution
    virtual void onTick(const std::string& symbol, const Trade& tick) = 0;
    virtual void onOrderFill(const OrderFill& fill) = 0;
    virtual void onOrderReject(const OrderReject& reject) = 0;
    virtual void onBalanceUpdate(const Balance& balance) = 0;
    virtual void onPositionUpdate(const Position& position) = 0;
    
    // Strategy information
    std::string getName() const { return name_; }
    StrategyStatus getStatus() const { return status_; }
    StrategyMetrics getMetrics() const;
    nlohmann::json getConfig() const { return config_; }
    
    // Strategy control
    void setRiskManager(std::shared_ptr<RiskManager> risk_manager);
    void setPortfolioManager(std::shared_ptr<PortfolioManager> portfolio_manager);
    void addExchange(const std::string& exchange_name, std::shared_ptr<ExchangeManager> exchange);
    
    // Configuration
    void updateConfig(const nlohmann::json& config);
    
protected:
    // Helper methods for derived strategies
    bool validateOrder(const Order& order);
    bool placeOrder(const Order& order);
    bool cancelOrder(const std::string& order_id);
    void recordTrade(const Trade& trade);
    void updateMetrics();
    
    // Data access
    std::shared_ptr<ExchangeManager> getExchange(const std::string& name);
    std::shared_ptr<RiskManager> getRiskManager() { return risk_manager_; }
    std::shared_ptr<PortfolioManager> getPortfolioManager() { return portfolio_manager_; }
    std::shared_ptr<Logger> getLogger() { return logger_; }
    
    // Configuration access
    template<typename T>
    T getConfigValue(const std::string& key, const T& default_value) const {
        if (config_.contains(key)) {
            return config_[key].get<T>();
        }
        return default_value;
    }
    
    void setStatus(StrategyStatus status) { status_ = status; }

protected:
    std::string name_;
    std::shared_ptr<Logger> logger_;
    nlohmann::json config_;
    
    // Dependencies
    std::shared_ptr<RiskManager> risk_manager_;
    std::shared_ptr<PortfolioManager> portfolio_manager_;
    std::unordered_map<std::string, std::shared_ptr<ExchangeManager>> exchanges_;
    
    // Metrics
    StrategyMetrics metrics_;
    mutable std::mutex metrics_mutex_;
    
    // Thread safety
    mutable std::mutex config_mutex_;
    mutable std::mutex exchanges_mutex_;

private:
    std::atomic<StrategyStatus> status_;
};

// Strategy Engine - manages multiple strategies
class StrategyEngine {
public:
    StrategyEngine(std::shared_ptr<Logger> logger, const nlohmann::json& config);
    ~StrategyEngine();
    
    // Strategy management
    void addStrategy(std::shared_ptr<BaseStrategy> strategy);
    void removeStrategy(const std::string& strategy_name);
    std::shared_ptr<BaseStrategy> getStrategy(const std::string& name);
    std::vector<std::shared_ptr<BaseStrategy>> getAllStrategies();
    
    // Engine control
    void start();
    void stop();
    void pause();
    void resume();
    bool isRunning() const { return running_; }
    
    // Strategy control
    void startStrategy(const std::string& name);
    void stopStrategy(const std::string& name);
    void pauseStrategy(const std::string& name);
    void resumeStrategy(const std::string& name);
    
    // Event handling
    void onTick(const std::string& symbol, const Trade& tick);
    void onOrderFill(const OrderFill& fill);
    void onOrderReject(const OrderReject& reject);
    void onBalanceUpdate(const Balance& balance);
    void onPositionUpdate(const Position& position);
    
    // Metrics and reporting
    nlohmann::json getEngineMetrics() const;
    nlohmann::json getStrategyMetrics(const std::string& name) const;
    nlohmann::json getAllStrategyMetrics() const;
    
    // Dependencies
    void setRiskManager(std::shared_ptr<RiskManager> risk_manager);
    void setPortfolioManager(std::shared_ptr<PortfolioManager> portfolio_manager);
    void addExchange(const std::string& exchange_name, std::shared_ptr<ExchangeManager> exchange);
    
private:
    void runEngine();
    void distributeEvent(const std::function<void(std::shared_ptr<BaseStrategy>)>& event);
    
    std::shared_ptr<Logger> logger_;
    nlohmann::json config_;
    
    // Strategy management
    std::unordered_map<std::string, std::shared_ptr<BaseStrategy>> strategies_;
    mutable std::mutex strategies_mutex_;
    
    // Engine state
    std::atomic<bool> running_;
    std::atomic<bool> paused_;
    std::thread engine_thread_;
    
    // Dependencies
    std::shared_ptr<RiskManager> risk_manager_;
    std::shared_ptr<PortfolioManager> portfolio_manager_;
    std::unordered_map<std::string, std::shared_ptr<ExchangeManager>> exchanges_;
    mutable std::mutex exchanges_mutex_;
    
    // Event queue (simplified - in production would use a proper queue)
    std::mutex event_mutex_;
};

} // namespace moneybot

#endif // STRATEGY_ENGINE_H
