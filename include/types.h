#pragma once

#include <string>
#include <chrono>
#include <nlohmann/json.hpp>

namespace moneybot {

enum class OrderSide {
    BUY,
    SELL
};

enum class OrderType {
    MARKET,
    LIMIT,
    STOP_LOSS,
    TAKE_PROFIT
};

enum class OrderStatus {
    PENDING,
    ACKNOWLEDGED,
    PARTIALLY_FILLED,
    FILLED,
    CANCELLED,
    REJECTED
};

struct Order {
    std::string order_id;
    std::string client_order_id;
    std::string symbol;
    OrderSide side;
    OrderType type;
    double quantity;
    double price;
    OrderStatus status;
    std::chrono::system_clock::time_point timestamp;
    
    Order() = default;
    Order(const nlohmann::json& data);
};

struct OrderAck {
    std::string order_id;
    std::string client_order_id;
    std::chrono::system_clock::time_point timestamp;
};

struct OrderReject {
    std::string order_id;
    std::string client_order_id;
    std::string reason;
    std::chrono::system_clock::time_point timestamp;
};

struct OrderFill {
    std::string order_id;
    std::string trade_id;
    double price;
    double quantity;
    double commission;
    std::string commission_asset;
    std::chrono::system_clock::time_point timestamp;
};

struct Trade {
    std::string trade_id;
    std::string symbol;
    double price;
    double quantity;
    OrderSide side;
    std::chrono::system_clock::time_point timestamp;
    
    Trade() = default;
    Trade(const nlohmann::json& data);
};

struct Balance {
    std::string asset;
    double free;
    double locked;
    double total;
    
    Balance() = default;
    Balance(const nlohmann::json& data);
};

struct Position {
    std::string symbol;
    double quantity;
    double avg_price;
    double unrealized_pnl;
    double realized_pnl;
    
    Position() = default;
    Position(const nlohmann::json& data);
};

struct OrderBookLevel {
    double price;
    double quantity;
    
    OrderBookLevel() = default;
    OrderBookLevel(double p, double q) : price(p), quantity(q) {}
};

} // namespace moneybot 