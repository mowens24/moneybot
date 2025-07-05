#include "order_manager.h"
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <openssl/hmac.h>
#include <openssl/sha.h>
#include <sstream>
#include <iomanip>
#include <random>

namespace moneybot {

OrderManager::OrderManager(std::shared_ptr<Logger> logger, const nlohmann::json& config)
    : logger_(logger), config_(config), running_(false) {
    
    // Load API credentials
    api_key_ = config["exchange"]["rest_api"]["api_key"].get<std::string>();
    secret_key_ = config["exchange"]["rest_api"]["secret_key"].get<std::string>();
    base_url_ = config["exchange"]["rest_api"]["base_url"].get<std::string>();
    
    // Initialize HTTP client
    resolver_ = std::make_unique<boost::asio::ip::tcp::resolver>(ioc_);
    
    logger_->getLogger()->info("OrderManager initialized for {}", base_url_);
}

OrderManager::~OrderManager() {
    stop();
}

std::string OrderManager::placeOrder(const Order& order, OrderCallback ack_cb, 
                                    RejectCallback reject_cb, FillCallback fill_cb) {
    if (!running_) {
        logger_->getLogger()->warn("OrderManager not running, cannot place order");
        return "";
    }
    
    // Generate order ID
    std::string order_id = generateClientOrderId();
    
    // Register callbacks
    registerCallbacks(order_id, ack_cb, reject_cb, fill_cb);
    
    // Prepare order data
    nlohmann::json order_data;
    order_data["symbol"] = order.symbol;
    order_data["side"] = (order.side == OrderSide::BUY) ? "BUY" : "SELL";
    
    std::string type_str;
    switch (order.type) {
        case OrderType::MARKET: type_str = "MARKET"; break;
        case OrderType::LIMIT: type_str = "LIMIT"; break;
        case OrderType::STOP_LOSS: type_str = "STOP_LOSS"; break;
        case OrderType::TAKE_PROFIT: type_str = "TAKE_PROFIT"; break;
    }
    order_data["type"] = type_str;
    order_data["quantity"] = std::to_string(order.quantity);
    
    if (order.type == OrderType::LIMIT) {
        order_data["price"] = std::to_string(order.price);
        order_data["timeInForce"] = "GTC";
    }
    
    order_data["newClientOrderId"] = order_id;
    order_data["timestamp"] = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    // Make API request
    try {
        nlohmann::json response = makeRequest("/api/v3/order", "POST", order_data);
        
        if (response.contains("orderId")) {
            std::string server_order_id = response["orderId"].get<std::string>();
            
            // Simulate order acknowledgment
            OrderAck ack;
            ack.order_id = server_order_id;
            ack.client_order_id = order_id;
            ack.timestamp = std::chrono::system_clock::now();
            
            logger_->getLogger()->info("Order placed successfully: {} {} {} @ {}", 
                                      order.symbol, 
                                      order.side == OrderSide::BUY ? "BUY" : "SELL",
                                      order.quantity, order.price);
            
            // Call callback if provided
            if (ack_cb) {
                ack_cb(ack);
            }
            
            return server_order_id;
        } else {
            // Handle error response
            std::string error_msg = response.contains("msg") ? response["msg"].get<std::string>() : "Unknown error";
            
            OrderReject reject;
            reject.order_id = order_id;
            reject.client_order_id = order_id;
            reject.reason = error_msg;
            reject.timestamp = std::chrono::system_clock::now();
            
            logger_->getLogger()->error("Order placement failed: {}", error_msg);
            
            if (reject_cb) {
                reject_cb(reject);
            }
            
            return "";
        }
    } catch (const std::exception& e) {
        logger_->getLogger()->error("Order placement exception: {}", e.what());
        return "";
    }
}

bool OrderManager::cancelOrder(const std::string& order_id) {
    if (!running_) {
        logger_->getLogger()->warn("OrderManager not running, cannot cancel order");
        return false;
    }
    
    nlohmann::json cancel_data;
    cancel_data["orderId"] = order_id;
    cancel_data["timestamp"] = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    try {
        nlohmann::json response = makeRequest("/api/v3/order", "DELETE", cancel_data);
        
        if (response.contains("orderId")) {
            logger_->getLogger()->info("Order cancelled successfully: {}", order_id);
            
            // Remove callbacks
            {
                std::lock_guard<std::mutex> lock(callbacks_mutex_);
                callbacks_.erase(order_id);
            }
            
            return true;
        } else {
            std::string error_msg = response.contains("msg") ? response["msg"].get<std::string>() : "Unknown error";
            logger_->getLogger()->error("Order cancellation failed: {}", error_msg);
            return false;
        }
    } catch (const std::exception& e) {
        logger_->getLogger()->error("Order cancellation exception: {}", e.what());
        return false;
    }
}

bool OrderManager::cancelAllOrders(const std::string& symbol) {
    if (!running_) {
        logger_->getLogger()->warn("OrderManager not running, cannot cancel orders");
        return false;
    }
    
    nlohmann::json cancel_data;
    cancel_data["symbol"] = symbol;
    cancel_data["timestamp"] = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    try {
        nlohmann::json response = makeRequest("/api/v3/openOrders", "DELETE", cancel_data);
        
        if (response.is_array()) {
            logger_->getLogger()->info("Cancelled {} orders for symbol: {}", response.size(), symbol);
            
            // Clear all callbacks
            {
                std::lock_guard<std::mutex> lock(callbacks_mutex_);
                callbacks_.clear();
            }
            
            return true;
        } else {
            std::string error_msg = response.contains("msg") ? response["msg"].get<std::string>() : "Unknown error";
            logger_->getLogger()->error("Cancel all orders failed: {}", error_msg);
            return false;
        }
    } catch (const std::exception& e) {
        logger_->getLogger()->error("Cancel all orders exception: {}", e.what());
        return false;
    }
}

std::vector<Balance> OrderManager::getBalances() {
    try {
        nlohmann::json response = makeRequest("/api/v3/account", "GET");
        
        std::vector<Balance> balances;
        if (response.contains("balances")) {
            for (const auto& balance_data : response["balances"]) {
                Balance balance(balance_data);
                if (balance.total > 0) { // Only include non-zero balances
                    balances.push_back(balance);
                }
            }
        }
        
        return balances;
    } catch (const std::exception& e) {
        logger_->getLogger()->error("Failed to get balances: {}", e.what());
        return {};
    }
}

std::vector<Position> OrderManager::getPositions() {
    // Binance US doesn't support futures, so positions are always zero
    std::vector<Position> positions;
    
    Position btc_position;
    btc_position.symbol = "BTCUSDT";
    btc_position.quantity = 0.0;
    btc_position.avg_price = 0.0;
    btc_position.unrealized_pnl = 0.0;
    btc_position.realized_pnl = 0.0;
    positions.push_back(btc_position);
    
    return positions;
}

std::vector<Order> OrderManager::getOpenOrders(const std::string& symbol) {
    try {
        std::string endpoint = "/api/v3/openOrders";
        if (!symbol.empty()) {
            endpoint += "?symbol=" + symbol;
        }
        
        nlohmann::json response = makeRequest(endpoint, "GET");
        
        std::vector<Order> orders;
        if (response.is_array()) {
            for (const auto& order_data : response) {
                orders.emplace_back(order_data);
            }
        }
        
        return orders;
    } catch (const std::exception& e) {
        logger_->getLogger()->error("Failed to get open orders: {}", e.what());
        return {};
    }
}

void OrderManager::start() {
    if (running_) {
        logger_->getLogger()->warn("OrderManager already running");
        return;
    }
    
    running_ = true;
    logger_->getLogger()->info("OrderManager started");
}

void OrderManager::stop() {
    if (!running_) {
        return;
    }
    
    running_ = false;
    
    // Cancel all orders
    cancelAllOrders("");
    
    logger_->getLogger()->info("OrderManager stopped");
}

void OrderManager::updateConfig(const nlohmann::json& config) {
    std::lock_guard<std::mutex> lock(config_mutex_);
    config_ = config;
    
    // Update API credentials
    api_key_ = config["exchange"]["rest_api"]["api_key"].get<std::string>();
    secret_key_ = config["exchange"]["rest_api"]["secret_key"].get<std::string>();
    base_url_ = config["exchange"]["rest_api"]["base_url"].get<std::string>();
    
    logger_->getLogger()->info("OrderManager config updated");
}

nlohmann::json OrderManager::makeRequest(const std::string& endpoint, const std::string& method, 
                                        const nlohmann::json& data) {
    try {
        // Parse URL
        std::string host = base_url_.substr(base_url_.find("://") + 3);
        std::string port = "443";
        std::string path = endpoint;
        
        // Prepare query string for GET requests
        std::string query_string;
        if (method == "GET" && !data.empty()) {
            for (auto it = data.begin(); it != data.end(); ++it) {
                if (!query_string.empty()) query_string += "&";
                query_string += it.key() + "=" + it.value().get<std::string>();
            }
            if (!query_string.empty()) {
                path += "?" + query_string;
            }
        }
        
        // Add timestamp and signature for authenticated requests
        if (method != "GET") {
            if (!query_string.empty()) query_string += "&";
            query_string += "timestamp=" + std::to_string(
                std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count());
            
            if (!api_key_.empty()) {
                query_string += "&signature=" + signRequest(query_string);
            }
        }
        
        // Create HTTP request
        boost::beast::http::request<boost::beast::http::string_body> req;
        req.method_string(method);
        req.target(path + (method != "GET" && !query_string.empty() ? "?" + query_string : ""));
        req.version(11);
        req.set(boost::beast::http::field::host, host);
        req.set(boost::beast::http::field::user_agent, "MoneyBot/1.0");
        
        if (!api_key_.empty()) {
            req.set("X-MBX-APIKEY", api_key_);
        }
        
        if (method != "GET" && !data.empty()) {
            std::string body = data.dump();
            req.body() = body;
            req.set(boost::beast::http::field::content_type, "application/json");
            req.set(boost::beast::http::field::content_length, std::to_string(body.size()));
        }
        
        // Make request
        boost::asio::ip::tcp::resolver resolver(ioc_);
        auto const results = resolver.resolve(host, port);
        
        boost::asio::ip::tcp::socket socket(ioc_);
        boost::asio::connect(socket, results.begin(), results.end());
        
        boost::beast::http::write(socket, req);
        
        boost::beast::flat_buffer buffer;
        boost::beast::http::response<boost::beast::http::string_body> res;
        boost::beast::http::read(socket, buffer, res);
        
        // Parse response
        nlohmann::json response = nlohmann::json::parse(res.body());
        
        logger_->getLogger()->debug("API {} {}: {}", method, endpoint, response.dump());
        
        return response;
        
    } catch (const std::exception& e) {
        logger_->getLogger()->error("HTTP request failed: {}", e.what());
        return nlohmann::json::object();
    }
}

std::string OrderManager::signRequest(const std::string& query_string) {
    unsigned char hash[32];
    unsigned int hash_len;
    
    HMAC(EVP_sha256(), secret_key_.c_str(), secret_key_.length(),
         (unsigned char*)query_string.c_str(), query_string.length(),
         hash, &hash_len);
    
    std::stringstream ss;
    for (unsigned int i = 0; i < hash_len; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    
    return ss.str();
}

void OrderManager::handleOrderUpdate(const nlohmann::json& data) {
    logger_->getLogger()->debug("Received order update: {}", data.dump());
    
    // Handle order status updates
    if (data.contains("e") && data["e"].get<std::string>() == "executionReport") {
        std::string order_id = data["i"].get<std::string>();
        std::string status = data["X"].get<std::string>();
        
        std::lock_guard<std::mutex> lock(callbacks_mutex_);
        auto it = callbacks_.find(order_id);
        if (it != callbacks_.end()) {
            if (status == "FILLED" && it->second.fill_callback) {
                OrderFill fill;
                fill.order_id = order_id;
                fill.trade_id = data["t"].get<std::string>();
                fill.price = std::stod(data["L"].get<std::string>());
                fill.quantity = std::stod(data["l"].get<std::string>());
                fill.commission = std::stod(data["n"].get<std::string>());
                fill.commission_asset = data["N"].get<std::string>();
                fill.timestamp = std::chrono::system_clock::now();
                
                it->second.fill_callback(fill);
            }
        }
    }
}

void OrderManager::handleAccountUpdate(const nlohmann::json& data) {
    logger_->getLogger()->debug("Received account update: {}", data.dump());
}

std::string OrderManager::generateClientOrderId() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(100000, 999999);
    
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()).count();
    
