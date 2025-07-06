#ifndef LOGGER_H
#define LOGGER_H

#include <cstddef>
#include <cstdint>
#include <climits>
#include <spdlog/spdlog.h>
#include <memory>

namespace moneybot {
    class Logger {
    public:
        Logger();
        std::shared_ptr<spdlog::logger> getLogger() const { return logger_; }
    private:
        std::shared_ptr<spdlog::logger> logger_;
    };
} // namespace moneybot

#endif // LOGGER_H