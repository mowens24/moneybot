#include "core/strategy_controller.h"
#include <chrono>
#include <algorithm>
#include <numeric>
#include <iostream>

namespace moneybot {

StrategyController::StrategyController() {
    logger_ = &ModernLogger::getInstance();
    config_manager_ = &ConfigManager::getInstance();
    
    // Get legacy logger for components that need it
    auto legacy_logger = std::make_shared<Logger>();
    
    // Initialize event manager
    event_manager_ = &EventManager::getInstance();
    
    // Initialize core managers
    exchange_manager_ = std::make_shared<ExchangeManager>(
        std::shared_ptr<ConfigManager>(config_manager_, [](ConfigManager*){}),
        std::shared_ptr<ModernLogger>(logger_, [](ModernLogger*){}),
        std::shared_ptr<EventManager>(event_manager_, [](EventManager*){})
    );
    
    portfolio_manager_ = std::make_shared<PortfolioManager>(legacy_logger, nlohmann::json{});
    risk_manager_ = std::make_shared<RiskManager>(legacy_logger, nlohmann::json{});
    
    // Initialize strategy engine
    strategy_engine_ = std::make_unique<StrategyEngine>(legacy_logger, nlohmann::json{});
    
    logger_->info("StrategyController initialized");
}

StrategyController::~StrategyController() {
    if (is_running_) {
        stop();
    }
}

bool StrategyController::initialize(const nlohmann::json& config) {
    std::lock_guard<std::mutex> lock(config_mutex_);
    
    try {
        config_ = config;
        
        // Core managers are already constructed and ready to use
        
        // Initialize strategy engine with a default configuration
        // (We'll remove the initialize method requirement for now)
        
        // Load strategies from config
        if (config.contains("strategies")) {
            for (const auto& [name, strategy_config] : config["strategies"].items()) {
                if (strategy_config.value("enabled", false)) {
                    std::string type = strategy_config.value("type", "");
                    
                    // Skip strategies without a type or with unrecognized types
                    if (type.empty()) {
                        logger_->warn("Strategy '" + name + "' has no type specified - skipping");
                        continue;
                    }
                    
                    // Only load strategies we have implementations for
                    if (type != "triangle_arbitrage" && type != "cross_exchange_arbitrage") {
                        logger_->warn("Strategy '" + name + "' has unrecognized type '" + type + "' - skipping");
                        continue;
                    }
                    
                    if (!addStrategy(name, type, strategy_config)) {
                        logger_->warn("Failed to add strategy: " + name);
                    }
                }
            }
        }
        
        logger_->info("StrategyController initialized successfully");
        return true;
        
    } catch (const std::exception& e) {
        logger_->error("Exception in StrategyController::initialize: " + std::string(e.what()));
        return false;
    }
}

void StrategyController::start() {
    std::lock_guard<std::mutex> lock(state_mutex_);
    
    if (is_running_) {
        logger_->warn("StrategyController already running");
        return;
    }
    
    try {
        // Start core managers (remove calls to non-existent methods)
        // exchange_manager_, portfolio_manager_, risk_manager_ are already initialized
        
        // Start strategy engine
        strategy_engine_->start();
        
        // Start monitoring thread
        should_stop_monitor_ = false;
        monitor_thread_ = std::thread(&StrategyController::monitorLoop, this);
        
        is_running_ = true;
        is_paused_ = false;
        emergency_stopped_ = false;
        
        logger_->info("StrategyController started");
        
    } catch (const std::exception& e) {
        logger_->error("Exception in StrategyController::start: " + std::string(e.what()));
    }
}

void StrategyController::stop() {
    std::lock_guard<std::mutex> lock(state_mutex_);
    
    if (!is_running_) {
        return;
    }
    
    try {
        // Stop monitoring thread
        should_stop_monitor_ = true;
        if (monitor_thread_.joinable()) {
            monitor_thread_.join();
        }
        
        // Stop strategy engine
        strategy_engine_->stop();
        
        // Core managers don't need explicit stopping
        
        is_running_ = false;
        is_paused_ = false;
        
        logger_->info("StrategyController stopped");
        
    } catch (const std::exception& e) {
        logger_->error("Exception in StrategyController::stop: " + std::string(e.what()));
    }
}

void StrategyController::pause() {
    std::lock_guard<std::mutex> lock(state_mutex_);
    
    if (!is_running_ || is_paused_) {
        return;
    }
    
    strategy_engine_->pause();
    is_paused_ = true;
    
    logger_->info("StrategyController paused");
}

void StrategyController::resume() {
    std::lock_guard<std::mutex> lock(state_mutex_);
    
    if (!is_running_ || !is_paused_) {
        return;
    }
    
    strategy_engine_->resume();
    is_paused_ = false;
    
    logger_->info("StrategyController resumed");
}

bool StrategyController::isRunning() const {
    std::lock_guard<std::mutex> lock(state_mutex_);
    return is_running_;
}

bool StrategyController::addStrategy(const std::string& name, const std::string& type, const nlohmann::json& config) {
    std::lock_guard<std::mutex> lock(strategies_mutex_);
    
    try {
        // Check if strategy already exists
        if (strategies_.find(name) != strategies_.end()) {
            logger_->warn("Strategy already exists: " + name);
            return false;
        }
        
        // Create strategy
        auto strategy = createStrategy(type, config);
        if (!strategy) {
            logger_->error("Failed to create strategy: " + name + " (type: " + type + ")");
            return false;
        }
        
        // Setup callbacks
        setupStrategyCallbacks(strategy);
        
        // Add to strategy engine
        strategy_engine_->addStrategy(strategy);
        
        // Track strategy
        strategies_[name] = strategy;
        
        // Initialize status
        StrategyStatus status;
        status.name = name;
        status.type = type;
        status.is_active = false;
        status.is_enabled = config.value("enabled", false);
        status.total_pnl = 0.0;
        status.daily_pnl = 0.0;
        status.total_trades = 0;
        status.win_rate = 0.0;
        status.sharpe_ratio = 0.0;
        status.max_drawdown = 0.0;
        status.last_update = std::chrono::steady_clock::now();
        status.last_error = "";
        
        strategy_statuses_[name] = status;
        
        logger_->info("Strategy added: " + name + " (type: " + type + ")");
        
        if (on_strategy_update_) {
            on_strategy_update_("Strategy added: " + name);
        }
        
        return true;
        
    } catch (const std::exception& e) {
        logger_->error("Exception in addStrategy: " + std::string(e.what()));
        return false;
    }
}

bool StrategyController::removeStrategy(const std::string& name) {
    std::lock_guard<std::mutex> lock(strategies_mutex_);
    
    try {
        auto it = strategies_.find(name);
        if (it == strategies_.end()) {
            logger_->warn("Strategy not found for removal: " + name);
            return false;
        }
        
        // Stop strategy first
        strategy_engine_->stopStrategy(name);
        strategy_engine_->removeStrategy(name);
        
        // Remove from tracking
        strategies_.erase(it);
        strategy_statuses_.erase(name);
        
        logger_->info("Strategy removed: " + name);
        
        if (on_strategy_update_) {
            on_strategy_update_("Strategy removed: " + name);
        }
        
        return true;
        
    } catch (const std::exception& e) {
        logger_->error("Exception in removeStrategy: " + std::string(e.what()));
        return false;
    }
}

bool StrategyController::enableStrategy(const std::string& name) {
    std::lock_guard<std::mutex> lock(strategies_mutex_);
    
    auto it = strategy_statuses_.find(name);
    if (it == strategy_statuses_.end()) {
        logger_->warn("Strategy not found for enabling: " + name);
        return false;
    }
    
    if (strategy_engine_->getStrategy(name)) {
        strategy_engine_->startStrategy(name);
        it->second.is_enabled = true;
        logger_->info("Strategy enabled: " + name);
        
        if (on_strategy_update_) {
            on_strategy_update_("Strategy enabled: " + name);
        }
        
        return true;
    }
    
    logger_->error("Failed to enable strategy: " + name);
    return false;
}

bool StrategyController::disableStrategy(const std::string& name) {
    std::lock_guard<std::mutex> lock(strategies_mutex_);
    
    auto it = strategy_statuses_.find(name);
    if (it == strategy_statuses_.end()) {
        logger_->warn("Strategy not found for disabling: " + name);
        return false;
    }
    
    if (strategy_engine_->getStrategy(name)) {
        strategy_engine_->stopStrategy(name);
        it->second.is_enabled = false;
        it->second.is_active = false;
        logger_->info("Strategy disabled: " + name);
        
        if (on_strategy_update_) {
            on_strategy_update_("Strategy disabled: " + name);
        }
        
        return true;
    }
    
    logger_->error("Failed to disable strategy: " + name);
    return false;
}

std::vector<std::string> StrategyController::getStrategyNames() const {
    std::lock_guard<std::mutex> lock(strategies_mutex_);
    
    std::vector<std::string> names;
    for (const auto& [name, _] : strategies_) {
        names.push_back(name);
    }
    
    return names;
}

StrategyController::StrategyStatus StrategyController::getStrategyStatus(const std::string& name) const {
    std::lock_guard<std::mutex> lock(strategies_mutex_);
    
    auto it = strategy_statuses_.find(name);
    if (it != strategy_statuses_.end()) {
        return it->second;
    }
    
    return StrategyStatus{}; // Return empty status if not found
}

StrategyController::PerformanceMetrics StrategyController::getPerformanceMetrics() const {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    return current_metrics_;
}

nlohmann::json StrategyController::getDetailedReport() const {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    
    nlohmann::json report;
    report["timestamp"] = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    report["is_running"] = is_running_.load();
    report["is_paused"] = is_paused_.load();
    report["emergency_stopped"] = emergency_stopped_.load();
    
    // Overall metrics
    report["overall_metrics"] = {
        {"total_portfolio_value", current_metrics_.total_portfolio_value},
        {"total_pnl", current_metrics_.total_pnl},
        {"daily_pnl", current_metrics_.daily_pnl},
        {"total_trades", current_metrics_.total_trades},
        {"overall_win_rate", current_metrics_.overall_win_rate},
        {"overall_sharpe_ratio", current_metrics_.overall_sharpe_ratio},
        {"max_drawdown", current_metrics_.max_drawdown},
        {"var_95", current_metrics_.var_95}
    };
    
    // Strategy details
    report["strategies"] = nlohmann::json::array();
    for (const auto& status : current_metrics_.strategy_statuses) {
        nlohmann::json strategy_report;
        strategy_report["name"] = status.name;
        strategy_report["type"] = status.type;
        strategy_report["is_active"] = status.is_active;
        strategy_report["is_enabled"] = status.is_enabled;
        strategy_report["total_pnl"] = status.total_pnl;
        strategy_report["daily_pnl"] = status.daily_pnl;
        strategy_report["total_trades"] = status.total_trades;
        strategy_report["win_rate"] = status.win_rate;
        strategy_report["sharpe_ratio"] = status.sharpe_ratio;
        strategy_report["max_drawdown"] = status.max_drawdown;
        strategy_report["last_error"] = status.last_error;
        
        report["strategies"].push_back(strategy_report);
    }
    
    return report;
}

void StrategyController::resetMetrics() {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    
    current_metrics_ = PerformanceMetrics{};
    
    // Reset strategy statuses
    std::lock_guard<std::mutex> strategies_lock(strategies_mutex_);
    for (auto& [name, status] : strategy_statuses_) {
        status.total_pnl = 0.0;
        status.daily_pnl = 0.0;
        status.total_trades = 0;
        status.win_rate = 0.0;
        status.sharpe_ratio = 0.0;
        status.max_drawdown = 0.0;
        status.last_error = "";
    }
    
    logger_->info("Metrics reset");
}

void StrategyController::emergencyStop() {
    std::lock_guard<std::mutex> lock(state_mutex_);
    
    emergency_stopped_ = true;
    
    // Stop all strategies immediately
    strategy_engine_->stop();
    
    // Cancel all open orders (remove non-existent method call)
    // portfolio_manager_->cancelAllOrders();
    
    logger_->critical("EMERGENCY STOP ACTIVATED");
    
    if (on_strategy_update_) {
        on_strategy_update_("EMERGENCY STOP ACTIVATED");
    }
}

bool StrategyController::isEmergencyStopped() const {
    std::lock_guard<std::mutex> lock(state_mutex_);
    return emergency_stopped_;
}

void StrategyController::setGUICallbacks(
    std::function<void(const std::string&)> on_strategy_update,
    std::function<void(const PerformanceMetrics&)> on_metrics_update) {
    
    on_strategy_update_ = on_strategy_update;
    on_metrics_update_ = on_metrics_update;
}

void StrategyController::updateConfiguration(const nlohmann::json& config) {
    std::lock_guard<std::mutex> lock(config_mutex_);
    config_ = config;
    
    // Update core managers (remove non-existent method calls)
    // The managers will pick up config changes as needed
    
    logger_->info("Configuration updated");
}

nlohmann::json StrategyController::getConfiguration() const {
    std::lock_guard<std::mutex> lock(config_mutex_);
    return config_;
}

// Private methods

void StrategyController::monitorLoop() {
    logger_->info("StrategyController monitor loop started");
    
    while (!should_stop_monitor_) {
        try {
            updateStrategyStatuses();
            updatePerformanceMetrics();
            checkRiskLimits();
            processStrategyEvents();
            
            // Update GUI if callback is set
            if (on_metrics_update_) {
                on_metrics_update_(getPerformanceMetrics());
            }
            
        } catch (const std::exception& e) {
            logger_->error("Exception in monitor loop: " + std::string(e.what()));
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100)); // 10 Hz monitoring
    }
    