    std::stringstream ss;
    ss << "OM_" << timestamp << "_" << dis(gen);
    return ss.str();
}

void OrderManager::registerCallbacks(const std::string& order_id, OrderCallback ack_cb, 
                                   RejectCallback reject_cb, FillCallback fill_cb) {
    std::lock_guard<std::mutex> lock(callbacks_mutex_);
    
    OrderCallbacks callbacks;
    callbacks.ack_callback = ack_cb;
    callbacks.reject_callback = reject_cb;
    callbacks.fill_callback = fill_cb;
    
    callbacks_[order_id] = callbacks;
}

std::string OrderManager::createUserDataStream() {
    try {
        std::string host = base_url_.substr(base_url_.find("://") + 3);
        std::string port = "443";
        std::string path = "/api/v3/userDataStream";

        // Create HTTP POST request
        boost::beast::http::request<boost::beast::http::string_body> req;
        req.method(boost::beast::http::verb::post);
        req.target(path);
        req.version(11);
        req.set(boost::beast::http::field::host, host);
        req.set(boost::beast::http::field::user_agent, "MoneyBot/1.0");
        if (!api_key_.empty()) {
            req.set("X-MBX-APIKEY", api_key_);
        }
        req.body() = "";
        req.prepare_payload();

        // Make HTTPS request (using plain TCP for now, should use SSL in production)
        boost::asio::ip::tcp::resolver resolver(ioc_);
        auto const results = resolver.resolve(host, port);
        boost::asio::ip::tcp::socket socket(ioc_);
        boost::asio::connect(socket, results.begin(), results.end());
        boost::beast::http::write(socket, req);
        boost::beast::flat_buffer buffer;
        boost::beast::http::response<boost::beast::http::string_body> res;
        boost::beast::http::read(socket, buffer, res);
        nlohmann::json response = nlohmann::json::parse(res.body());
        logger_->getLogger()->info("User data stream response: {}", response.dump());
        if (response.contains("listenKey")) {
            return response["listenKey"].get<std::string>();
        }
        logger_->getLogger()->error("Failed to get listenKey from user data stream response");
        return "";
    } catch (const std::exception& e) {
        logger_->getLogger()->error("Failed to create user data stream: {}", e.what());
        return "";
    }
}

} // namespace moneybot