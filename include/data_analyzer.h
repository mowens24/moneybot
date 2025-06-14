#ifndef DATA_ANALYZER_H
#define DATA_ANALYZER_H

#include "logger.h"
#include <sqlite3.h>
#include <vector>
#include <string>
#include <tuple>
#include <cstdint> // For int64_t

namespace moneybot {
    class DataAnalyzer {
    public:
        explicit DataAnalyzer(std::shared_ptr<Logger> logger);
        ~DataAnalyzer();
        double computeAverageSpread() const;
        double computeVWAP(bool bids) const;
        std::vector<std::tuple<int64_t, double, double>> getTicksByTime(
            const std::string& symbol, int64_t start_time, int64_t end_time) const;
    private:
        std::shared_ptr<Logger> logger_;
        sqlite3* db_;
        void openDatabase();
        void closeDatabase();
    };
} // namespace moneybot

#endif // DATA_ANALYZER_H