    logger_->info("StrategyController monitor loop stopped");
}

void StrategyController::updateStrategyStatuses() {
    std::lock_guard<std::mutex> lock(strategies_mutex_);
    
    for (auto& [name, status] : strategy_statuses_) {
        auto it = strategies_.find(name);
        if (it != strategies_.end()) {
            auto strategy = it->second;
            auto metrics = strategy->getMetrics();
            
            status.is_active = strategy->getStatus() == moneybot::StrategyStatus::RUNNING;
            status.total_pnl = metrics.total_pnl;
            status.daily_pnl = 0.0; // TODO: Calculate daily PnL
            status.total_trades = metrics.total_trades;
            status.win_rate = metrics.win_rate;
            status.sharpe_ratio = metrics.sharpe_ratio;
            status.max_drawdown = metrics.max_drawdown;
            status.last_update = std::chrono::steady_clock::now();
            
            // Check for errors
            // Skip error time check for now
            // TODO: Add error tracking to StrategyMetrics
        }
    }
}

void StrategyController::updatePerformanceMetrics() {
    std::lock_guard<std::mutex> metrics_lock(metrics_mutex_);
    std::lock_guard<std::mutex> strategies_lock(strategies_mutex_);
    
    // Get portfolio metrics
    auto portfolio_metrics = portfolio_manager_->getPerformanceReport();
    
    current_metrics_.total_portfolio_value = portfolio_metrics.value("total_value", 0.0);
    current_metrics_.total_pnl = portfolio_metrics.value("total_pnl", 0.0);
    current_metrics_.daily_pnl = portfolio_metrics.value("daily_pnl", 0.0);
    current_metrics_.total_trades = portfolio_metrics.value("total_trades", 0);
    
    // Calculate overall metrics
    current_metrics_.overall_win_rate = calculateOverallWinRate();
    current_metrics_.overall_sharpe_ratio = calculateOverallSharpeRatio();
    current_metrics_.max_drawdown = calculateMaxDrawdown();
    current_metrics_.var_95 = calculateVaR95();
    
    // Copy strategy statuses
    current_metrics_.strategy_statuses.clear();
    for (const auto& [name, status] : strategy_statuses_) {
        current_metrics_.strategy_statuses.push_back(status);
    }
}

