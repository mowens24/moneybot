#include "strategy/cross_exchange_arbitrage_strategy.h"
#include <algorithm>
#include <cmath>

namespace moneybot {
namespace strategy {

CrossExchangeArbitrageStrategy::CrossExchangeArbitrageStrategy(
    const std::string& name,
    const nlohmann::json& config,
    std::shared_ptr<Logger> logger,
    std::shared_ptr<RiskManager> risk_manager,
    std::shared_ptr<ExchangeManager> exchange_manager
) : BaseStrategy(name, config, logger), exchange_manager_(exchange_manager) {
    
    // Set risk manager
    setRiskManager(risk_manager);
    
    // Parse configuration directly from the strategy config
    min_profit_percentage_ = config.value("min_profit_bps", 15.0) / 10000.0; // Convert from bps to percentage
    max_position_size_ = config.value("max_position_size", 1000.0);
    max_order_size_ = config.value("max_order_size", 100.0);
    max_opportunities_per_minute_ = config.value("max_opportunities_per_minute", 10);
    max_latency_ = std::chrono::milliseconds(config.value("execution_timeout_ms", 3000));
    
    // Parse exchanges from config
    if (config.contains("exchange_a")) {
        monitored_exchanges_.push_back(config["exchange_a"].get<std::string>());
    }
    if (config.contains("exchange_b")) {
        monitored_exchanges_.push_back(config["exchange_b"].get<std::string>());
    }
    
    // Parse symbol from config
    if (config.contains("symbol")) {
        monitored_symbols_.push_back(config["symbol"].get<std::string>());
    }
    
    // Initialize rate limiting
    last_execution_time_ = std::chrono::system_clock::now();
    executions_this_minute_ = 0;
    minute_start_ = std::chrono::system_clock::now();
    
    logger_->getLogger()->info("CrossExchangeArbitrageStrategy '{}' initialized", name);
}

bool CrossExchangeArbitrageStrategy::initialize() {
    return true;
}

void CrossExchangeArbitrageStrategy::pause() {
    // Pause the strategy
}

void CrossExchangeArbitrageStrategy::resume() {
    // Resume the strategy
}

void CrossExchangeArbitrageStrategy::onTick(const MarketTick& tick) {
    // if (!is_active_) return;
    
    std::string exchange = getExchangeFromTick(tick);
    if (!isExchangeMonitored(exchange) || !isSymbolMonitored(tick.symbol)) {
        return;
    }
    
    // Update price information
    updatePrice(tick.symbol, exchange, tick);
    
    // Check for arbitrage opportunities
    detectArbitrageOpportunities(tick.symbol);
}

void CrossExchangeArbitrageStrategy::onOrderUpdate(const OrderUpdate& update) {
    // if (!is_active_) return;
    
    // Track order execution for arbitrage opportunities
    // This would update the position tracking and profit calculations
    std::string side_str = (update.side == OrderSide::BUY) ? "BUY" : "SELL";
    logger_->getLogger()->info("Order update for {}: {} {} @ {}", 
                  update.symbol, side_str, update.quantity, update.price);
}

void CrossExchangeArbitrageStrategy::onTradeUpdate(const TradeUpdate& update) {
    // if (!is_active_) return;
    
    // Update metrics when trades are executed
    std::string side_str = (update.side == OrderSide::BUY) ? "BUY" : "SELL";
    logger_->getLogger()->info("Trade update for {}: {} {} @ {}", 
                  update.symbol, side_str, update.quantity, update.price);
}

void CrossExchangeArbitrageStrategy::start() {
    // Strategy::start();
    
    // Reset performance metrics
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    metrics_ = PerformanceMetrics{};
    
    logger_->getLogger()->info("CrossExchangeArbitrageStrategy '{}' started", name_);
}

void CrossExchangeArbitrageStrategy::stop() {
    // Strategy::stop();
    
    // Clean up any pending positions
    cleanupStaleData();
    
    logger_->getLogger()->info("CrossExchangeArbitrageStrategy '{}' stopped", name_);
}

nlohmann::json CrossExchangeArbitrageStrategy::getMetrics() const {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    
    nlohmann::json metrics; // = Strategy::getMetrics();
    
    metrics["cross_exchange_arbitrage"] = {
        {"opportunities_detected", metrics_.opportunities_detected},
        {"opportunities_executed", metrics_.opportunities_executed},
        {"total_profit", metrics_.total_profit},
        {"total_fees", metrics_.total_fees},
        {"net_profit", metrics_.net_profit},
        {"win_rate", metrics_.win_rate},
        {"avg_profit_per_trade", metrics_.avg_profit_per_trade},
        {"max_profit", metrics_.max_profit},
        {"max_loss", metrics_.max_loss},
        {"winning_trades", metrics_.winning_trades},
        {"losing_trades", metrics_.losing_trades},
        {"execution_rate", metrics_.opportunities_executed > 0 ? 
                          static_cast<double>(metrics_.opportunities_executed) / metrics_.opportunities_detected : 0.0}
    };
    
    // Add current opportunities
    {
        std::lock_guard<std::mutex> opp_lock(opportunities_mutex_);
        metrics["current_opportunities"] = static_cast<int>(opportunities_.size());
    }
    
    return metrics;
}

nlohmann::json CrossExchangeArbitrageStrategy::getConfig() const {
    nlohmann::json config; // = Strategy::getConfig();
    
    config["cross_exchange_arbitrage"] = {
        {"min_profit_percentage", min_profit_percentage_},
        {"max_position_size", max_position_size_},
        {"max_order_size", max_order_size_},
        {"max_opportunities_per_minute", max_opportunities_per_minute_},
        {"max_latency_ms", max_latency_.count()},
        {"monitored_exchanges", monitored_exchanges_},
        {"monitored_symbols", monitored_symbols_}
    };
    
    return config;
}

void CrossExchangeArbitrageStrategy::updateConfig(const nlohmann::json& config) {
    // Strategy::updateConfig(config);
    
    if (config.contains("cross_exchange_arbitrage")) {
        auto strat_config = config["cross_exchange_arbitrage"];
        
        if (strat_config.contains("min_profit_percentage")) {
            min_profit_percentage_ = strat_config["min_profit_percentage"];
        }
        if (strat_config.contains("max_position_size")) {
            max_position_size_ = strat_config["max_position_size"];
        }
        if (strat_config.contains("max_order_size")) {
            max_order_size_ = strat_config["max_order_size"];
        }
        if (strat_config.contains("max_opportunities_per_minute")) {
            max_opportunities_per_minute_ = strat_config["max_opportunities_per_minute"];
        }
        if (strat_config.contains("max_latency_ms")) {
            max_latency_ = std::chrono::milliseconds(strat_config["max_latency_ms"]);
        }
        
        logger_->getLogger()->info("CrossExchangeArbitrageStrategy '{}' config updated", name_);
    }
}

void CrossExchangeArbitrageStrategy::updatePrice(const std::string& symbol, const std::string& exchange, const MarketTick& tick) {
    std::lock_guard<std::mutex> lock(prices_mutex_);
    
    ExchangePrice price;
    price.exchange = exchange;
    price.bid = tick.bid;
    price.ask = tick.ask;
    price.quantity = tick.volume;
    price.timestamp = tick.timestamp;
    
    prices_[symbol][exchange] = price;
}

void CrossExchangeArbitrageStrategy::detectArbitrageOpportunities(const std::string& symbol) {
    std::lock_guard<std::mutex> lock(prices_mutex_);
    
    if (prices_[symbol].size() < 2) {
        return; // Need at least 2 exchanges
    }
    
    // Clean up stale data first
    cleanupStaleData();
    
    // Find the best bid and ask across all exchanges
    double best_bid = 0.0;
    double best_ask = std::numeric_limits<double>::max();
    std::string best_bid_exchange, best_ask_exchange;
    double best_bid_quantity = 0.0, best_ask_quantity = 0.0;
    
    for (const auto& [exchange, price] : prices_[symbol]) {
        if (isPriceStale(price)) continue;
        
        if (price.bid > best_bid) {
            best_bid = price.bid;
            best_bid_exchange = exchange;
            best_bid_quantity = price.quantity;
        }
        
        if (price.ask < best_ask) {
            best_ask = price.ask;
            best_ask_exchange = exchange;
            best_ask_quantity = price.quantity;
        }
    }
    
    // Check if we have a valid arbitrage opportunity
    if (best_bid_exchange.empty() || best_ask_exchange.empty() || 
        best_bid_exchange == best_ask_exchange) {
        return;
    }
    
    // Calculate profit
    double profit = best_bid - best_ask;
    double profit_percentage = (profit / best_ask) * 100.0;
    
    if (profit_percentage < min_profit_percentage_) {
        return;
    }
    
    // Calculate maximum quantity we can trade
    double max_quantity = std::min({best_bid_quantity, best_ask_quantity, max_order_size_});
    
    // Calculate fees
    double buy_fees = calculateFees(best_ask_exchange, max_quantity, best_ask);
    double sell_fees = calculateFees(best_bid_exchange, max_quantity, best_bid);
    double net_profit = profit * max_quantity - buy_fees - sell_fees;
    
    if (net_profit <= 0) {
        return;
    }
    
    // Create opportunity
    ArbitrageOpportunity opportunity;
    opportunity.symbol = symbol;
    opportunity.buy_exchange = best_ask_exchange;
    opportunity.sell_exchange = best_bid_exchange;
    opportunity.buy_price = best_ask;
    opportunity.sell_price = best_bid;
    opportunity.quantity = max_quantity;
    opportunity.profit = net_profit;
    opportunity.profit_percentage = profit_percentage;
    opportunity.timestamp = std::chrono::system_clock::now();
    
    // Validate and execute if profitable
    if (validateOpportunity(opportunity)) {
        {
            std::lock_guard<std::mutex> opp_lock(opportunities_mutex_);
            opportunities_.push_back(opportunity);
        }
        
        {
            std::lock_guard<std::mutex> metrics_lock(metrics_mutex_);
            metrics_.opportunities_detected++;
            metrics_.last_opportunity = opportunity.timestamp;
        }
        
        logger_->getLogger()->info("Arbitrage opportunity detected: {} {} on {}/{} profit: {:.2f}% (${:.2f})",
                      opportunity.symbol, opportunity.quantity,
                      opportunity.buy_exchange, opportunity.sell_exchange,
                      opportunity.profit_percentage, opportunity.profit);
        
        // Execute if conditions are met
        if (checkRateLimits()) {
            executeArbitrageOpportunity(opportunity);
        }
    }
}

bool CrossExchangeArbitrageStrategy::validateOpportunity(const ArbitrageOpportunity& opportunity) const {
    // Check risk limits
    if (!risk_manager_->checkPositionRisk(opportunity.symbol, opportunity.quantity)) {
        return false;
    }
    
    // Check if we have enough funds on both exchanges
    // This would require integration with exchange balance checking
    // For now, assume we have sufficient funds
    
    // Check if opportunity is still valid (not too old)
    auto age = std::chrono::system_clock::now() - opportunity.timestamp;
    if (age > max_latency_) {
        return false;
    }
    
    return true;
}

void CrossExchangeArbitrageStrategy::executeArbitrageOpportunity(const ArbitrageOpportunity& opportunity) {
    if (!checkRateLimits()) {
        return;
    }
    
    logger_->getLogger()->info("Executing arbitrage opportunity: {} {} on {}/{} profit: {:.2f}%",
                  opportunity.symbol, opportunity.quantity,
                  opportunity.buy_exchange, opportunity.sell_exchange,
                  opportunity.profit_percentage);
    
    // In a real implementation, this would:
    // 1. Place simultaneous buy order on buy_exchange and sell order on sell_exchange
    // 2. Monitor order execution
    // 3. Handle partial fills and cancellations
    // 4. Calculate actual profit after execution
    
    // For now, simulate successful execution
    double actual_profit = opportunity.profit * 0.95; // Assume 5% slippage
    
    // Update metrics
    updateMetrics(opportunity, actual_profit);
    
    // Update rate limiting
    last_execution_time_ = std::chrono::system_clock::now();
    executions_this_minute_++;
    
    // Move to executed opportunities
    {
        std::lock_guard<std::mutex> lock(opportunities_mutex_);
        executed_opportunities_.push_back(opportunity);
        
        // Remove from current opportunities
        opportunities_.erase(
            std::remove_if(opportunities_.begin(), opportunities_.end(),
                          [&](const ArbitrageOpportunity& opp) {
                              return opp.symbol == opportunity.symbol &&
                                     opp.buy_exchange == opportunity.buy_exchange &&
                                     opp.sell_exchange == opportunity.sell_exchange;
                          }),
            opportunities_.end()
        );
    }
}

bool CrossExchangeArbitrageStrategy::checkRateLimits() const {
    auto now = std::chrono::system_clock::now();
    
    // Reset minute counter if needed
    if (now - minute_start_ >= std::chrono::minutes(1)) {
        const_cast<CrossExchangeArbitrageStrategy*>(this)->executions_this_minute_ = 0;
        const_cast<CrossExchangeArbitrageStrategy*>(this)->minute_start_ = now;
    }
    
    return executions_this_minute_ < max_opportunities_per_minute_;
}

double CrossExchangeArbitrageStrategy::calculateFees(const std::string& exchange, double quantity, double price) const {
    // Default fee structure - in practice this would be exchange-specific
    double fee_rate = 0.001; // 0.1% fee
    return quantity * price * fee_rate;
}

bool CrossExchangeArbitrageStrategy::isPriceStale(const ExchangePrice& price) const {
    auto age = std::chrono::system_clock::now() - price.timestamp;
    return age > max_latency_;
}

void CrossExchangeArbitrageStrategy::cleanupStaleData() {
    auto now = std::chrono::system_clock::now();
    
    // Remove stale prices
    for (auto& [symbol, exchanges] : prices_) {
        for (auto it = exchanges.begin(); it != exchanges.end();) {
            if (now - it->second.timestamp > max_latency_) {
                it = exchanges.erase(it);
            } else {
                ++it;
            }
        }
    }
    
    // Remove stale opportunities
    std::lock_guard<std::mutex> lock(opportunities_mutex_);
    opportunities_.erase(
        std::remove_if(opportunities_.begin(), opportunities_.end(),
                      [&](const ArbitrageOpportunity& opp) {
                          return now - opp.timestamp > max_latency_;
                      }),
        opportunities_.end()
    );
}

void CrossExchangeArbitrageStrategy::updateMetrics(const ArbitrageOpportunity& opportunity, double actual_profit) {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    
    metrics_.opportunities_executed++;
    metrics_.total_profit += actual_profit;
    metrics_.last_execution = std::chrono::system_clock::now();
    
    if (actual_profit > 0) {
        metrics_.winning_trades++;
        metrics_.max_profit = std::max(metrics_.max_profit, actual_profit);
    } else {
        metrics_.losing_trades++;
        metrics_.max_loss = std::min(metrics_.max_loss, actual_profit);
    }
    
    // Update derived metrics
    if (metrics_.opportunities_executed > 0) {
        metrics_.win_rate = static_cast<double>(metrics_.winning_trades) / metrics_.opportunities_executed;
        metrics_.avg_profit_per_trade = metrics_.total_profit / metrics_.opportunities_executed;
        metrics_.net_profit = metrics_.total_profit - metrics_.total_fees;
    }
}

std::string CrossExchangeArbitrageStrategy::getExchangeFromTick(const MarketTick& tick) const {
    // Extract exchange name from tick - in practice this would be part of the tick data
    // For now, return a placeholder
    return "binance"; // This would be determined from tick.exchange_id or similar
}

bool CrossExchangeArbitrageStrategy::isExchangeMonitored(const std::string& exchange) const {
    return monitored_exchanges_.empty() || 
           std::find(monitored_exchanges_.begin(), monitored_exchanges_.end(), exchange) != monitored_exchanges_.end();
}

bool CrossExchangeArbitrageStrategy::isSymbolMonitored(const std::string& symbol) const {
    return monitored_symbols_.empty() || 
           std::find(monitored_symbols_.begin(), monitored_symbols_.end(), symbol) != monitored_symbols_.end();
}

double CrossExchangeArbitrageStrategy::getMinQuantity(const std::string& symbol, const std::string& exchange) const {
    // Exchange-specific minimum quantities
    return 0.001; // Default minimum
}

double CrossExchangeArbitrageStrategy::getMaxQuantity(const std::string& symbol, const std::string& exchange) const {
    // Exchange-specific maximum quantities
    return max_order_size_;
}

// BaseStrategy interface implementation
void CrossExchangeArbitrageStrategy::onTick(const std::string& symbol, const Trade& tick) {
    // Convert Trade to MarketTick for existing implementation
    MarketTick market_tick;
    market_tick.symbol = symbol;
    market_tick.last_price = tick.price;
    market_tick.volume = tick.quantity;
    market_tick.timestamp = tick.timestamp;
    
    // Call the existing onTick method
    onTick(market_tick);
}

void CrossExchangeArbitrageStrategy::onOrderFill(const OrderFill& fill) {
    // Convert OrderFill to OrderUpdate for existing implementation
    OrderUpdate order_update;
    order_update.order_id = fill.order_id;
    order_update.price = fill.price;
    order_update.quantity = fill.quantity;
    order_update.timestamp = fill.timestamp;
    
    onOrderUpdate(order_update);
}

void CrossExchangeArbitrageStrategy::onOrderReject(const OrderReject& reject) {
    // Log order rejection
    logger_->getLogger()->warn("Order rejected: {} - {}", reject.order_id, reject.reason);
}

void CrossExchangeArbitrageStrategy::onBalanceUpdate(const Balance& balance) {
    // Update internal balance tracking
    logger_->getLogger()->info("Balance update: {} = {}", balance.asset, balance.free);
}

void CrossExchangeArbitrageStrategy::onPositionUpdate(const Position& position) {
    // Update internal position tracking
    logger_->getLogger()->info("Position update: {} = {}", position.symbol, position.quantity);
}

} // namespace strategy
} // namespace moneybot
