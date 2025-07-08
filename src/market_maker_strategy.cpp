#include "market_maker_strategy.h"
#include <algorithm>
#include <cmath>
#include <random>

namespace moneybot {

MarketMakerConfig::MarketMakerConfig(const nlohmann::json& j) {
    base_spread_bps = j.value("base_spread_bps", 5.0);
    max_spread_bps = j.value("max_spread_bps", 50.0);
    min_spread_bps = j.value("min_spread_bps", 1.0);
    order_size = j.value("order_size", 0.001);
    max_position = j.value("max_position", 0.01);
    inventory_skew_factor = j.value("inventory_skew_factor", 2.0);
    volatility_window = j.value("volatility_window", 100);
    volatility_multiplier = j.value("volatility_multiplier", 1.5);
    refresh_interval_ms = j.value("refresh_interval_ms", 1000);
    min_profit_bps = j.value("min_profit_bps", 0.5);
    rebalance_threshold = j.value("rebalance_threshold", 0.5);
    max_slippage_bps = j.value("max_slippage_bps", 10.0);
    aggressive_rebalancing = j.value("aggressive_rebalancing", false);
}

MarketMakerStrategy::MarketMakerStrategy(std::shared_ptr<Logger> logger,
                                       std::shared_ptr<OrderManager> order_manager,
                                       std::shared_ptr<RiskManager> risk_manager,
                                       const nlohmann::json& config)
    : logger_(logger), order_manager_(order_manager), risk_manager_(risk_manager),
      current_position_(0.0), current_bid_price_(0.0), current_ask_price_(0.0), mid_price_(0.0),
      current_volatility_(0.0), current_spread_bps_(0.0),
      total_pnl_(0.0), realized_pnl_(0.0), unrealized_pnl_(0.0), 
      total_trades_(0), avg_spread_(0.0), spread_samples_(0), strategy_active_(true) {
    
    loadConfig(config);
    // Note: deque doesn't have reserve, but we'll limit size in push operations
    
    if (logger_) {
        logger_->getLogger()->info("MarketMakerStrategy initialized for {} with enhanced features", symbol_);
        logger_->getLogger()->info("  Base Spread: {}bps", config_.base_spread_bps);
        logger_->getLogger()->info("  Max Position: {}", config_.max_position);
        logger_->getLogger()->info("  Order Size: {}", config_.order_size);
        logger_->getLogger()->info("  Refresh Interval: {}ms", config_.refresh_interval_ms);
    }
}

MarketMakerStrategy::~MarketMakerStrategy() {
    shutdown();
}

void MarketMakerStrategy::initialize() {
    if (logger_) logger_->getLogger()->info("Initializing MarketMakerStrategy");
    
    // Cancel any existing orders
    cancelAllOrders();
    
    // Reset state
    current_position_ = 0.0;
    total_pnl_ = 0.0;
    total_trades_ = 0;
    avg_spread_ = 0.0;
    spread_samples_ = 0;
    
    last_quote_time_ = std::chrono::system_clock::now();
    last_rebalance_time_ = std::chrono::system_clock::now();
    
    if (logger_) logger_->getLogger()->info("MarketMakerStrategy initialized successfully");
}

void MarketMakerStrategy::shutdown() {
    if (logger_) logger_->getLogger()->info("Shutting down MarketMakerStrategy");
    cancelAllOrders();
}

void MarketMakerStrategy::onOrderBookUpdate(const OrderBook& order_book) {
    std::lock_guard<std::mutex> lock(state_mutex_);
    
    if (!strategy_active_) return;
    
    // Update mid price
    double best_bid = order_book.getBestBid();
    double best_ask = order_book.getBestAsk();
    
    if (best_bid > 0 && best_ask > 0) {
        mid_price_ = (best_bid + best_ask) / 2.0;
        
        // Track price history for volatility calculation
        recent_prices_.push_back(mid_price_);
        if (recent_prices_.size() > config_.volatility_window) {
            recent_prices_.pop_front();
        }
        
        // Calculate current spread
        double spread_bps = (best_ask - best_bid) / mid_price_ * 10000.0;
        recent_spreads_.push_back(spread_bps);
        if (recent_spreads_.size() > config_.volatility_window) {
            recent_spreads_.pop_front();
        }
        
        // Update running average spread
        if (spread_samples_ == 0) {
            avg_spread_ = spread_bps;
        } else {
            avg_spread_ = (avg_spread_ * spread_samples_ + spread_bps) / (spread_samples_ + 1);
        }
        spread_samples_++;
        
        // Calculate current volatility
        current_volatility_ = calculateVolatility();
        
        // Update optimal spread
        current_spread_bps_ = calculateOptimalSpread();
        
        // Check if we should refresh orders
        if (shouldRefreshOrders()) {
            cancelStaleOrders();
            calculateQuotes();
            placeQuotes();
        }
        
        // Update metrics
        updateMetrics();
    }
}

void MarketMakerStrategy::onTrade(const Trade& trade) {
    logger_->getLogger()->debug("Trade: {} {} @ {}", 
                               trade.symbol, trade.quantity, trade.price);
    // No position update here; only update position on order fills (onOrderFill)
}

void MarketMakerStrategy::onOrderAck(const OrderAck& ack) {
    logger_->getLogger()->debug("Order acknowledged: {}", ack.order_id);
}

void MarketMakerStrategy::onOrderReject(const OrderReject& reject) {
    logger_->getLogger()->warn("Order rejected: {} - {}", reject.order_id, reject.reason);
    
    // Remove from active orders
    std::lock_guard<std::mutex> lock(orders_mutex_);
    active_orders_.erase(reject.order_id);
}

void MarketMakerStrategy::onOrderFill(const OrderFill& fill) {
    logger_->getLogger()->info("Order filled: {} {} @ {}", 
                               fill.order_id, fill.quantity, fill.price);
    
    // Update position
    auto it = active_orders_.find(fill.order_id);
    if (it != active_orders_.end()) {
        updatePosition(fill.quantity, it->second.side);
        
        // Remove filled order
        std::lock_guard<std::mutex> lock(orders_mutex_);
        active_orders_.erase(fill.order_id);
    }
}

void MarketMakerStrategy::updateConfig(const nlohmann::json& config) {
    loadConfig(config);
    logger_->getLogger()->info("MarketMakerStrategy config updated");
}

void MarketMakerStrategy::calculateQuotes() {
    if (mid_price_ <= 0) return;
    
    // Calculate spread in price terms using optimal spread
    double spread_price = mid_price_ * (current_spread_bps_ / 10000.0);
    double half_spread = spread_price / 2.0;
    
    // Apply inventory skew
    double skew = calculateInventorySkew();
    double skew_adjustment = mid_price_ * (skew / 10000.0);
    
    // Calculate target prices
    current_bid_price_ = mid_price_ - half_spread + skew_adjustment;
    current_ask_price_ = mid_price_ + half_spread + skew_adjustment;
    
    // Round to appropriate precision (assume 5 decimal places for crypto)
    current_bid_price_ = std::floor(current_bid_price_ * 100000) / 100000;
    current_ask_price_ = std::ceil(current_ask_price_ * 100000) / 100000;
    
    logger_->getLogger()->debug("Calculated quotes: Bid: {:.5f}, Ask: {:.5f}, Spread: {:.2f}bps, Skew: {:.2f}",
                               current_bid_price_, current_ask_price_, current_spread_bps_, skew);
}

void MarketMakerStrategy::placeQuotes() {
    if (risk_manager_ && risk_manager_->isEmergencyStopped()) {
        if (logger_) logger_->getLogger()->warn("Not placing quotes: Emergency stop active");
        return;
    }
    
    // Cancel stale orders first
    cancelStaleOrders();
    
    // Get sophisticated order sizes
    auto [bid_size, ask_size] = calculateOrderSizes();
    
    // Place bid order
    if (current_bid_price_ > 0 && isWithinRiskLimits(current_bid_price_, bid_size, true)) {
        placeBidOrder(current_bid_price_, bid_size);
    }
    
    // Place ask order
    if (current_ask_price_ > 0 && isWithinRiskLimits(current_ask_price_, ask_size, false)) {
        placeAskOrder(current_ask_price_, ask_size);
    }
    
    last_quote_time_ = std::chrono::system_clock::now();
}

void MarketMakerStrategy::cancelStaleOrders() {
    auto now = std::chrono::system_clock::now();
    std::vector<std::string> stale_orders;
    
    {
        std::lock_guard<std::mutex> lock(orders_mutex_);
        for (const auto& [order_id, order] : active_orders_) {
            if (isOrderStale(order_id)) {
                stale_orders.push_back(order_id);
            }
        }
    }
    
    for (const auto& order_id : stale_orders) {
        cancelOrder(order_id);
    }
}

void MarketMakerStrategy::rebalancePosition() {
    if (std::abs(current_position_) < config_.rebalance_threshold) {
        return; // No rebalancing needed
    }
    
    auto now = std::chrono::system_clock::now();
    if (now - last_rebalance_time_ < std::chrono::milliseconds(config_.refresh_interval_ms)) {
        return; // Too soon to rebalance
    }
    
    logger_->getLogger()->info("Rebalancing position: {}", current_position_);
    
    // Cancel all orders
    cancelAllOrders();
    
    // Place aggressive orders to reduce position
    double rebalance_size = std::min(std::abs(current_position_), config_.order_size);
    
    if (current_position_ > 0) {
        // Long position - place aggressive sell
        double aggressive_price = mid_price_ * (1.0 - config_.max_slippage_bps / 10000.0);
        placeAskOrder(aggressive_price, rebalance_size);
    } else {
        // Short position - place aggressive buy
        double aggressive_price = mid_price_ * (1.0 + config_.max_slippage_bps / 10000.0);
        placeBidOrder(aggressive_price, rebalance_size);
    }
    
    last_rebalance_time_ = now;
}

void MarketMakerStrategy::placeBidOrder(double price, double quantity) {
    Order order;
    order.symbol = symbol_;
    order.side = OrderSide::BUY;
    order.type = OrderType::LIMIT;
    order.quantity = quantity;
    order.price = price;
    order.client_order_id = generateClientOrderId();
    
    if (risk_manager_ && !risk_manager_->checkOrderRisk(order)) {
        if (logger_) logger_->getLogger()->warn("Bid order rejected by risk manager");
        return;
    }
    
    if (order_manager_) {
        std::string order_id = order_manager_->placeOrder(order);
        if (!order_id.empty()) {
            std::lock_guard<std::mutex> lock(orders_mutex_);
            active_orders_[order_id] = {order_id, OrderSide::BUY, price, quantity, 
                                       std::chrono::system_clock::now()};
            if (logger_) logger_->getLogger()->debug("Bid order placed: {} @ {}", quantity, price);
        }
    }
}

void MarketMakerStrategy::placeAskOrder(double price, double quantity) {
    Order order;
    order.symbol = symbol_;
    order.side = OrderSide::SELL;
    order.type = OrderType::LIMIT;
    order.quantity = quantity;
    order.price = price;
    order.client_order_id = generateClientOrderId();
    
    if (risk_manager_ && !risk_manager_->checkOrderRisk(order)) {
        if (logger_) logger_->getLogger()->warn("Ask order rejected by risk manager");
        return;
    }
    
    if (order_manager_) {
        std::string order_id = order_manager_->placeOrder(order);
        if (!order_id.empty()) {
            std::lock_guard<std::mutex> lock(orders_mutex_);
            active_orders_[order_id] = {order_id, OrderSide::SELL, price, quantity, 
                                       std::chrono::system_clock::now()};
            if (logger_) logger_->getLogger()->debug("Ask order placed: {} @ {}", quantity, price);
        }
    }
}

void MarketMakerStrategy::cancelOrder(const std::string& order_id) {
    if (order_manager_ && order_manager_->cancelOrder(order_id)) {
        std::lock_guard<std::mutex> lock(orders_mutex_);
        active_orders_.erase(order_id);
        if (logger_) logger_->getLogger()->debug("Order cancelled: {}", order_id);
    }
}

void MarketMakerStrategy::cancelAllOrders() {
    std::vector<std::string> order_ids;
    {
        std::lock_guard<std::mutex> lock(orders_mutex_);
        for (const auto& [order_id, _] : active_orders_) {
            order_ids.push_back(order_id);
        }
    }
    
    for (const auto& order_id : order_ids) {
        cancelOrder(order_id);
    }
}

double MarketMakerStrategy::getCurrentPosition() {
    return current_position_;
}

double MarketMakerStrategy::getTargetPosition() {
    return 0.0; // Market makers typically target zero position
}

void MarketMakerStrategy::updatePosition(double fill_quantity, OrderSide side) {
    if (side == OrderSide::BUY) {
        current_position_ += fill_quantity;
    } else {
        current_position_ -= fill_quantity;
    }
    
    // Check if rebalancing is needed
    if (std::abs(current_position_) > config_.rebalance_threshold) {
        rebalancePosition();
    }
}

double MarketMakerStrategy::calculateBidPrice() {
    return current_bid_price_;
}

double MarketMakerStrategy::calculateAskPrice() {
    return current_ask_price_;
}

double MarketMakerStrategy::calculateOrderSize() {
    // Base size with some randomization to avoid detection
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_real_distribution<> dis(0.8, 1.2);
    
    return config_.order_size * dis(gen);
}

bool MarketMakerStrategy::shouldPlaceOrders() {
    auto now = std::chrono::system_clock::now();
    return (now - last_quote_time_) >= std::chrono::milliseconds(config_.refresh_interval_ms);
}

bool MarketMakerStrategy::isOrderStale(const std::string& order_id) {
    auto it = active_orders_.find(order_id);
    if (it == active_orders_.end()) return true;
    
    auto now = std::chrono::system_clock::now();
    return (now - it->second.timestamp) > std::chrono::milliseconds(config_.refresh_interval_ms * 2);
}

void MarketMakerStrategy::loadConfig(const nlohmann::json& config) {
    symbol_ = config["strategy"]["symbol"].get<std::string>();
    config_ = MarketMakerConfig(config["strategy"]["config"]);
}

std::string MarketMakerStrategy::generateClientOrderId() {
    static int counter = 0;
    return "MM_" + std::to_string(++counter) + "_" + std::to_string(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count());
}

// Enhanced market making methods
double MarketMakerStrategy::calculateOptimalSpread() const {
    if (mid_price_ <= 0) return config_.base_spread_bps;
    
    // Base spread
    double spread_bps = config_.base_spread_bps;
    
    // Adjust for volatility
    if (current_volatility_ > 0) {
        spread_bps += current_volatility_ * config_.volatility_multiplier * 10000; // Convert to bps
    }
    
    // Adjust for inventory (wider spread if we have large position)
    double inventory_factor = std::abs(current_position_) / config_.max_position;
    spread_bps += inventory_factor * config_.base_spread_bps * 0.5;
    
    // Ensure within bounds
    spread_bps = std::max(config_.min_spread_bps, std::min(config_.max_spread_bps, spread_bps));
    
    return spread_bps;
}

double MarketMakerStrategy::calculateInventorySkew() const {
    if (config_.max_position <= 0) return 0.0;
    
    // Skew orders based on current position
    // If long, skew towards selling (widen bid, tighten ask)
    // If short, skew towards buying (tighten bid, widen ask)
    double position_ratio = current_position_ / config_.max_position;
    return position_ratio * config_.inventory_skew_factor;
}

double MarketMakerStrategy::calculateVolatility() const {
    if (recent_prices_.size() < 10) return 0.0;
    
    // Calculate returns
    std::vector<double> returns;
    for (size_t i = 1; i < recent_prices_.size(); ++i) {
        double ret = (recent_prices_[i] - recent_prices_[i-1]) / recent_prices_[i-1];
        returns.push_back(ret);
    }
    
    // Calculate standard deviation
    double mean = 0.0;
    for (double ret : returns) {
        mean += ret;
    }
    mean /= returns.size();
    
    double variance = 0.0;
    for (double ret : returns) {
        variance += (ret - mean) * (ret - mean);
    }
    variance /= returns.size();
    
    return std::sqrt(variance);
}

std::pair<double, double> MarketMakerStrategy::calculateOrderSizes() const {
    double base_size = config_.order_size;
    
    // Reduce size if approaching position limits
    double position_utilization = std::abs(current_position_) / config_.max_position;
    double size_factor = std::max(0.1, 1.0 - position_utilization);
    
    double bid_size = base_size * size_factor;
    double ask_size = base_size * size_factor;
    
    // Further adjust based on inventory
    if (current_position_ > 0) {
        // Long position - prefer to sell
        ask_size *= 1.2;
        bid_size *= 0.8;
    } else if (current_position_ < 0) {
        // Short position - prefer to buy
        bid_size *= 1.2;
        ask_size *= 0.8;
    }
    
    return {bid_size, ask_size};
}

bool MarketMakerStrategy::shouldRefreshOrders() const {
    auto now = std::chrono::system_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_quote_time_);
    