void StrategyController::checkRiskLimits() {
    // Check overall risk limits
    auto risk_metrics = risk_manager_->getRiskReport();
    
    // Check for emergency stop (simplified for now)
    if (risk_metrics.contains("emergency_stop") && risk_metrics["emergency_stop"].get<bool>()) {
        if (!emergency_stopped_) {
            logger_->critical("Risk manager triggered emergency stop");
            emergencyStop();
        }
    }
    
    // Check individual strategy limits
    std::lock_guard<std::mutex> lock(strategies_mutex_);
    for (const auto& [name, status] : strategy_statuses_) {
        if (!checkStrategyLimits(name)) {
            logger_->warn("Strategy " + name + " violated risk limits");
            handleRiskViolation(name, "Risk limit violation");
        }
    }
}

void StrategyController::processStrategyEvents() {
    // Process events from strategy engine
    // This would handle strategy state changes, errors, etc.
    // For now, we'll keep it simple
}

std::shared_ptr<BaseStrategy> StrategyController::createStrategy(const std::string& type, const nlohmann::json& config) {
    try {
        if (type == "triangle_arbitrage") {
            auto legacy_logger = std::make_shared<Logger>();
            return std::make_shared<TriangleArbitrageStrategy>("triangle_arbitrage", config, legacy_logger);
        } else if (type == "cross_exchange_arbitrage") {
            auto legacy_logger = std::make_shared<Logger>();
            return std::make_shared<strategy::CrossExchangeArbitrageStrategy>(
                "cross_exchange_arbitrage", config, legacy_logger, risk_manager_, exchange_manager_);
        } else {
            logger_->error("Unknown strategy type: " + type);
            return nullptr;
        }
    } catch (const std::exception& e) {
        logger_->error("Exception creating strategy: " + std::string(e.what()));
        return nullptr;
    }
}

