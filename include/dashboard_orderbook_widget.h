#pragma once
#include <nlohmann/json.hpp>
#include <vector>
#include <string>

namespace moneybot {

// Simple order book entry struct
struct OrderBookEntry {
    double price;
    double size;
};

// Renders the order book widget in the dashboard
// bids and asks: vectors of price/size pairs (descending for bids, ascending for asks)
void RenderOrderBookWidget(const std::vector<OrderBookEntry>& bids, const std::vector<OrderBookEntry>& asks);

} // namespace moneybot
