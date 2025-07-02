#include "types.h"
#include <sstream>
#include <chrono>

namespace moneybot {

Trade::Trade(const nlohmann::json& data) {
    trade_id = data.value("t", "");
    symbol = data.value("s", "");
    price = std::stod(data.value("p", "0"));
    quantity = std::stod(data.value("q", "0"));
    
    std::string side_str = data.value("m", "false");
    side = (side_str == "true") ? OrderSide::SELL : OrderSide::BUY; // m=true means maker was seller
    
    timestamp = std::chrono::system_clock::now();
}

Order::Order(const nlohmann::json& data) {
    order_id = data.value("orderId", "");
    client_order_id = data.value("clientOrderId", "");
    symbol = data.value("symbol", "");
    
    std::string side_str = data.value("side", "");
    side = (side_str == "BUY") ? OrderSide::BUY : OrderSide::SELL;
    
    std::string type_str = data.value("type", "");
    if (type_str == "MARKET") type = OrderType::MARKET;
    else if (type_str == "LIMIT") type = OrderType::LIMIT;
    else if (type_str == "STOP_LOSS") type = OrderType::STOP_LOSS;
    else if (type_str == "TAKE_PROFIT") type = OrderType::TAKE_PROFIT;
    else type = OrderType::LIMIT;
    
    quantity = std::stod(data.value("origQty", "0"));
    price = std::stod(data.value("price", "0"));
    
    std::string status_str = data.value("status", "");
    if (status_str == "NEW") status = OrderStatus::PENDING;
    else if (status_str == "PARTIALLY_FILLED") status = OrderStatus::PARTIALLY_FILLED;
    else if (status_str == "FILLED") status = OrderStatus::FILLED;
    else if (status_str == "CANCELED") status = OrderStatus::CANCELLED;
    else if (status_str == "REJECTED") status = OrderStatus::REJECTED;
    else status = OrderStatus::PENDING;
    
    timestamp = std::chrono::system_clock::now();
}

Balance::Balance(const nlohmann::json& data) {
    asset = data.value("asset", "");
    free = std::stod(data.value("free", "0"));
    locked = std::stod(data.value("locked", "0"));
    total = free + locked;
}

Position::Position(const nlohmann::json& data) {
    symbol = data.value("symbol", "");
    quantity = std::stod(data.value("positionAmt", "0"));
    avg_price = std::stod(data.value("entryPrice", "0"));
    unrealized_pnl = std::stod(data.value("unRealizedProfit", "0"));
    realized_pnl = std::stod(data.value("realizedProfit", "0"));
}

} // namespace moneybot 