#include "risk_manager.h"
#include <algorithm>
#include <cmath>

namespace moneybot {

RiskLimits::RiskLimits(const nlohmann::json& j) {
    max_position_size = j["max_position_size"].get<double>();
    max_order_size = j["max_order_size"].get<double>();
    max_daily_loss = j["max_daily_loss"].get<double>();
    max_drawdown = j["max_drawdown"].get<double>();
    max_orders_per_minute = j["max_orders_per_minute"].get<int>();
    min_spread = j["min_spread"].get<double>();
    max_slippage = j["max_slippage"].get<double>();
}

RiskManager::RiskManager(std::shared_ptr<Logger> logger, const nlohmann::json& config)
    : logger_(logger), limits_(config["risk"]), total_realized_pnl_(0.0), 
      peak_equity_(0.0), current_equity_(0.0), emergency_stopped_(false),
      session_start_(std::chrono::system_clock::now()),
      last_pnl_update_(std::chrono::system_clock::now()) {
    logger_->getLogger()->info("RiskManager initialized with limits: max_position={}, max_order={}, max_daily_loss={}",
                              limits_.max_position_size, limits_.max_order_size, limits_.max_daily_loss);
}

bool RiskManager::checkOrderRisk(const Order& order) {
    if (emergency_stopped_) {
        logger_->getLogger()->warn("Order rejected: Emergency stop active");
        return false;
    }
    
    if (order.quantity > limits_.max_order_size) {
        logger_->getLogger()->warn("Order rejected: Quantity {} exceeds max order size {}", 
                                  order.quantity, limits_.max_order_size);
        return false;
    }
    
    if (!checkOrderRate(order.symbol)) {
        logger_->getLogger()->warn("Order rejected: Rate limit exceeded for symbol {}", order.symbol);
        return false;
    }
    
    return true;
}

bool RiskManager::checkPositionRisk(const std::string& symbol, double new_position) {
    if (emergency_stopped_) return false;
    
    if (std::abs(new_position) > limits_.max_position_size) {
        logger_->getLogger()->warn("Position risk check failed: {} exceeds max position size {}", 
                                  new_position, limits_.max_position_size);
        return false;
    }
    
    return true;
}

bool RiskManager::checkDailyLoss(double current_pnl) {
    if (emergency_stopped_) return false;
    
    if (current_pnl < limits_.max_daily_loss) {
        logger_->getLogger()->error("Daily loss limit exceeded: {} < {}", current_pnl, limits_.max_daily_loss);
        emergencyStop();
        return false;
    }
    
    return true;
}

bool RiskManager::checkDrawdown(double current_drawdown) {
    if (emergency_stopped_) return false;
    
    if (current_drawdown < limits_.max_drawdown) {
        logger_->getLogger()->error("Drawdown limit exceeded: {} < {}", current_drawdown, limits_.max_drawdown);
        emergencyStop();
        return false;
    }
    
    return true;
}

bool RiskManager::checkOrderRate(const std::string& symbol) {
    std::lock_guard<std::mutex> lock(rates_mutex_);
    
    auto now = std::chrono::system_clock::now();
    auto& rate_info = order_rates_[symbol];
    
    // Reset window if more than 1 minute has passed
    if (now - rate_info.window_start > std::chrono::minutes(1)) {
        rate_info.order_count = 0;
        rate_info.window_start = now;
    }
    
    if (rate_info.order_count >= limits_.max_orders_per_minute) {
        return false;
    }
    
    rate_info.order_count++;
    return true;
}

void RiskManager::updatePosition(const std::string& symbol, double quantity, double price) {
    std::lock_guard<std::mutex> lock(positions_mutex_);
    
    auto& pos = positions_[symbol];
    double old_quantity = pos.quantity;
    
    // Update average price
    if (old_quantity + quantity != 0) {
        pos.avg_price = (pos.avg_price * old_quantity + price * quantity) / (old_quantity + quantity);
    }
    
    pos.quantity += quantity;
    pos.last_update = std::chrono::system_clock::now();
    
    logger_->getLogger()->debug("Position updated: {} = {} @ {}", symbol, pos.quantity, pos.avg_price);
}

void RiskManager::updatePnL(const std::string& symbol, double pnl) {
    std::lock_guard<std::mutex> lock(positions_mutex_);
    
    auto& pos = positions_[symbol];
    pos.unrealized_pnl = pnl;
    last_pnl_update_ = std::chrono::system_clock::now();
    
    // Update total PnL
    total_realized_pnl_ += pnl;
    current_equity_ = total_realized_pnl_;
    
    // Update peak equity
    if (current_equity_ > peak_equity_) {
        peak_equity_ = current_equity_;
    }
    
    // Check risk limits
    checkDailyLoss(current_equity_);
    checkDrawdown(calculateDrawdown());
}

void RiskManager::setRiskLimits(const RiskLimits& limits) {
    std::lock_guard<std::mutex> lock(limits_mutex_);
    limits_ = limits;
    logger_->getLogger()->info("Risk limits updated");
}

RiskLimits RiskManager::getRiskLimits() const {
    std::lock_guard<std::mutex> lock(limits_mutex_);
    return limits_;
}

void RiskManager::emergencyStop() {
    emergency_stopped_ = true;
    logger_->getLogger()->error("EMERGENCY STOP ACTIVATED");
}

void RiskManager::resume() {
    emergency_stopped_ = false;
    logger_->getLogger()->info("Risk manager resumed");
}

bool RiskManager::isEmergencyStopped() const {
    return emergency_stopped_;
}

nlohmann::json RiskManager::getRiskReport() const {
    std::lock_guard<std::mutex> lock_pos(positions_mutex_);
    std::lock_guard<std::mutex> lock_limits(limits_mutex_);
    
    nlohmann::json report;
    report["emergency_stopped"] = emergency_stopped_;
    report["total_realized_pnl"] = total_realized_pnl_;
    report["current_equity"] = current_equity_;
    report["peak_equity"] = peak_equity_;
    report["drawdown"] = calculateDrawdown();
    report["daily_pnl"] = calculateDailyPnL();
    report["session_duration"] = std::chrono::duration_cast<std::chrono::hours>(
        std::chrono::system_clock::now() - session_start_).count();
    
    // Position summary
    nlohmann::json positions;
    for (const auto& [symbol, pos] : positions_) {
        positions[symbol] = {
            {"quantity", pos.quantity},
            {"avg_price", pos.avg_price},
            {"unrealized_pnl", pos.unrealized_pnl}
        };
    }
    report["positions"] = positions;
    
    // Risk limits
    report["limits"] = {
        {"max_position_size", limits_.max_position_size},
        {"max_order_size", limits_.max_order_size},
        {"max_daily_loss", limits_.max_daily_loss},
        {"max_drawdown", limits_.max_drawdown}
    };
    
    return report;
}

double RiskManager::calculateDrawdown() const {
    if (peak_equity_ <= 0) return 0.0;
    return (current_equity_ - peak_equity_) / peak_equity_ * 100.0;
}

double RiskManager::calculateDailyPnL() const {
    // For simplicity, return total PnL since session start
    // In a real system, you'd track daily boundaries
    return total_realized_pnl_;
}

} // namespace moneybot 