void StrategyController::setupStrategyCallbacks(std::shared_ptr<BaseStrategy> strategy) {
    // Setup callbacks for strategy events
    // This would include order callbacks, error callbacks, etc.
    // Implementation depends on the Strategy base class interface
}

void StrategyController::handleRiskViolation(const std::string& strategy_name, const std::string& violation) {
    logger_->warn("Risk violation for strategy " + strategy_name + ": " + violation);
    
    // Disable the strategy temporarily
    disableStrategy(strategy_name);
    
    // Update status
    std::lock_guard<std::mutex> lock(strategies_mutex_);
    auto it = strategy_statuses_.find(strategy_name);
    if (it != strategy_statuses_.end()) {
        it->second.last_error = violation;
    }
}

bool StrategyController::checkStrategyLimits(const std::string& strategy_name) const {
    auto it = strategy_statuses_.find(strategy_name);
    if (it == strategy_statuses_.end()) {
        return false;
    }
    
    const auto& status = it->second;
    
    // Check basic limits (these could be configurable)
    if (status.max_drawdown > 10.0) { // 10% max drawdown
        return false;
    }
    
    if (status.daily_pnl < -5000.0) { // $5000 daily loss limit
        return false;
    }
    
    return true;
}

double StrategyController::calculateOverallWinRate() const {
    int total_trades = 0;
    int winning_trades = 0;
    
    for (const auto& [name, status] : strategy_statuses_) {
        total_trades += status.total_trades;
        winning_trades += static_cast<int>(status.total_trades * status.win_rate / 100.0);
    }
    
    return total_trades > 0 ? (static_cast<double>(winning_trades) / total_trades) * 100.0 : 0.0;
}

