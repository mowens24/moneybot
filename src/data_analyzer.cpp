#include "data_analyzer.h"
#include <sqlite3.h>

namespace moneybot {
    DataAnalyzer::DataAnalyzer(std::shared_ptr<Logger> logger) : logger_(logger) {
        openDatabase();
    }

    DataAnalyzer::~DataAnalyzer() {
        closeDatabase();
    }

    void DataAnalyzer::openDatabase() {
        if (sqlite3_open("data/ticks.db", &db_) != SQLITE_OK) {
            logger_->getLogger()->error("Failed to open ticks.db: {}", sqlite3_errmsg(db_));
            throw std::runtime_error("Database open failed");
        }
    }

    void DataAnalyzer::closeDatabase() {
        if (db_) {
            sqlite3_close(db_);
            db_ = nullptr;
        }
    }

    double DataAnalyzer::computeAverageSpread() const {
        const char* sql = "SELECT AVG(ask_price - bid_price) FROM ticks WHERE bid_price > 0 AND ask_price > 0;";
        sqlite3_stmt* stmt;
        double spread = 0.0;
        if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) == SQLITE_OK) {
            if (sqlite3_step(stmt) == SQLITE_ROW) {
                spread = sqlite3_column_double(stmt, 0);
            }
            sqlite3_finalize(stmt);
        } else {
            logger_->getLogger()->error("Query failed: {}", sqlite3_errmsg(db_));
        }
        return spread;
    }

    double DataAnalyzer::computeVWAP(bool bids) const {
        const char* sql = bids ? "SELECT SUM(bid_price * bid_qty) / SUM(bid_qty) FROM ticks WHERE bid_qty > 0;"
                               : "SELECT SUM(ask_price * ask_qty) / SUM(ask_qty) FROM ticks WHERE ask_qty > 0;";
        sqlite3_stmt* stmt;
        double vwap = 0.0;
        if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) == SQLITE_OK) {
            if (sqlite3_step(stmt) == SQLITE_ROW) {
                vwap = sqlite3_column_double(stmt, 0);
            }
            sqlite3_finalize(stmt);
        } else {
            logger_->getLogger()->error("Query failed: {}", sqlite3_errmsg(db_));
        }
        return vwap;
    }

    std::vector<std::tuple<int64_t, double, double>> DataAnalyzer::getTicksByTime(
        const std::string& symbol, int64_t start_time, int64_t end_time) const {
        std::vector<std::tuple<int64_t, double, double>> ticks;
        const char* sql = "SELECT timestamp, bid_price, bid_qty FROM ticks "
                          "WHERE symbol = ? AND timestamp BETWEEN ? AND ? "
                          "ORDER BY timestamp;";
        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_text(stmt, 1, symbol.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_int64(stmt, 2, start_time);
            sqlite3_bind_int64(stmt, 3, end_time);
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                ticks.emplace_back(
                    sqlite3_column_int64(stmt, 0),
                    sqlite3_column_double(stmt, 1),
                    sqlite3_column_double(stmt, 2)
                );
            }
            sqlite3_finalize(stmt);
        } else {
            logger_->getLogger()->error("Query failed: {}", sqlite3_errmsg(db_));
        }
        return ticks;
    }
} // namespace moneybot