#ifndef STRATEGY_H
#define STRATEGY_H

#include "order_book.h"
#include "types.h"
#include <memory>
#include <string>

namespace moneybot {

class Strategy {
public:
    virtual ~Strategy() = default;
    
    // Core strategy methods
    virtual void onOrderBookUpdate(const OrderBook& order_book) = 0;
    virtual void onTrade(const Trade& trade) = 0;
    virtual void onOrderAck(const OrderAck& ack) = 0;
    virtual void onOrderReject(const OrderReject& reject) = 0;
    virtual void onOrderFill(const OrderFill& fill) = 0;
    
    // Lifecycle methods
    virtual void initialize() = 0;
    virtual void shutdown() = 0;
    
    // Configuration
    virtual std::string getName() const = 0;
    virtual void updateConfig(const nlohmann::json& config) = 0;
};

} // namespace moneybot

#endif // STRATEGY_H 