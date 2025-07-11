#include "core/portfolio_manager.h"
#include "logger.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <numeric>
#include <iostream>

namespace moneybot {

PortfolioManager::PortfolioManager(std::shared_ptr<Logger> logger, const nlohmann::json& config)
    : logger_(logger), session_start_value_(0.0), day_start_value_(0.0), 
      total_realized_pnl_(0.0), base_currency_("USD"), auto_save_snapshots_(true),
      max_history_snapshots_(1000) {
    
    // Load configuration
    if (config.contains("portfolio")) {
        const auto& portfolio_config = config["portfolio"];
        if (portfolio_config.contains("base_currency")) {
            base_currency_ = portfolio_config["base_currency"];
        }
        if (portfolio_config.contains("auto_save_snapshots")) {
            auto_save_snapshots_ = portfolio_config["auto_save_snapshots"];
        }
        if (portfolio_config.contains("max_history_snapshots")) {
            max_history_snapshots_ = portfolio_config["max_history_snapshots"];
        }
    }
    
    // Initialize timestamps
    session_start_time_ = std::chrono::system_clock::now();
    day_start_time_ = session_start_time_;
    
    std::cout << "PortfolioManager initialized with base currency: " << base_currency_ << std::endl;
}

void PortfolioManager::updateBalance(const Balance& balance) {
    std::lock_guard<std::mutex> lock(balances_mutex_);
    balances_[balance.asset] = balance;
    
    std::cout << "Balance updated: " << balance.asset << " = " << balance.total 
              << " (free: " << balance.free << ", locked: " << balance.locked << ")" << std::endl;
    
    // Notify callback
    {
        std::lock_guard<std::mutex> callback_lock(callbacks_mutex_);
        if (balance_callback_) {
            balance_callback_(balance);
        }
    }
    
    notifyPortfolioUpdate();
}

void PortfolioManager::updateBalances(const std::vector<Balance>& balances) {
    std::lock_guard<std::mutex> lock(balances_mutex_);
    
    for (const auto& balance : balances) {
        balances_[balance.asset] = balance;
    }
    
    std::cout << "Updated " << balances.size() << " balances" << std::endl;
    notifyPortfolioUpdate();
}

Balance PortfolioManager::getBalance(const std::string& asset) const {
    std::lock_guard<std::mutex> lock(balances_mutex_);
    auto it = balances_.find(asset);
    if (it != balances_.end()) {
        return it->second;
    }
    return Balance{}; // Return empty balance if not found
}

std::vector<Balance> PortfolioManager::getAllBalances() const {
    std::lock_guard<std::mutex> lock(balances_mutex_);
    std::vector<Balance> result;
    result.reserve(balances_.size());
    
    for (const auto& [asset, balance] : balances_) {
        result.push_back(balance);
    }
    
    return result;
}

double PortfolioManager::getTotalValue() const {
    std::lock_guard<std::mutex> lock_balances(balances_mutex_);
    std::lock_guard<std::mutex> lock_positions(positions_mutex_);
    
    double total = 0.0;
    
    // Add balance values
    for (const auto& [asset, balance] : balances_) {
        if (asset == base_currency_) {
            total += balance.total;
        } else {
            // Convert to base currency (simplified - would need proper exchange rates)
            double price = getPrice(asset + base_currency_);
            total += balance.total * price;
        }
    }
    
    // Add position values
    for (const auto& [symbol, position] : positions_) {
        total += calculatePositionValue(position);
    }
    
    return total;
}

double PortfolioManager::getAvailableBalance(const std::string& asset) const {
    auto balance = getBalance(asset);
    return balance.free;
}

void PortfolioManager::updatePosition(const Position& position) {
    std::lock_guard<std::mutex> lock(positions_mutex_);
    
    if (position.quantity == 0.0) {
        // Remove position if quantity is zero
        positions_.erase(position.symbol);
        std::cout << "Position closed: " << position.symbol << std::endl;
    } else {
        positions_[position.symbol] = position;
        std::cout << "Position updated: " << position.symbol << " = " << position.quantity 
                  << " @ " << position.avg_price << std::endl;
    }
    
    // Notify callback
    {
        std::lock_guard<std::mutex> callback_lock(callbacks_mutex_);
        if (position_callback_) {
            position_callback_(position);
        }
    }
    
    notifyPortfolioUpdate();
}

void PortfolioManager::updatePositions(const std::vector<Position>& positions) {
    std::lock_guard<std::mutex> lock(positions_mutex_);
    
    // Clear existing positions
    positions_.clear();
    
    // Add new positions
    for (const auto& position : positions) {
        if (position.quantity != 0.0) {
            positions_[position.symbol] = position;
        }
    }
    
    std::cout << "Updated " << positions_.size() << " positions" << std::endl;
    notifyPortfolioUpdate();
}

Position PortfolioManager::getPosition(const std::string& symbol) const {
    std::lock_guard<std::mutex> lock(positions_mutex_);
    auto it = positions_.find(symbol);
    if (it != positions_.end()) {
        return it->second;
    }
    return Position{}; // Return empty position if not found
}

std::vector<Position> PortfolioManager::getAllPositions() const {
    std::lock_guard<std::mutex> lock(positions_mutex_);
    std::vector<Position> result;
    result.reserve(positions_.size());
    
    for (const auto& [symbol, position] : positions_) {
        result.push_back(position);
    }
    
    return result;
}

std::vector<Position> PortfolioManager::getActivePositions() const {
    std::lock_guard<std::mutex> lock(positions_mutex_);
    std::vector<Position> result;
    
    for (const auto& [symbol, position] : positions_) {
        if (position.quantity != 0.0) {
            result.push_back(position);
        }
    }
    
    return result;
}

void PortfolioManager::recordTrade(const Trade& trade) {
    std::lock_guard<std::mutex> lock(trades_mutex_);
    
    TradeRecord record;
    record.trade = trade;
    record.timestamp = trade.timestamp;
    
    // Calculate PnL (simplified)
    auto position = getPosition(trade.symbol);
    if (position.quantity != 0.0) {
        record.pnl = (trade.price - position.avg_price) * trade.quantity;
        if (trade.side == OrderSide::SELL) {
            record.pnl = -record.pnl;
        }
    } else {
        record.pnl = 0.0;
    }
    
    trade_history_.push_back(record);
    
    // Update realized PnL
    if (record.pnl != 0.0) {
        total_realized_pnl_ += record.pnl;
    }
    
    std::cout << "Trade recorded: " << trade.symbol << " " << trade.quantity 
              << " @ " << trade.price << " PnL: " << record.pnl << std::endl;
}

void PortfolioManager::recordOrderFill(const OrderFill& fill) {
    // Convert fill to trade format
    Trade trade;
    trade.trade_id = fill.trade_id;
    trade.symbol = ""; // Would need to get this from order context
    trade.price = fill.price;
    trade.quantity = fill.quantity;
    trade.timestamp = fill.timestamp;
    
    recordTrade(trade);
}

std::vector<Trade> PortfolioManager::getTradeHistory(const std::string& symbol) const {
    std::lock_guard<std::mutex> lock(trades_mutex_);
    std::vector<Trade> result;
    
    for (const auto& record : trade_history_) {
        if (symbol.empty() || record.trade.symbol == symbol) {
            result.push_back(record.trade);
        }
    }
    
    return result;
}

double PortfolioManager::getUnrealizedPnL() const {
    std::lock_guard<std::mutex> lock(positions_mutex_);
    double total_unrealized = 0.0;
    
    for (const auto& [symbol, position] : positions_) {
        total_unrealized += position.unrealized_pnl;
    }
    
    return total_unrealized;
}

double PortfolioManager::getRealizedPnL() const {
    return total_realized_pnl_;
}

double PortfolioManager::getDayPnL() const {
    return getTotalValue() - day_start_value_;
}

double PortfolioManager::getTotalPnL() const {
    return getRealizedPnL() + getUnrealizedPnL();
}

void PortfolioManager::resetDayPnL() {
    day_start_value_ = getTotalValue();
    day_start_time_ = std::chrono::system_clock::now();
    std::cout << "Day PnL reset. New baseline: " << day_start_value_ << std::endl;
}

PortfolioMetrics PortfolioManager::calculateMetrics() const {
    std::lock_guard<std::mutex> lock(trades_mutex_);
    
    PortfolioMetrics metrics = {};
    
    if (trade_history_.empty()) {
        return metrics;
    }
    
    // Calculate basic metrics
    metrics.total_trades = static_cast<int>(trade_history_.size());
    
    double total_pnl = 0.0;
    double total_wins = 0.0;
    double total_losses = 0.0;
    
    for (const auto& record : trade_history_) {
        total_pnl += record.pnl;
        
        if (record.pnl > 0) {
            metrics.winning_trades++;
            total_wins += record.pnl;
        } else if (record.pnl < 0) {
            metrics.losing_trades++;
            total_losses += std::abs(record.pnl);
        }
    }
    
    // Calculate derived metrics
    if (metrics.total_trades > 0) {
        metrics.win_rate = static_cast<double>(metrics.winning_trades) / metrics.total_trades;
    }
    
    if (metrics.winning_trades > 0) {
        metrics.avg_win = total_wins / metrics.winning_trades;
    }
    
    if (metrics.losing_trades > 0) {
        metrics.avg_loss = total_losses / metrics.losing_trades;
    }
    
    if (metrics.avg_loss > 0) {
        metrics.profit_factor = metrics.avg_win / metrics.avg_loss;
    }
    
    // Calculate returns
    if (session_start_value_ > 0) {
        metrics.total_return = (getTotalValue() - session_start_value_) / session_start_value_;
    }
    
    if (day_start_value_ > 0) {
        metrics.daily_return = (getTotalValue() - day_start_value_) / day_start_value_;
    }
    
    metrics.max_drawdown = calculateMaxDrawdown();
    metrics.sharpe_ratio = calculateSharpeRatio();
    
    return metrics;
}

PortfolioSnapshot PortfolioManager::getSnapshot() const {
    PortfolioSnapshot snapshot;
    snapshot.timestamp = std::chrono::system_clock::now();
    snapshot.total_value = getTotalValue();
    snapshot.unrealized_pnl = getUnrealizedPnL();
    snapshot.realized_pnl = getRealizedPnL();
    snapshot.day_pnl = getDayPnL();
    snapshot.total_pnl = getTotalPnL();
    snapshot.balances = balances_;
    snapshot.positions = positions_;
    
    return snapshot;
}

std::vector<PortfolioSnapshot> PortfolioManager::getHistorySnapshots(int count) const {
    std::lock_guard<std::mutex> lock(snapshots_mutex_);
    
    std::vector<PortfolioSnapshot> result;
    int start_idx = std::max(0, static_cast<int>(history_snapshots_.size()) - count);
    
    for (size_t i = start_idx; i < history_snapshots_.size(); ++i) {
        result.push_back(history_snapshots_[i]);
    }
    
    return result;
}

double PortfolioManager::calculateVaR(double confidence) const {
    // Simplified VaR calculation using historical method
    std::lock_guard<std::mutex> lock(snapshots_mutex_);
    
    if (history_snapshots_.size() < 2) {
        return 0.0;
    }
    
    std::vector<double> returns;
    for (size_t i = 1; i < history_snapshots_.size(); ++i) {
        double prev_value = history_snapshots_[i-1].total_value;
        double curr_value = history_snapshots_[i].total_value;
        
        if (prev_value > 0) {
            returns.push_back((curr_value - prev_value) / prev_value);
        }
    }
    
    if (returns.empty()) {
        return 0.0;
    }
    
    std::sort(returns.begin(), returns.end());
    size_t index = static_cast<size_t>((1.0 - confidence) * returns.size());
    
    return returns[index] * getTotalValue();
}

double PortfolioManager::calculateMaxDrawdown() const {
    std::lock_guard<std::mutex> lock(snapshots_mutex_);
    
    if (history_snapshots_.size() < 2) {
        return 0.0;
    }
    
    double peak = history_snapshots_[0].total_value;
    double max_drawdown = 0.0;
    
    for (const auto& snapshot : history_snapshots_) {
        if (snapshot.total_value > peak) {
            peak = snapshot.total_value;
        }
        
        double drawdown = (peak - snapshot.total_value) / peak;
        max_drawdown = std::max(max_drawdown, drawdown);
    }
    
    return max_drawdown;
}

double PortfolioManager::calculateSharpeRatio() const {
    // Simplified Sharpe ratio calculation
    std::lock_guard<std::mutex> lock(snapshots_mutex_);
    
    if (history_snapshots_.size() < 2) {
        return 0.0;
    }
    
    std::vector<double> returns;
    for (size_t i = 1; i < history_snapshots_.size(); ++i) {
        double prev_value = history_snapshots_[i-1].total_value;
        double curr_value = history_snapshots_[i].total_value;
        
        if (prev_value > 0) {
            returns.push_back((curr_value - prev_value) / prev_value);
        }
    }
    
    if (returns.empty()) {
        return 0.0;
    }
    
    double mean_return = std::accumulate(returns.begin(), returns.end(), 0.0) / returns.size();
    
    double variance = 0.0;
    for (double ret : returns) {
        variance += (ret - mean_return) * (ret - mean_return);
    }
    variance /= returns.size();
    
    double std_dev = std::sqrt(variance);
    if (std_dev == 0) {
        return 0.0;
    }
    
    // Assuming risk-free rate of 0 for simplicity
    return mean_return / std_dev;
}

void PortfolioManager::setPortfolioUpdateCallback(PortfolioUpdateCallback callback) {
    std::lock_guard<std::mutex> lock(callbacks_mutex_);
    portfolio_callback_ = callback;
}

void PortfolioManager::setBalanceUpdateCallback(BalanceUpdateCallback callback) {
    std::lock_guard<std::mutex> lock(callbacks_mutex_);
    balance_callback_ = callback;
}

void PortfolioManager::setPositionUpdateCallback(PositionUpdateCallback callback) {
    std::lock_guard<std::mutex> lock(callbacks_mutex_);
    position_callback_ = callback;
}

void PortfolioManager::saveSnapshot(const std::string& filename) const {
    std::string file = filename;
    if (file.empty()) {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        file = "portfolio_snapshot_" + std::to_string(time_t) + ".json";
    }
    
    nlohmann::json json_snapshot;
    auto snapshot = getSnapshot();
    
    json_snapshot["timestamp"] = std::chrono::duration_cast<std::chrono::milliseconds>(
        snapshot.timestamp.time_since_epoch()).count();
    json_snapshot["total_value"] = snapshot.total_value;
    json_snapshot["unrealized_pnl"] = snapshot.unrealized_pnl;
    json_snapshot["realized_pnl"] = snapshot.realized_pnl;
    json_snapshot["day_pnl"] = snapshot.day_pnl;
    json_snapshot["total_pnl"] = snapshot.total_pnl;
    
    // Save balances
    for (const auto& [asset, balance] : snapshot.balances) {
        json_snapshot["balances"][asset] = {
            {"free", balance.free},
            {"locked", balance.locked},
            {"total", balance.total}
        };
    }
    
    // Save positions
    for (const auto& [symbol, position] : snapshot.positions) {
        json_snapshot["positions"][symbol] = {
            {"quantity", position.quantity},
            {"avg_price", position.avg_price},
            {"unrealized_pnl", position.unrealized_pnl},
            {"realized_pnl", position.realized_pnl}
        };
    }
    
    std::ofstream file_stream(file);
    file_stream << json_snapshot.dump(4);
    file_stream.close();
    
    std::cout << "Portfolio snapshot saved to: " << file << std::endl;
}

void PortfolioManager::loadSnapshot(const std::string& filename) {
    std::ifstream file_stream(filename);
    if (!file_stream.is_open()) {
        std::cerr << "Failed to open snapshot file: " << filename << std::endl;
        return;
    }
    
    nlohmann::json json_snapshot;
    file_stream >> json_snapshot;
    file_stream.close();
    
    // Load balances
    if (json_snapshot.contains("balances")) {
        std::lock_guard<std::mutex> lock(balances_mutex_);
        balances_.clear();
        
        for (const auto& [asset, balance_data] : json_snapshot["balances"].items()) {
            Balance balance;
            balance.asset = asset;
            balance.free = balance_data["free"];
            balance.locked = balance_data["locked"];
            balance.total = balance_data["total"];
            balances_[asset] = balance;
        }
    }
    
    // Load positions
    if (json_snapshot.contains("positions")) {
        std::lock_guard<std::mutex> lock(positions_mutex_);
        positions_.clear();
        
        for (const auto& [symbol, position_data] : json_snapshot["positions"].items()) {
            Position position;
            position.symbol = symbol;
            position.quantity = position_data["quantity"];
            position.avg_price = position_data["avg_price"];
            position.unrealized_pnl = position_data["unrealized_pnl"];
            position.realized_pnl = position_data["realized_pnl"];
            positions_[symbol] = position;
        }
    }
    
    std::cout << "Portfolio snapshot loaded from: " << filename << std::endl;
}

nlohmann::json PortfolioManager::getPortfolioReport() const {
    nlohmann::json report;
    auto snapshot = getSnapshot();
    auto metrics = calculateMetrics();
    
    report["timestamp"] = std::chrono::duration_cast<std::chrono::milliseconds>(
        snapshot.timestamp.time_since_epoch()).count();
    report["total_value"] = snapshot.total_value;
    report["unrealized_pnl"] = snapshot.unrealized_pnl;
    report["realized_pnl"] = snapshot.realized_pnl;
    report["day_pnl"] = snapshot.day_pnl;
    report["total_pnl"] = snapshot.total_pnl;
    report["base_currency"] = base_currency_;
    
    // Add metrics
    report["metrics"] = {
        {"total_return", metrics.total_return},
        {"daily_return", metrics.daily_return},
        {"max_drawdown", metrics.max_drawdown},
        {"sharpe_ratio", metrics.sharpe_ratio},
        {"win_rate", metrics.win_rate},
        {"profit_factor", metrics.profit_factor},
        {"total_trades", metrics.total_trades}
    };
    
    // Add balances
    report["balances"] = nlohmann::json::object();
    for (const auto& [asset, balance] : snapshot.balances) {
        report["balances"][asset] = {
            {"free", balance.free},
            {"locked", balance.locked},
            {"total", balance.total}
        };
    }
    
    // Add positions
    report["positions"] = nlohmann::json::object();
    for (const auto& [symbol, position] : snapshot.positions) {
        report["positions"][symbol] = {
            {"quantity", position.quantity},
            {"avg_price", position.avg_price},
            {"unrealized_pnl", position.unrealized_pnl},
            {"realized_pnl", position.realized_pnl}
        };
    }
    
    return report;
}

nlohmann::json PortfolioManager::getPerformanceReport() const {
    nlohmann::json report;
    auto metrics = calculateMetrics();
    
    report["performance"] = {
        {"total_return", metrics.total_return},
        {"daily_return", metrics.daily_return},
        {"max_drawdown", metrics.max_drawdown},
        {"sharpe_ratio", metrics.sharpe_ratio},
        {"win_rate", metrics.win_rate},
        {"avg_win", metrics.avg_win},
        {"avg_loss", metrics.avg_loss},
        {"profit_factor", metrics.profit_factor},
        {"total_trades", metrics.total_trades},
        {"winning_trades", metrics.winning_trades},
        {"losing_trades", metrics.losing_trades}
    };
    
    report["risk_metrics"] = {
        {"var_95", calculateVaR(0.95)},
        {"var_99", calculateVaR(0.99)},
        {"max_drawdown", calculateMaxDrawdown()}
    };
    
    return report;
}

void PortfolioManager::clear() {
    std::lock_guard<std::mutex> lock_balances(balances_mutex_);
    std::lock_guard<std::mutex> lock_positions(positions_mutex_);
    std::lock_guard<std::mutex> lock_trades(trades_mutex_);
    std::lock_guard<std::mutex> lock_snapshots(snapshots_mutex_);
    
    balances_.clear();
    positions_.clear();
    trade_history_.clear();
    history_snapshots_.clear();
    
    total_realized_pnl_ = 0.0;
    session_start_value_ = 0.0;
    day_start_value_ = 0.0;
    
    std::cout << "Portfolio cleared" << std::endl;
}

void PortfolioManager::setBaseCurrency(const std::string& currency) {
    base_currency_ = currency;
    std::cout << "Base currency set to: " << currency << std::endl;
}

std::string PortfolioManager::getBaseCurrency() const {
    return base_currency_;
}

// Private methods implementation

double PortfolioManager::calculatePositionValue(const Position& position) const {
    // Simplified position value calculation
    return position.quantity * position.avg_price;
}

double PortfolioManager::calculateUnrealizedPnLForPosition(const Position& position) const {
    // Get current market price
    double current_price = getPrice(position.symbol);
    if (current_price <= 0) {
        return position.unrealized_pnl; // Fall back to stored value
    }
    
    return (current_price - position.avg_price) * position.quantity;
}

void PortfolioManager::updatePortfolioMetrics() {
    // This would be called periodically to update metrics
    // For now, metrics are calculated on-demand
}

void PortfolioManager::notifyPortfolioUpdate() {
    std::lock_guard<std::mutex> lock(callbacks_mutex_);
    if (portfolio_callback_) {
        portfolio_callback_(getSnapshot());
    }
    
    // Save snapshot if auto-save is enabled
    if (auto_save_snapshots_) {
        std::lock_guard<std::mutex> snapshots_lock(snapshots_mutex_);
        history_snapshots_.push_back(getSnapshot());
        
        // Limit snapshot history
        if (history_snapshots_.size() > max_history_snapshots_) {
            history_snapshots_.erase(history_snapshots_.begin());
        }
    }
}

double PortfolioManager::getPrice(const std::string& symbol) const {
    std::lock_guard<std::mutex> lock(prices_mutex_);
    auto it = price_cache_.find(symbol);
    if (it != price_cache_.end()) {
        return it->second;
    }
    return 1.0; // Default price for base currency pairs
}

void PortfolioManager::updatePrice(const std::string& symbol, double price) {
    std::lock_guard<std::mutex> lock(prices_mutex_);
    price_cache_[symbol] = price;
    price_timestamps_[symbol] = std::chrono::system_clock::now();
}

} // namespace moneybot
