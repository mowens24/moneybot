
#pragma once

#include <nlohmann/json.hpp>
#include "strategy.h"
#include "types.h"

namespace moneybot {

// DummyStrategy: A placeholder strategy for testing and backtesting. Tracks event counts and last event for debugging.
class DummyStrategy : public Strategy {
public:
    explicit DummyStrategy(const nlohmann::json& config)
        : config_(config), initialized_(false), trades_(0), fills_(0) {}

    void onOrderBookUpdate(const OrderBook&) override {}
    void onTrade(const Trade& trade) override {
        ++trades_;
        last_trade_ = trade;
    }
    void onOrderAck(const OrderAck& ack) override { last_ack_ = ack; }
    void onOrderReject(const OrderReject& reject) override { last_reject_ = reject; }
    void onOrderFill(const OrderFill& fill) override {
        ++fills_;
        last_fill_ = fill;
    }
    void initialize() override { initialized_ = true; }
    void shutdown() override { initialized_ = false; }
    std::string getName() const override { return "DummyStrategy"; }
    void updateConfig(const nlohmann::json& config) override { config_ = config; }

    // Dummy metrics for backtest reporting
    int getTradeCount() const { return trades_; }
    int getFillCount() const { return fills_; }
    bool isInitialized() const { return initialized_; }
    const Trade& getLastTrade() const { return last_trade_; }
    const OrderAck& getLastAck() const { return last_ack_; }
    const OrderReject& getLastReject() const { return last_reject_; }
    const OrderFill& getLastFill() const { return last_fill_; }

private:
    nlohmann::json config_;
    bool initialized_;
    int trades_;
    int fills_;
    Trade last_trade_;
    OrderAck last_ack_;
    OrderReject last_reject_;
    OrderFill last_fill_;
};

} // namespace moneybot
