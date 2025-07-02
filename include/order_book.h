#ifndef ORDER_BOOK_H
#define ORDER_BOOK_H

#include "logger.h"
#include <sqlite3.h>
#include <nlohmann/json.hpp>
#include <vector>
#include <map>
#include <queue>

namespace moneybot {
    class OrderBook {
    public:
        explicit OrderBook(std::shared_ptr<Logger> logger);
        ~OrderBook();
        void update(const nlohmann::json& depth_data);
        std::pair<double, double> getBestBidAsk() const;
        double getBestBid() const;
        double getBestAsk() const;
        void flushBatch();
        void pruneOldData(int64_t max_age_ms);
    private:
        struct Tick {
            int64_t timestamp;
            std::string symbol;
            double bid_price, bid_qty, ask_price, ask_qty;
        };
        std::shared_ptr<Logger> logger_;
        sqlite3* db_;
        std::map<double, double, std::greater<double>> bids_;
        std::map<double, double> asks_;
        std::queue<Tick> insert_queue_;
        static constexpr size_t BATCH_SIZE = 100;
        static constexpr int64_t PRUNE_INTERVAL_MS = 60000;
        int64_t last_prune_time_ = 0;
        void openDatabase();
        void closeDatabase();
        void storeTick(const Tick& tick);
        void initializeSchema();
    };
} // namespace moneybot

#endif // ORDER_BOOK_H