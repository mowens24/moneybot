#ifndef MONEYBOT_H
#define MONEYBOT_H

#include <cstddef>
#include <cstdint>
#include <climits>
#include "logger.h"
#include "network.h"
#include "order_book.h"
#include "strategy.h"
#include "order_manager.h"
#include "risk_manager.h"
#include "market_maker_strategy.h"
#include <nlohmann/json.hpp>
#include <memory>
#include <thread>
#include <atomic>

namespace moneybot {
    class TradingEngine {
    public:
        TradingEngine(const nlohmann::json& config);
        ~TradingEngine();
        
        void start();
        void stop();
        void emergencyStop();
        
        // Status and monitoring
        bool isRunning() const { return running_.load(); }
        nlohmann::json getStatus() const;
        nlohmann::json getPerformanceMetrics() const;
        
        // Configuration
        void updateConfig(const nlohmann::json& config);
        
        // --- Live status helpers ---
        std::pair<double, double> getBestBidAsk() const {
            if (order_book_) return order_book_->getBestBidAsk();
            return {0.0, 0.0};
        }
        double getBestBid() const {
            if (order_book_) return order_book_->getBestBid();
            return 0.0;
        }
        double getBestAsk() const {
            if (order_book_) return order_book_->getBestAsk();
            return 0.0;
        }
        std::string getLastEvent() const { return last_event_; }
        void setLastEvent(const std::string& evt) { last_event_ = evt; }
        bool isWsConnected() const { return ws_connected_; }
        void setWsConnected(bool v) { ws_connected_ = v; }
        // --- End live status helpers ---
        
    private:
        // Event handlers
        void onOrderBookUpdate(const OrderBook& order_book);
        void onTrade(const Trade& trade);
        void onOrderAck(const OrderAck& ack);
        void onOrderReject(const OrderReject& reject);
        void onOrderFill(const OrderFill& fill);
        
        // Lifecycle management
        void initializeComponents();
        void shutdownComponents();
        
        // Thread management
        void networkThread();
        void strategyThread();
        
        // Configuration
        void loadConfig(const nlohmann::json& config);
        
        // Core components
        std::shared_ptr<Logger> logger_;
        std::shared_ptr<OrderBook> order_book_;
        std::shared_ptr<Network> network_;
        std::shared_ptr<OrderManager> order_manager_;
        std::shared_ptr<RiskManager> risk_manager_;
        std::shared_ptr<Strategy> strategy_;
        
        // Configuration
        nlohmann::json config_;
        
        // Threading
        std::thread network_thread_;
        std::thread strategy_thread_;
        std::atomic<bool> running_;
        std::atomic<bool> emergency_stop_;
        
        // Performance tracking
        std::chrono::system_clock::time_point start_time_;
        double total_pnl_;
        int total_trades_;
        
        // Thread safety
        mutable std::mutex config_mutex_;
        mutable std::mutex status_mutex_;
        
        // Live status helpers
        std::string last_event_;
        bool ws_connected_;
    };
} // namespace moneybot

#endif // MONEYBOT_H