    // Time-based refresh
    if (elapsed.count() > config_.refresh_interval_ms) {
        return true;
    }
    
    // Price-based refresh (if market moved significantly)
    if (mid_price_ > 0) {
        double expected_bid = mid_price_ - (mid_price_ * current_spread_bps_ / 20000.0);
        double expected_ask = mid_price_ + (mid_price_ * current_spread_bps_ / 20000.0);
        
        // Refresh if our orders are more than 2 ticks away from optimal
        double tick_size = mid_price_ * 0.0001; // Assume 1bp tick size
        if (std::abs(current_bid_price_ - expected_bid) > 2 * tick_size ||
            std::abs(current_ask_price_ - expected_ask) > 2 * tick_size) {
            return true;
        }
    }
    
    return false;
}

bool MarketMakerStrategy::isWithinRiskLimits(double price, double size, bool is_buy) const {
    // Check position limits
    double new_position = current_position_;
    if (is_buy) {
        new_position += size;
    } else {
        new_position -= size;
    }
    
    if (std::abs(new_position) > config_.max_position) {
        logger_->getLogger()->warn("Order rejected: would exceed position limit ({} > {})", 
                                 std::abs(new_position), config_.max_position);
        return false;
    }
    
    // Check if strategy is still active
    if (!strategy_active_) {
        return false;
    }
    
    return true;
}

void MarketMakerStrategy::updateMetrics() {
    // Update unrealized PnL
    if (current_position_ != 0.0 && mid_price_ > 0.0) {
        unrealized_pnl_ = current_position_ * mid_price_;
    }
    
    total_pnl_ = realized_pnl_ + unrealized_pnl_;
    
    // Log strategy state periodically
    static int log_counter = 0;
    if (++log_counter % 50 == 0) { // Log every 50 updates
        logStrategyState();
    }
}

void MarketMakerStrategy::logStrategyState() const {
    logger_->getLogger()->info("=== Market Maker State ===");
    logger_->getLogger()->info("Position: {} | PnL: {} (R: {}, U: {})", 
                             current_position_, total_pnl_, realized_pnl_, unrealized_pnl_);
    logger_->getLogger()->info("Spread: {:.2f}bps | Volatility: {:.6f}", current_spread_bps_, current_volatility_);
    logger_->getLogger()->info("Active Orders: {} | Mid Price: {:.2f}", active_orders_.size(), mid_price_);
    logger_->getLogger()->info("Market: {:.2f} / {:.2f} (spread: {:.2f}bps)", 
                             current_bid_price_, current_ask_price_, avg_spread_);
}

} // namespace moneybot 