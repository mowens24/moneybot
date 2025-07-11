#include "testing/mock_exchange.h"
#include <random>
#include <algorithm>

namespace moneybot {
namespace testing {

MockExchange::MockExchange(const std::string& name, const nlohmann::json& config)
    : exchange_name_(name), config_(config), is_connected_(false), next_order_id_(1) {
    
    // Initialize logger
    logger_ = std::make_shared<Logger>();
    
    // Parse configuration
    if (config_.contains("latency_ms")) {
        latency_ = std::chrono::milliseconds(config_["latency_ms"].get<int>());
    }
    
    if (config_.contains("fee_rate")) {
        fee_rate_ = config_["fee_rate"].get<double>();
    }
    
    if (config_.contains("slippage_rate")) {
        slippage_rate_ = config_["slippage_rate"].get<double>();
    }
    
    if (config_.contains("error_rate")) {
        error_rate_ = config_["error_rate"].get<double>();
    }
    
    if (config_.contains("initial_prices")) {
        for (const auto& [symbol, price_data] : config_["initial_prices"].items()) {
            MarketData data;
            data.symbol = symbol;
            data.bid = price_data["bid"].get<double>();
            data.ask = price_data["ask"].get<double>();
            data.last_price = (data.bid + data.ask) / 2.0;
            data.volume = price_data.value("volume", 1000.0);
            data.timestamp = std::chrono::system_clock::now();
            market_data_[symbol] = data;
        }
    }
    
    if (config_.contains("initial_balances")) {
        for (const auto& [asset, balance] : config_["initial_balances"].items()) {
            balances_[asset] = balance.get<double>();
        }
    }
    
    // Start background threads
    market_thread_ = std::thread(&MockExchange::marketDataLoop, this);
    order_thread_ = std::thread(&MockExchange::orderProcessingLoop, this);
}

MockExchange::~MockExchange() {
    stop();
}

void MockExchange::start() {
    std::lock_guard<std::mutex> lock(state_mutex_);
    is_connected_ = true;
    logger_->getLogger()->info("MockExchange '{}' started", exchange_name_);
}

void MockExchange::stop() {
    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        is_connected_ = false;
    }
    
    if (market_thread_.joinable()) {
        market_thread_.join();
    }
    
    if (order_thread_.joinable()) {
        order_thread_.join();
    }
    
