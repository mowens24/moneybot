#include "order_book.h"
#include <chrono>

namespace moneybot {
    OrderBook::OrderBook(std::shared_ptr<Logger> logger) : logger_(logger) {
        openDatabase();
        initializeSchema();
    }

    OrderBook::~OrderBook() {
        flushBatch();
        closeDatabase();
    }

    void OrderBook::openDatabase() {
        if (sqlite3_open("data/ticks.db", &db_) != SQLITE_OK) {
            logger_->getLogger()->error("Failed to open ticks.db: {}", sqlite3_errmsg(db_));
            throw std::runtime_error("Database open failed");
        }
    }

    void OrderBook::closeDatabase() {
        if (db_) {
            sqlite3_close(db_);
            db_ = nullptr;
        }
    }

    void OrderBook::initializeSchema() {
        const char* sql = "CREATE TABLE IF NOT EXISTS ticks ("
                          "timestamp INTEGER NOT NULL, "
                          "symbol TEXT NOT NULL, "
                          "bid_price REAL, "
                          "bid_qty REAL, "
                          "ask_price REAL, "
                          "ask_qty REAL, "
                          "PRIMARY KEY (timestamp, symbol)"
                          ");"
                          "CREATE INDEX IF NOT EXISTS idx_ticks_time ON ticks (timestamp);"
                          "CREATE INDEX IF NOT EXISTS idx_ticks_symbol ON ticks (symbol);";
        char* err_msg = nullptr;
        if (sqlite3_exec(db_, sql, nullptr, nullptr, &err_msg) != SQLITE_OK) {
            std::string error = err_msg ? err_msg : "Unknown error";
            logger_->getLogger()->error("Schema creation failed: {}", error);
            sqlite3_free(err_msg);
            throw std::runtime_error("Schema creation failed: " + error);
        }
        logger_->getLogger()->info("Database schema initialized successfully.");
    }

    void OrderBook::storeTick(const Tick& tick) {
        insert_queue_.push(tick);
        if (insert_queue_.size() >= BATCH_SIZE) {
            flushBatch();
        }
    }

    void OrderBook::flushBatch() {
        if (insert_queue_.empty()) return;
        const char* sql = "INSERT OR REPLACE INTO ticks (timestamp, symbol, bid_price, bid_qty, ask_price, ask_qty) "
                          "VALUES (?, ?, ?, ?, ?, ?);";
        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
            logger_->getLogger()->error("Prepare failed: {}", sqlite3_errmsg(db_));
            return;
        }

        sqlite3_exec(db_, "BEGIN TRANSACTION;", nullptr, nullptr, nullptr);
        while (!insert_queue_.empty()) {
            const auto& tick = insert_queue_.front();
            sqlite3_bind_int64(stmt, 1, tick.timestamp);
            sqlite3_bind_text(stmt, 2, tick.symbol.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_double(stmt, 3, tick.bid_price);
            sqlite3_bind_double(stmt, 4, tick.bid_qty);
            sqlite3_bind_double(stmt, 5, tick.ask_price);
            sqlite3_bind_double(stmt, 6, tick.ask_qty);
            if (sqlite3_step(stmt) != SQLITE_DONE) {
                logger_->getLogger()->error("Insert failed: {}", sqlite3_errmsg(db_));
            }
            sqlite3_reset(stmt);
            insert_queue_.pop();
        }
        sqlite3_exec(db_, "COMMIT;", nullptr, nullptr, nullptr);
        sqlite3_finalize(stmt);
        logger_->getLogger()->info("Flushed {} ticks to database", BATCH_SIZE);
    }

    void OrderBook::pruneOldData(int64_t max_age_ms) {
        auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        if (now - last_prune_time_ < PRUNE_INTERVAL_MS) return;

        int64_t cutoff = now - max_age_ms;
        const char* sql = "DELETE FROM ticks WHERE timestamp < ?;";
        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_int64(stmt, 1, cutoff);
            if (sqlite3_step(stmt) == SQLITE_DONE) {
                int rows = sqlite3_changes(db_);
                if (rows > 0) {
                    logger_->getLogger()->info("Pruned {} old ticks", rows);
                }
            }
            sqlite3_finalize(stmt);
        }
        last_prune_time_ = now;
    }

    void OrderBook::update(const nlohmann::json& depth_data) {
        if (!depth_data.contains("s") || !depth_data.contains("E") ||
            !depth_data.contains("bids") || !depth_data.contains("asks")) {
            logger_->getLogger()->error("Invalid depth data format");
            return;
        }
        bids_.clear();
        asks_.clear();
        std::string symbol = depth_data["s"].get<std::string>();
        int64_t timestamp = depth_data["E"].get<int64_t>();

        for (const auto& bid : depth_data["bids"]) {
            double price = std::stod(bid[0].get<std::string>());
            double qty = std::stod(bid[1].get<std::string>());
            bids_[price] = qty;
        }
        for (const auto& ask : depth_data["asks"]) {
            double price = std::stod(ask[0].get<std::string>());
            double qty = std::stod(ask[1].get<std::string>());
            asks_[price] = qty;
        }

        if (!bids_.empty() && !asks_.empty()) {
            Tick tick{
                timestamp,
                symbol,
                bids_.begin()->first, bids_.begin()->second,
                asks_.begin()->first, asks_.begin()->second
            };
            storeTick(tick);
            logger_->getLogger()->info("Updated order book: bid={:.2f}, ask={:.2f}", tick.bid_price, tick.ask_price);
        }

        pruneOldData(24 * 3600 * 1000); // Keep 24 hours
    }

    std::pair<double, double> OrderBook::getBestBidAsk() const {
        if (!bids_.empty()) {
            return {bids_.begin()->first, bids_.begin()->second};
        }
        return {0.0, 0.0};
    }

    double OrderBook::getBestBid() const {
        if (!bids_.empty()) {
            return bids_.begin()->first;
        }
        return 0.0;
    }

    double OrderBook::getBestAsk() const {
        if (!asks_.empty()) {
            return asks_.begin()->first;
        }
        return 0.0;
    }
} // namespace moneybot