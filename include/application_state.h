#pragma once

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <functional>
#include <atomic>
#include <chrono>

namespace moneybot {

enum class AppState {
    INITIALIZING,
    CONNECTING,
    CONNECTED,
    TRADING,
    PAUSED,
    DISCONNECTING,
    SHUTDOWN,
    ERROR
};

enum class TradingMode {
    DEMO,
    PAPER,
    LIVE
};

class ApplicationStateManager {
public:
    static ApplicationStateManager& getInstance() {
        static ApplicationStateManager instance;
        return instance;
    }

    // State management
    void setState(AppState state);
    AppState getState() const;
    std::string getStateString() const;
    bool isStateTransitionValid(AppState from, AppState to) const;

    // Trading mode
    void setTradingMode(TradingMode mode);
    TradingMode getTradingMode() const;
    std::string getTradingModeString() const;

    // Configuration
    void setConfig(const std::string& key, const std::string& value);
    std::string getConfig(const std::string& key, const std::string& default_value = "") const;
    bool hasConfig(const std::string& key) const;

    // Connection status
    void setExchangeConnected(const std::string& exchange, bool connected);
    bool isExchangeConnected(const std::string& exchange) const;
    size_t getConnectedExchangeCount() const;

    // Error handling
    void setError(const std::string& error_message);
    std::string getLastError() const;
    bool hasError() const;
    void clearError();

    // Statistics
    void incrementOrderCount();
    void incrementTradeCount();
    void updatePnL(double pnl);
    size_t getOrderCount() const;
    size_t getTradeCount() const;
    double getTotalPnL() const;

    // Callbacks for state changes
    using StateChangeCallback = std::function<void(AppState old_state, AppState new_state)>;
    void addStateChangeCallback(const std::string& name, StateChangeCallback callback);
    void removeStateChangeCallback(const std::string& name);

    // Shutdown management
    void requestShutdown();
    bool isShutdownRequested() const;
    void setShutdownComplete();

    // Utility functions
    std::chrono::steady_clock::time_point getStartTime() const;
    std::chrono::seconds getUptime() const;

private:
    ApplicationStateManager();
    ~ApplicationStateManager() = default;
    ApplicationStateManager(const ApplicationStateManager&) = delete;
    ApplicationStateManager& operator=(const ApplicationStateManager&) = delete;

    void notifyStateChange(AppState old_state, AppState new_state);

    mutable std::mutex state_mutex_;
    std::atomic<AppState> current_state_{AppState::INITIALIZING};
    std::atomic<TradingMode> trading_mode_{TradingMode::DEMO};
    
    std::unordered_map<std::string, std::string> config_;
    std::unordered_map<std::string, bool> exchange_connections_;
    std::unordered_map<std::string, StateChangeCallback> state_callbacks_;
    
    std::string last_error_;
    std::atomic<bool> has_error_{false};
    
    std::atomic<size_t> order_count_{0};
    std::atomic<size_t> trade_count_{0};
    std::atomic<double> total_pnl_{0.0};
    
    std::atomic<bool> shutdown_requested_{false};
    std::atomic<bool> shutdown_complete_{false};
    
    std::chrono::steady_clock::time_point start_time_;
};

} // namespace moneybot