double StrategyController::calculateOverallSharpeRatio() const {
    std::vector<double> sharpe_ratios;
    std::vector<double> weights;
    
    for (const auto& [name, status] : strategy_statuses_) {
        if (status.total_trades > 0) {
            sharpe_ratios.push_back(status.sharpe_ratio);
            weights.push_back(std::abs(status.total_pnl)); // Weight by absolute PnL
        }
    }
    
    if (sharpe_ratios.empty()) {
        return 0.0;
    }
    
    double total_weight = std::accumulate(weights.begin(), weights.end(), 0.0);
    if (total_weight == 0.0) {
        return 0.0;
    }
    
    double weighted_sharpe = 0.0;
    for (size_t i = 0; i < sharpe_ratios.size(); ++i) {
        weighted_sharpe += sharpe_ratios[i] * (weights[i] / total_weight);
    }
    
    return weighted_sharpe;
}

double StrategyController::calculateMaxDrawdown() const {
    double max_drawdown = 0.0;
    
    for (const auto& [name, status] : strategy_statuses_) {
        max_drawdown = std::max(max_drawdown, status.max_drawdown);
    }
    
    return max_drawdown;
}

double StrategyController::calculateVaR95() const {
    // Simple VaR calculation based on current portfolio value and historical volatility
    // This is a placeholder implementation
    return current_metrics_.total_portfolio_value * 0.05; // 5% of portfolio value
}

} // namespace moneybot