    logger_->getLogger()->info("MockExchange '{}' stopped", exchange_name_);
}

bool MockExchange::isConnected() const {
    std::lock_guard<std::mutex> lock(state_mutex_);
    return is_connected_;
}

std::string MockExchange::placeOrder(const Order& order) {
    if (!isConnected()) {
        throw std::runtime_error("Exchange not connected");
    }
    
    // Simulate random errors
    if (shouldSimulateError()) {
        throw std::runtime_error("Simulated order placement error");
    }
    
    // Create order with unique ID
    MockOrder mock_order;
    mock_order.order_id = std::to_string(next_order_id_++);
    mock_order.symbol = order.symbol;
    mock_order.side = order.side;
    mock_order.type = order.type;
    mock_order.quantity = order.quantity;
    mock_order.price = order.price;
    mock_order.status = OrderStatus::PENDING;
    mock_order.filled_quantity = 0.0;
    mock_order.avg_fill_price = 0.0;
    mock_order.timestamp = std::chrono::system_clock::now();
    
    // Validate order
    if (!validateOrder(mock_order)) {
        throw std::runtime_error("Order validation failed");
    }
    
    // Add to pending orders
    {
        std::lock_guard<std::mutex> lock(orders_mutex_);
        pending_orders_[mock_order.order_id] = mock_order;
    }
    
    logger_->getLogger()->info("Order placed: {} {} {} {} @ {}", 
                  mock_order.order_id, mock_order.symbol, 
                  mock_order.side == OrderSide::BUY ? "BUY" : "SELL",
                  mock_order.quantity, mock_order.price);
    
    return mock_order.order_id;
}

bool MockExchange::cancelOrder(const std::string& order_id) {
    if (!isConnected()) {
        return false;
    }
    
    std::lock_guard<std::mutex> lock(orders_mutex_);
    auto it = pending_orders_.find(order_id);
    if (it != pending_orders_.end()) {
        it->second.status = OrderStatus::CANCELLED;
        executed_orders_[order_id] = it->second;
        pending_orders_.erase(it);
        
        logger_->getLogger()->info("Order cancelled: {}", order_id);
        return true;
    }
    
    return false;
}

Order MockExchange::getOrder(const std::string& order_id) const {
    std::lock_guard<std::mutex> lock(orders_mutex_);
    
    // Check pending orders
    auto it = pending_orders_.find(order_id);
    if (it != pending_orders_.end()) {
        return convertToOrder(it->second);
    }
    
    // Check executed orders
    auto it2 = executed_orders_.find(order_id);
    if (it2 != executed_orders_.end()) {
        return convertToOrder(it2->second);
    }
    
    throw std::runtime_error("Order not found: " + order_id);
}

std::vector<Order> MockExchange::getOpenOrders() const {
    std::lock_guard<std::mutex> lock(orders_mutex_);
    std::vector<Order> orders;
    
    for (const auto& [id, mock_order] : pending_orders_) {
        if (mock_order.status == OrderStatus::ACKNOWLEDGED) {
            orders.push_back(convertToOrder(mock_order));
        }
    }
    
    return orders;
}

std::vector<Order> MockExchange::getOrderHistory(const std::string& symbol, int limit) const {
    std::lock_guard<std::mutex> lock(orders_mutex_);
    std::vector<Order> orders;
    
    for (const auto& [id, mock_order] : executed_orders_) {
        if (symbol.empty() || mock_order.symbol == symbol) {
            orders.push_back(convertToOrder(mock_order));
        }
    }
    
    // Sort by timestamp (newest first)
    std::sort(orders.begin(), orders.end(), [](const Order& a, const Order& b) {
        return a.timestamp > b.timestamp;
    });
    
    if (limit > 0 && orders.size() > static_cast<size_t>(limit)) {
        orders.resize(limit);
    }
    
    return orders;
}

double MockExchange::getBalance(const std::string& asset) const {
    std::lock_guard<std::mutex> lock(balances_mutex_);
    auto it = balances_.find(asset);
    return it != balances_.end() ? it->second : 0.0;
}

std::unordered_map<std::string, double> MockExchange::getAllBalances() const {
    std::lock_guard<std::mutex> lock(balances_mutex_);
    return balances_;
}

MarketData MockExchange::getMarketData(const std::string& symbol) const {
    std::lock_guard<std::mutex> lock(market_data_mutex_);
    auto it = market_data_.find(symbol);
    if (it != market_data_.end()) {
        return it->second;
    }
    
    throw std::runtime_error("Market data not found for symbol: " + symbol);
}

std::vector<MarketData> MockExchange::getAllMarketData() const {
    std::lock_guard<std::mutex> lock(market_data_mutex_);
    std::vector<MarketData> data;
    
    for (const auto& [symbol, market_data] : market_data_) {
        data.push_back(market_data);
    }
    
    return data;
}

void MockExchange::subscribeToMarketData(const std::string& symbol, MarketDataCallback callback) {
    std::lock_guard<std::mutex> lock(callbacks_mutex_);
    market_data_callbacks_[symbol] = callback;
    logger_->getLogger()->info("Subscribed to market data for: {}", symbol);
}

void MockExchange::subscribeToOrderUpdates(OrderUpdateCallback callback) {
    std::lock_guard<std::mutex> lock(callbacks_mutex_);
    order_update_callback_ = callback;
    logger_->getLogger()->info("Subscribed to order updates");
}

void MockExchange::subscribeToTradeUpdates(TradeUpdateCallback callback) {
    std::lock_guard<std::mutex> lock(callbacks_mutex_);
    trade_update_callback_ = callback;
    logger_->getLogger()->info("Subscribed to trade updates");
}

void MockExchange::setBalance(const std::string& asset, double balance) {
    std::lock_guard<std::mutex> lock(balances_mutex_);
    balances_[asset] = balance;
    logger_->getLogger()->info("Balance set: {} = {}", asset, balance);
}

void MockExchange::setMarketData(const std::string& symbol, const MarketData& data) {
    {
        std::lock_guard<std::mutex> lock(market_data_mutex_);
        market_data_[symbol] = data;
    }
    
    // Notify subscribers
    notifyMarketDataUpdate(symbol, data);
}

void MockExchange::simulateMarketMovement(const std::string& symbol, double price_change_percent) {
    std::lock_guard<std::mutex> lock(market_data_mutex_);
    auto it = market_data_.find(symbol);
    if (it != market_data_.end()) {
        double change_factor = 1.0 + (price_change_percent / 100.0);
        it->second.bid *= change_factor;
        it->second.ask *= change_factor;
        it->second.last_price *= change_factor;
        it->second.timestamp = std::chrono::system_clock::now();
        
        // Notify subscribers
        notifyMarketDataUpdate(symbol, it->second);
    }
}

void MockExchange::simulateOrderExecution(const std::string& order_id, double fill_price, double fill_quantity) {
    std::lock_guard<std::mutex> lock(orders_mutex_);
    auto it = pending_orders_.find(order_id);
    if (it != pending_orders_.end()) {
        auto& order = it->second;
        
        // Update order status
        double remaining = order.quantity - order.filled_quantity;
        double actual_fill = std::min(fill_quantity, remaining);
        
        order.filled_quantity += actual_fill;
        order.avg_fill_price = ((order.avg_fill_price * (order.filled_quantity - actual_fill)) + 
                               (fill_price * actual_fill)) / order.filled_quantity;
        
        if (order.filled_quantity >= order.quantity) {
            order.status = OrderStatus::FILLED;
            executed_orders_[order_id] = order;
            pending_orders_.erase(it);
        } else {
            order.status = OrderStatus::PARTIALLY_FILLED;
        }
        
        // Update balances
        updateBalanceAfterTrade(order, actual_fill, fill_price);
        
        // Notify subscribers
        notifyOrderUpdate(order);
        notifyTradeUpdate(order, actual_fill, fill_price);
        
        logger_->getLogger()->info("Order executed: {} filled {} @ {}", order_id, actual_fill, fill_price);
    }
}

void MockExchange::setLatency(std::chrono::milliseconds latency) {
    latency_ = latency;
}

void MockExchange::setFeeRate(double fee_rate) {
    fee_rate_ = fee_rate;
}

void MockExchange::setSlippageRate(double slippage_rate) {
    slippage_rate_ = slippage_rate;
}

void MockExchange::setErrorRate(double error_rate) {
    error_rate_ = error_rate;
}

nlohmann::json MockExchange::getStatistics() const {
    nlohmann::json stats;
    
    {
        std::lock_guard<std::mutex> lock(orders_mutex_);
        stats["total_orders"] = pending_orders_.size() + executed_orders_.size();
        stats["pending_orders"] = pending_orders_.size();
        stats["executed_orders"] = executed_orders_.size();
    }
    
    {
        std::lock_guard<std::mutex> lock(balances_mutex_);
        stats["total_assets"] = balances_.size();
    }
    
    {
        std::lock_guard<std::mutex> lock(market_data_mutex_);
        stats["market_data_symbols"] = market_data_.size();
    }
    
    stats["exchange_name"] = exchange_name_;
    stats["is_connected"] = isConnected();
    stats["latency_ms"] = latency_.count();
    stats["fee_rate"] = fee_rate_;
    stats["slippage_rate"] = slippage_rate_;
    stats["error_rate"] = error_rate_;
    
    return stats;
}

void MockExchange::reset() {
    std::lock_guard<std::mutex> state_lock(state_mutex_);
    std::lock_guard<std::mutex> orders_lock(orders_mutex_);
    std::lock_guard<std::mutex> balances_lock(balances_mutex_);
    std::lock_guard<std::mutex> market_lock(market_data_mutex_);
    
    pending_orders_.clear();
    executed_orders_.clear();
    balances_.clear();
    market_data_.clear();
    next_order_id_ = 1;
    
    logger_->getLogger()->info("MockExchange '{}' reset", exchange_name_);
}

void MockExchange::marketDataLoop() {
    while (true) {
        {
            std::lock_guard<std::mutex> lock(state_mutex_);
            if (!is_connected_) break;
        }
        
        // Simulate market data updates
        updateMarketData();
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void MockExchange::orderProcessingLoop() {
    while (true) {
        {
            std::lock_guard<std::mutex> lock(state_mutex_);
            if (!is_connected_) break;
        }
        
        // Process pending orders
        processOrders();
        
        std::this_thread::sleep_for(latency_);
    }
}

void MockExchange::updateMarketData() {
    std::lock_guard<std::mutex> lock(market_data_mutex_);
    
    for (auto& [symbol, data] : market_data_) {
        // Simulate small random price movements
        static std::random_device rd;
        static std::mt19937 gen(rd());
        static std::uniform_real_distribution<> dis(-0.001, 0.001);
        
        double change = dis(gen);
        data.bid *= (1.0 + change);
        data.ask *= (1.0 + change);
        data.last_price = (data.bid + data.ask) / 2.0;
        data.timestamp = std::chrono::system_clock::now();
        
        // Notify subscribers
        notifyMarketDataUpdate(symbol, data);
    }
}

void MockExchange::processOrders() {
    std::lock_guard<std::mutex> lock(orders_mutex_);
    
    for (auto& [order_id, order] : pending_orders_) {
        if (order.status == OrderStatus::PENDING) {
            order.status = OrderStatus::ACKNOWLEDGED;
            notifyOrderUpdate(order);
        }
        
        // Simulate order execution
        if (order.status == OrderStatus::ACKNOWLEDGED && shouldExecuteOrder(order)) {
            double fill_price = calculateFillPrice(order);
            double fill_quantity = order.quantity - order.filled_quantity;
            
            order.filled_quantity = order.quantity;
            order.avg_fill_price = fill_price;
            order.status = OrderStatus::FILLED;
            
            // Update balances
            updateBalanceAfterTrade(order, fill_quantity, fill_price);
            
            // Notify subscribers
            notifyOrderUpdate(order);
            notifyTradeUpdate(order, fill_quantity, fill_price);
            
            // Move to executed orders
            executed_orders_[order_id] = order;
        }
    }
    
    // Remove filled orders from pending
    for (auto it = pending_orders_.begin(); it != pending_orders_.end();) {
        if (it->second.status == OrderStatus::FILLED) {
            it = pending_orders_.erase(it);
        } else {
            ++it;
        }
    }
}

bool MockExchange::validateOrder(const MockOrder& order) const {
    // Check if symbol exists
    {
        std::lock_guard<std::mutex> lock(market_data_mutex_);
        if (market_data_.find(order.symbol) == market_data_.end()) {
            return false;
        }
    }
    
    // Check minimum quantity
    if (order.quantity <= 0) {
        return false;
    }
    
    // Check price for limit orders
    if (order.type == OrderType::LIMIT && order.price <= 0) {
        return false;
    }
    
    // Check balance for sell orders
    if (order.side == OrderSide::SELL) {
        std::string base_asset = getBaseAsset(order.symbol);
        if (getBalance(base_asset) < order.quantity) {
            return false;
        }
    }
    
    return true;
}

bool MockExchange::shouldExecuteOrder(const MockOrder& order) const {
    // Simulate market conditions for order execution
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_real_distribution<> dis(0.0, 1.0);
    
    // Higher probability for market orders
    if (order.type == OrderType::MARKET) {
        return dis(gen) < 0.9;
    }
    
    // For limit orders, check if price is achievable
    try {
        auto market_data = getMarketData(order.symbol);
        if (order.side == OrderSide::BUY && order.price >= market_data.ask) {
            return dis(gen) < 0.8;
        } else if (order.side == OrderSide::SELL && order.price <= market_data.bid) {
            return dis(gen) < 0.8;
        }
    } catch (...) {
        // If market data is not available, random execution
        return dis(gen) < 0.3;
    }
    
    return false;
}

double MockExchange::calculateFillPrice(const MockOrder& order) const {
    try {
        auto market_data = getMarketData(order.symbol);
        
        if (order.type == OrderType::MARKET) {
            double base_price = (order.side == OrderSide::BUY) ? market_data.ask : market_data.bid;
            
            // Apply slippage
            static std::random_device rd;
            static std::mt19937 gen(rd());
            static std::uniform_real_distribution<> dis(-slippage_rate_, slippage_rate_);
            
            double slippage = dis(gen);
            return base_price * (1.0 + slippage);
        } else {
            return order.price;
        }
    } catch (...) {
        return order.price;
    }
}

void MockExchange::updateBalanceAfterTrade(const MockOrder& order, double fill_quantity, double fill_price) {
    std::string base_asset = getBaseAsset(order.symbol);
    std::string quote_asset = getQuoteAsset(order.symbol);
    
    double fee = fill_quantity * fill_price * fee_rate_;
    
    if (order.side == OrderSide::BUY) {
        balances_[base_asset] += fill_quantity;
        balances_[quote_asset] -= (fill_quantity * fill_price + fee);
    } else {
        balances_[base_asset] -= fill_quantity;
        balances_[quote_asset] += (fill_quantity * fill_price - fee);
    }
}

bool MockExchange::shouldSimulateError() const {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_real_distribution<> dis(0.0, 1.0);
    
    return dis(gen) < error_rate_;
}

Order MockExchange::convertToOrder(const MockOrder& mock_order) const {
    Order order;
    order.symbol = mock_order.symbol;
    order.side = mock_order.side;
    order.type = mock_order.type;
    order.quantity = mock_order.quantity;
    order.price = mock_order.price;
    order.timestamp = mock_order.timestamp;
    return order;
}

void MockExchange::notifyMarketDataUpdate(const std::string& symbol, const MarketData& data) {
    std::lock_guard<std::mutex> lock(callbacks_mutex_);
    auto it = market_data_callbacks_.find(symbol);
    if (it != market_data_callbacks_.end() && it->second) {
        it->second(data);
    }
}

void MockExchange::notifyOrderUpdate(const MockOrder& order) {
    std::lock_guard<std::mutex> lock(callbacks_mutex_);
    if (order_update_callback_) {
        OrderUpdate update;
        update.order_id = order.order_id;
        update.symbol = order.symbol;
        update.side = order.side;
        update.type = order.type;
        update.quantity = order.quantity;
        update.price = order.price;
        update.status = order.status;
        update.filled_quantity = order.filled_quantity;
        update.avg_fill_price = order.avg_fill_price;
        update.timestamp = order.timestamp;
        
        order_update_callback_(update);
    }
}

void MockExchange::notifyTradeUpdate(const MockOrder& order, double fill_quantity, double fill_price) {
    std::lock_guard<std::mutex> lock(callbacks_mutex_);
    if (trade_update_callback_) {
        TradeUpdate update;
        update.trade_id = order.order_id + "_trade";
        update.symbol = order.symbol;
        update.side = order.side;
        update.quantity = fill_quantity;
        update.price = fill_price;
        update.fee = fill_quantity * fill_price * fee_rate_;
        update.timestamp = std::chrono::system_clock::now();
        
        trade_update_callback_(update);
    }
}

std::string MockExchange::getBaseAsset(const std::string& symbol) const {
    // Simple parsing for symbols like "BTCUSD"
    size_t pos = symbol.find("USD");
    if (pos != std::string::npos) {
        return symbol.substr(0, pos);
    }
    pos = symbol.find("EUR");
    if (pos != std::string::npos) {
        return symbol.substr(0, pos);
    }
    pos = symbol.find("BTC");
    if (pos != std::string::npos && pos > 0) {
        return symbol.substr(0, pos);
    }
    return symbol.substr(0, 3); // Default to first 3 characters
}

std::string MockExchange::getQuoteAsset(const std::string& symbol) const {
    // Simple parsing for symbols like "BTCUSD"
    size_t pos = symbol.find("USD");
    if (pos != std::string::npos) {
        return "USD";
    }
    pos = symbol.find("EUR");
    if (pos != std::string::npos) {
        return "EUR";
    }
    pos = symbol.find("BTC");
    if (pos != std::string::npos && pos > 0) {
        return "BTC";
    }
    return symbol.substr(3); // Default to remaining characters
}

} // namespace testing
} // namespace moneybot
