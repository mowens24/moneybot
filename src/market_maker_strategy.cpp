#include "market_maker_strategy.h"
#include <algorithm>
#include <cmath>
#include <random>

namespace moneybot {

MarketMakerConfig::MarketMakerConfig(const nlohmann::json& j) {
    spread_bps = j["spread_bps"].get<double>();
    order_size = j["order_size"].get<double>();
    max_position = j["max_position"].get<double>();
    rebalance_threshold = j["rebalance_threshold"].get<double>();
    refresh_interval_ms = j["refresh_interval_ms"].get<int>();
    min_spread_bps = j["min_spread_bps"].get<double>();
    max_slippage_bps = j["max_slippage_bps"].get<double>();
    aggressive_rebalancing = j["aggressive_rebalancing"].get<bool>();
}

MarketMakerStrategy::MarketMakerStrategy(std::shared_ptr<Logger> logger,
                                       std::shared_ptr<OrderManager> order_manager,
                                       std::shared_ptr<RiskManager> risk_manager,
                                       const nlohmann::json& config)
    : logger_(logger), order_manager_(order_manager), risk_manager_(risk_manager),
      current_position_(0.0), current_bid_price_(0.0), current_ask_price_(0.0), mid_price_(0.0),
      total_pnl_(0.0), total_trades_(0), avg_spread_(0.0), spread_samples_(0) {
    loadConfig(config);
    logger_->getLogger()->info("MarketMakerStrategy initialized for {}", symbol_);
}

MarketMakerStrategy::~MarketMakerStrategy() {
    shutdown();
}

void MarketMakerStrategy::initialize() {
    logger_->getLogger()->info("Initializing MarketMakerStrategy");
    
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
    
    logger_->getLogger()->info("MarketMakerStrategy initialized successfully");
}

void MarketMakerStrategy::shutdown() {
    logger_->getLogger()->info("Shutting down MarketMakerStrategy");
    cancelAllOrders();
}

void MarketMakerStrategy::onOrderBookUpdate(const OrderBook& order_book) {
    std::lock_guard<std::mutex> lock(state_mutex_);
    
    // Update mid price
    double best_bid = order_book.getBestBid();
    double best_ask = order_book.getBestAsk();
    
    if (best_bid > 0 && best_ask > 0) {
        mid_price_ = (best_bid + best_ask) / 2.0;
        
        // Calculate current spread
        double spread_bps = (best_ask - best_bid) / mid_price_ * 10000.0;
        if (spread_samples_ == 0) {
            avg_spread_ = spread_bps;
        } else {
            avg_spread_ = (avg_spread_ * spread_samples_ + spread_bps) / (spread_samples_ + 1);
        }
        spread_samples_++;
        
        // Check if we should place orders
        if (shouldPlaceOrders()) {
            calculateQuotes();
            placeQuotes();
        }
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
    
    // Calculate bid and ask prices based on spread
    double spread = config_.spread_bps / 10000.0;
    current_bid_price_ = mid_price_ * (1.0 - spread / 2.0);
    current_ask_price_ = mid_price_ * (1.0 + spread / 2.0);
    
    // Adjust for position skew
    double position_skew = current_position_ / config_.max_position;
    if (std::abs(position_skew) > 0.1) {
        if (position_skew > 0) {
            // Long position - bias towards selling
            current_bid_price_ *= (1.0 - position_skew * 0.001);
            current_ask_price_ *= (1.0 - position_skew * 0.002);
        } else {
            // Short position - bias towards buying
            current_bid_price_ *= (1.0 + position_skew * 0.002);
            current_ask_price_ *= (1.0 + position_skew * 0.001);
        }
    }
    
    // Round to appropriate precision
    current_bid_price_ = std::floor(current_bid_price_ * 100000) / 100000;
    current_ask_price_ = std::ceil(current_ask_price_ * 100000) / 100000;
}

void MarketMakerStrategy::placeQuotes() {
    if (risk_manager_->isEmergencyStopped()) {
        logger_->getLogger()->warn("Not placing quotes: Emergency stop active");
        return;
    }
    
    // Cancel stale orders first
    cancelStaleOrders();
    
    // Calculate order sizes
    double base_size = calculateOrderSize();
    
    // Place bid order
    if (current_bid_price_ > 0 && std::abs(current_position_) < config_.max_position) {
        placeBidOrder(current_bid_price_, base_size);
    }
    
    // Place ask order
    if (current_ask_price_ > 0 && std::abs(current_position_) < config_.max_position) {
        placeAskOrder(current_ask_price_, base_size);
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
    
    if (!risk_manager_->checkOrderRisk(order)) {
        logger_->getLogger()->warn("Bid order rejected by risk manager");
        return;
    }
    
    std::string order_id = order_manager_->placeOrder(order);
    if (!order_id.empty()) {
        std::lock_guard<std::mutex> lock(orders_mutex_);
        active_orders_[order_id] = {order_id, OrderSide::BUY, price, quantity, 
                                   std::chrono::system_clock::now()};
        logger_->getLogger()->debug("Bid order placed: {} @ {}", quantity, price);
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
    
    if (!risk_manager_->checkOrderRisk(order)) {
        logger_->getLogger()->warn("Ask order rejected by risk manager");
        return;
    }
    
    std::string order_id = order_manager_->placeOrder(order);
    if (!order_id.empty()) {
        std::lock_guard<std::mutex> lock(orders_mutex_);
        active_orders_[order_id] = {order_id, OrderSide::SELL, price, quantity, 
                                   std::chrono::system_clock::now()};
        logger_->getLogger()->debug("Ask order placed: {} @ {}", quantity, price);
    }
}

void MarketMakerStrategy::cancelOrder(const std::string& order_id) {
    if (order_manager_->cancelOrder(order_id)) {
        std::lock_guard<std::mutex> lock(orders_mutex_);
        active_orders_.erase(order_id);
        logger_->getLogger()->debug("Order cancelled: {}", order_id);
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

} // namespace moneybot 