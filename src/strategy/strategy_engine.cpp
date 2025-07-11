#include "strategy/strategy_engine.h"
#include "logger.h"
#include "risk_manager.h"
#include "core/portfolio_manager.h"
#include "exchange_interface.h"
#include <iostream>
#include <algorithm>

namespace moneybot {

// BaseStrategy implementation
BaseStrategy::BaseStrategy(const std::string& name, const nlohmann::json& config,
                   std::shared_ptr<Logger> logger)
    : name_(name), config_(config), logger_(logger), status_(StrategyStatus::STOPPED) {
    
    metrics_.strategy_start_time = std::chrono::system_clock::now();
    std::cout << "Strategy '" << name_ << "' created" << std::endl;
}

StrategyMetrics BaseStrategy::getMetrics() const {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    return metrics_;
}

void BaseStrategy::setRiskManager(std::shared_ptr<RiskManager> risk_manager) {
    risk_manager_ = risk_manager;
}

void BaseStrategy::setPortfolioManager(std::shared_ptr<PortfolioManager> portfolio_manager) {
    portfolio_manager_ = portfolio_manager;
}

void BaseStrategy::addExchange(const std::string& exchange_name, 
                          std::shared_ptr<ExchangeManager> exchange) {
    std::lock_guard<std::mutex> lock(exchanges_mutex_);
    exchanges_[exchange_name] = exchange;
}

void BaseStrategy::updateConfig(const nlohmann::json& config) {
    std::lock_guard<std::mutex> lock(config_mutex_);
    config_ = config;
}

bool BaseStrategy::validateOrder(const Order& order) {
    if (!risk_manager_) {
        std::cout << "Warning: No risk manager attached to strategy " << name_ << std::endl;
        return true;
    }
    
    return risk_manager_->checkOrderRisk(order);
}

bool BaseStrategy::placeOrder(const Order& order) {
    if (!validateOrder(order)) {
        std::cout << "Order validation failed for strategy " << name_ << std::endl;
        return false;
    }
    
    // Find appropriate exchange
    std::lock_guard<std::mutex> lock(exchanges_mutex_);
    for (auto& [exchange_name, exchange] : exchanges_) {
        if (exchange && exchange->isExchangeConnected(exchange_name)) {
            // Place order (simplified - in reality would match symbol to exchange)
            std::cout << "Placing order via " << exchange_name << " for strategy " << name_ << std::endl;
            // exchange->placeLimitOrder(order.symbol, order.side, order.quantity, order.price);
            return true;
        }
    }
    
    std::cout << "No connected exchange available for strategy " << name_ << std::endl;
    return false;
}

bool BaseStrategy::cancelOrder(const std::string& order_id) {
    std::lock_guard<std::mutex> lock(exchanges_mutex_);
    for (auto& [exchange_name, exchange] : exchanges_) {
        if (exchange && exchange->isExchangeConnected(exchange_name)) {
            // Cancel order (simplified)
            std::cout << "Canceling order " << order_id << " via " << exchange_name << std::endl;
            return true;
        }
    }
    return false;
}

void BaseStrategy::recordTrade(const Trade& trade) {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    
    metrics_.total_trades++;
    metrics_.last_trade_time = trade.timestamp;
    
    // Update portfolio manager if available
    if (portfolio_manager_) {
        portfolio_manager_->recordTrade(trade);
    }
    
    updateMetrics();
}

void BaseStrategy::updateMetrics() {
    // Calculate win rate
    if (metrics_.total_trades > 0) {
        metrics_.win_rate = static_cast<double>(metrics_.winning_trades) / metrics_.total_trades;
    }
    
    // Calculate averages
    if (metrics_.winning_trades > 0) {
        metrics_.avg_win = metrics_.total_pnl / metrics_.winning_trades;
    }
    
    if (metrics_.losing_trades > 0) {
        metrics_.avg_loss = std::abs(metrics_.total_pnl) / metrics_.losing_trades;
    }
}

std::shared_ptr<ExchangeManager> BaseStrategy::getExchange(const std::string& name) {
    std::lock_guard<std::mutex> lock(exchanges_mutex_);
    auto it = exchanges_.find(name);
    if (it != exchanges_.end()) {
        return it->second;
    }
    return nullptr;
}

// StrategyEngine implementation
StrategyEngine::StrategyEngine(std::shared_ptr<Logger> logger, const nlohmann::json& config)
    : logger_(logger), config_(config), running_(false), paused_(false) {
    
    std::cout << "StrategyEngine initialized" << std::endl;
}

StrategyEngine::~StrategyEngine() {
    stop();
    std::cout << "StrategyEngine destroyed" << std::endl;
}

void StrategyEngine::addStrategy(std::shared_ptr<BaseStrategy> strategy) {
    if (!strategy) {
        std::cout << "Cannot add null strategy" << std::endl;
        return;
    }
    
    std::lock_guard<std::mutex> lock(strategies_mutex_);
    strategies_[strategy->getName()] = strategy;
    
    // Set up dependencies
    if (risk_manager_) {
        strategy->setRiskManager(risk_manager_);
    }
    
    if (portfolio_manager_) {
        strategy->setPortfolioManager(portfolio_manager_);
    }
    
    // Add exchanges
    {
        std::lock_guard<std::mutex> exchange_lock(exchanges_mutex_);
        for (const auto& [name, exchange] : exchanges_) {
            strategy->addExchange(name, exchange);
        }
    }
    
    std::cout << "Strategy '" << strategy->getName() << "' added to engine" << std::endl;
}

void StrategyEngine::removeStrategy(const std::string& strategy_name) {
    std::lock_guard<std::mutex> lock(strategies_mutex_);
    
    auto it = strategies_.find(strategy_name);
    if (it != strategies_.end()) {
        // Stop strategy if running
        if (it->second->getStatus() == StrategyStatus::RUNNING) {
            it->second->stop();
        }
        
        strategies_.erase(it);
        std::cout << "Strategy '" << strategy_name << "' removed from engine" << std::endl;
    }
}

std::shared_ptr<BaseStrategy> StrategyEngine::getStrategy(const std::string& name) {
    std::lock_guard<std::mutex> lock(strategies_mutex_);
    auto it = strategies_.find(name);
    if (it != strategies_.end()) {
        return it->second;
    }
    return nullptr;
}

std::vector<std::shared_ptr<BaseStrategy>> StrategyEngine::getAllStrategies() {
    std::lock_guard<std::mutex> lock(strategies_mutex_);
    std::vector<std::shared_ptr<BaseStrategy>> result;
    result.reserve(strategies_.size());
    
    for (const auto& [name, strategy] : strategies_) {
        result.push_back(strategy);
    }
    
    return result;
}

void StrategyEngine::start() {
    if (running_) {
        std::cout << "StrategyEngine already running" << std::endl;
        return;
    }
    
    running_ = true;
    paused_ = false;
    
    // Start engine thread
    engine_thread_ = std::thread(&StrategyEngine::runEngine, this);
    
    std::cout << "StrategyEngine started" << std::endl;
}

void StrategyEngine::stop() {
    if (!running_) {
        return;
    }
    
    running_ = false;
    
    // Stop all strategies
    {
        std::lock_guard<std::mutex> lock(strategies_mutex_);
        for (auto& [name, strategy] : strategies_) {
            if (strategy->getStatus() == StrategyStatus::RUNNING) {
                strategy->stop();
            }
        }
    }
    
    // Wait for engine thread to finish
    if (engine_thread_.joinable()) {
        engine_thread_.join();
    }
    
    std::cout << "StrategyEngine stopped" << std::endl;
}

void StrategyEngine::pause() {
    paused_ = true;
    
    // Pause all strategies
    std::lock_guard<std::mutex> lock(strategies_mutex_);
    for (auto& [name, strategy] : strategies_) {
        if (strategy->getStatus() == StrategyStatus::RUNNING) {
            strategy->pause();
        }
    }
    
    std::cout << "StrategyEngine paused" << std::endl;
}

void StrategyEngine::resume() {
    paused_ = false;
    
    // Resume all strategies
    std::lock_guard<std::mutex> lock(strategies_mutex_);
    for (auto& [name, strategy] : strategies_) {
        if (strategy->getStatus() == StrategyStatus::RUNNING) {
            strategy->resume();
        }
    }
    
    std::cout << "StrategyEngine resumed" << std::endl;
}

void StrategyEngine::startStrategy(const std::string& name) {
    auto strategy = getStrategy(name);
    if (strategy) {
        strategy->start();
        std::cout << "Strategy '" << name << "' started" << std::endl;
    }
}

void StrategyEngine::stopStrategy(const std::string& name) {
    auto strategy = getStrategy(name);
    if (strategy) {
        strategy->stop();
        std::cout << "Strategy '" << name << "' stopped" << std::endl;
    }
}

void StrategyEngine::pauseStrategy(const std::string& name) {
    auto strategy = getStrategy(name);
    if (strategy) {
        strategy->pause();
        std::cout << "Strategy '" << name << "' paused" << std::endl;
    }
}

void StrategyEngine::resumeStrategy(const std::string& name) {
    auto strategy = getStrategy(name);
    if (strategy) {
        strategy->resume();
        std::cout << "Strategy '" << name << "' resumed" << std::endl;
    }
}

void StrategyEngine::onTick(const std::string& symbol, const Trade& tick) {
    if (paused_) return;
    
    distributeEvent([symbol, tick](std::shared_ptr<BaseStrategy> strategy) {
        if (strategy->getStatus() == StrategyStatus::RUNNING) {
            strategy->onTick(symbol, tick);
        }
    });
}

void StrategyEngine::onOrderFill(const OrderFill& fill) {
    if (paused_) return;
    
    distributeEvent([fill](std::shared_ptr<BaseStrategy> strategy) {
        if (strategy->getStatus() == StrategyStatus::RUNNING) {
            strategy->onOrderFill(fill);
        }
    });
}

void StrategyEngine::onOrderReject(const OrderReject& reject) {
    if (paused_) return;
    
    distributeEvent([reject](std::shared_ptr<BaseStrategy> strategy) {
        if (strategy->getStatus() == StrategyStatus::RUNNING) {
            strategy->onOrderReject(reject);
        }
    });
}

void StrategyEngine::onBalanceUpdate(const Balance& balance) {
    if (paused_) return;
    
    distributeEvent([balance](std::shared_ptr<BaseStrategy> strategy) {
        if (strategy->getStatus() == StrategyStatus::RUNNING) {
            strategy->onBalanceUpdate(balance);
        }
    });
}

void StrategyEngine::onPositionUpdate(const Position& position) {
    if (paused_) return;
    
    distributeEvent([position](std::shared_ptr<BaseStrategy> strategy) {
        if (strategy->getStatus() == StrategyStatus::RUNNING) {
            strategy->onPositionUpdate(position);
        }
    });
}

nlohmann::json StrategyEngine::getEngineMetrics() const {
    nlohmann::json metrics;
    
    std::lock_guard<std::mutex> lock(strategies_mutex_);
    
    metrics["total_strategies"] = strategies_.size();
    metrics["running_strategies"] = 0;
    metrics["paused_strategies"] = 0;
    metrics["stopped_strategies"] = 0;
    
    for (const auto& [name, strategy] : strategies_) {
        switch (strategy->getStatus()) {
            case StrategyStatus::RUNNING:
                metrics["running_strategies"] = metrics["running_strategies"].get<int>() + 1;
                break;
            case StrategyStatus::STOPPED:
                metrics["stopped_strategies"] = metrics["stopped_strategies"].get<int>() + 1;
                break;
            default:
                metrics["paused_strategies"] = metrics["paused_strategies"].get<int>() + 1;
                break;
        }
    }
    
    metrics["engine_running"] = running_.load();
    metrics["engine_paused"] = paused_.load();
    
    return metrics;
}

nlohmann::json StrategyEngine::getStrategyMetrics(const std::string& name) const {
    auto strategy = const_cast<StrategyEngine*>(this)->getStrategy(name);
    if (strategy) {
        auto metrics = strategy->getMetrics();
        
        nlohmann::json json_metrics;
        json_metrics["name"] = name;
        json_metrics["status"] = static_cast<int>(strategy->getStatus());
        json_metrics["total_trades"] = metrics.total_trades;
        json_metrics["winning_trades"] = metrics.winning_trades;
        json_metrics["losing_trades"] = metrics.losing_trades;
        json_metrics["total_pnl"] = metrics.total_pnl;
        json_metrics["win_rate"] = metrics.win_rate;
        json_metrics["avg_win"] = metrics.avg_win;
        json_metrics["avg_loss"] = metrics.avg_loss;
        json_metrics["max_drawdown"] = metrics.max_drawdown;
        json_metrics["sharpe_ratio"] = metrics.sharpe_ratio;
        
        return json_metrics;
    }
    
    return nlohmann::json{};
}

nlohmann::json StrategyEngine::getAllStrategyMetrics() const {
    nlohmann::json all_metrics;
    
    std::lock_guard<std::mutex> lock(strategies_mutex_);
    for (const auto& [name, strategy] : strategies_) {
        all_metrics[name] = getStrategyMetrics(name);
    }
    
    return all_metrics;
}

void StrategyEngine::setRiskManager(std::shared_ptr<RiskManager> risk_manager) {
    risk_manager_ = risk_manager;
    
    // Update all strategies
    std::lock_guard<std::mutex> lock(strategies_mutex_);
    for (auto& [name, strategy] : strategies_) {
        strategy->setRiskManager(risk_manager);
    }
}

void StrategyEngine::setPortfolioManager(std::shared_ptr<PortfolioManager> portfolio_manager) {
    portfolio_manager_ = portfolio_manager;
    
    // Update all strategies
    std::lock_guard<std::mutex> lock(strategies_mutex_);
    for (auto& [name, strategy] : strategies_) {
        strategy->setPortfolioManager(portfolio_manager);
    }
}

void StrategyEngine::addExchange(const std::string& exchange_name, 
                                 std::shared_ptr<ExchangeManager> exchange) {
    {
        std::lock_guard<std::mutex> lock(exchanges_mutex_);
        exchanges_[exchange_name] = exchange;
    }
    
    // Update all strategies
    std::lock_guard<std::mutex> lock(strategies_mutex_);
    for (auto& [name, strategy] : strategies_) {
        strategy->addExchange(exchange_name, exchange);
    }
}

void StrategyEngine::runEngine() {
    while (running_) {
        if (!paused_) {
            // Engine main loop - process events, update strategies, etc.
            // This is a simplified version - in production would handle event queues
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
    }
}

void StrategyEngine::distributeEvent(const std::function<void(std::shared_ptr<BaseStrategy>)>& event) {
    std::lock_guard<std::mutex> lock(strategies_mutex_);
    for (auto& [name, strategy] : strategies_) {
        try {
            event(strategy);
        } catch (const std::exception& e) {
            std::cout << "Error in strategy '" << name << "': " << e.what() << std::endl;
        }
    }
}

} // namespace moneybot
