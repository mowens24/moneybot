#ifndef ORDER_MANAGER_H
#define ORDER_MANAGER_H

#include "logger.h"
#include "types.h"
#include <boost/asio.hpp>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <nlohmann/json.hpp>

namespace moneybot {

class OrderManager {
public:
    using OrderCallback = std::function<void(const OrderAck&)>;
    using RejectCallback = std::function<void(const OrderReject&)>;
    using FillCallback = std::function<void(const OrderFill&)>;
    
    OrderManager(std::shared_ptr<Logger> logger, const nlohmann::json& config);
    ~OrderManager();
    
    // Order management
    std::string placeOrder(const Order& order, OrderCallback ack_cb = nullptr, 
                          RejectCallback reject_cb = nullptr, FillCallback fill_cb = nullptr);
    bool cancelOrder(const std::string& order_id);
    bool cancelAllOrders(const std::string& symbol);
    
    // Account information
    std::vector<Balance> getBalances();
    std::vector<Position> getPositions();
    std::vector<Order> getOpenOrders(const std::string& symbol = "");
    
    // Lifecycle
    void start();
    void stop();
    
    // Configuration
    void updateConfig(const nlohmann::json& config);
    
    // User data stream
    std::string createUserDataStream();

    // WebSocket handlers (made public for event routing)
    void handleOrderUpdate(const nlohmann::json& data);
    void handleAccountUpdate(const nlohmann::json& data);

private:
    // REST API methods
    nlohmann::json makeRequest(const std::string& endpoint, const std::string& method = "GET", 
                              const nlohmann::json& data = {});
    std::string signRequest(const std::string& query_string);
    
    // Internal helpers
    std::string generateClientOrderId();
    void registerCallbacks(const std::string& order_id, OrderCallback ack_cb, 
                          RejectCallback reject_cb, FillCallback fill_cb);
    
    std::shared_ptr<Logger> logger_;
    nlohmann::json config_;
    
    // API credentials
    std::string api_key_;
    std::string secret_key_;
    std::string base_url_;
    
    // Callback storage
    struct OrderCallbacks {
        OrderCallback ack_callback;
        RejectCallback reject_callback;
        FillCallback fill_callback;
    };
    std::unordered_map<std::string, OrderCallbacks> callbacks_;
    
    // HTTP client
    boost::asio::io_context ioc_;
    std::unique_ptr<boost::asio::ip::tcp::resolver> resolver_;
    
    // Thread safety
    mutable std::mutex callbacks_mutex_;
    mutable std::mutex config_mutex_;
    
    // State
    bool running_;
    std::thread worker_thread_;
};

} // namespace moneybot

#endif // ORDER_MANAGER_H