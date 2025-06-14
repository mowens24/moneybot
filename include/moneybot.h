#ifndef MONEYBOT_H
#define MONEYBOT_H

#include "logger.h"
#include "network.h"
#include "order_book.h"
#include <nlohmann/json.hpp>
#include <thread>

namespace moneybot {
    class TradingEngine {
    public:
        TradingEngine(const nlohmann::json& config);
        ~TradingEngine();
        void start();
        void stop();
    private:
        std::shared_ptr<Logger> logger_;
        std::shared_ptr<OrderBook> order_book_;
        std::shared_ptr<Network> network_;
        nlohmann::json config_;
        std::thread network_thread_;
    };
} // namespace moneybot

#endif // MONEYBOT_H