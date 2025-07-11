#pragma once

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/fmt/ostr.h>
#include <fmt/format.h>
#include <memory>
#include <string>

namespace moneybot {

class ModernLogger {
public:
    static ModernLogger& getInstance() {
        static ModernLogger instance;
        return instance;
    }

    void initialize(const std::string& log_level = "info", 
                   const std::string& log_file = "logs/moneybot.log",
                   size_t max_file_size = 1024 * 1024 * 10, // 10MB
                   size_t max_files = 5);

    // Simple logging functions
    void debug(const std::string& message);
    void info(const std::string& message);
    void warn(const std::string& message);
    void error(const std::string& message);
    void critical(const std::string& message);
    
    // Trading specific logging
    void trade(const std::string& message);
    void performance(const std::string& message);

    void flush() {
        if (logger_) logger_->flush();
        if (trade_logger_) trade_logger_->flush();
        if (performance_logger_) performance_logger_->flush();
    }

    void setLevel(const std::string& level);

private:
    ModernLogger() = default;
    ~ModernLogger() = default;
    ModernLogger(const ModernLogger&) = delete;
    ModernLogger& operator=(const ModernLogger&) = delete;

    std::shared_ptr<spdlog::logger> logger_;
    std::shared_ptr<spdlog::logger> trade_logger_;
    std::shared_ptr<spdlog::logger> performance_logger_;
};

// Convenience macros for formatted logging
#define LOG_DEBUG(msg) moneybot::ModernLogger::getInstance().debug(msg)
#define LOG_INFO(msg) moneybot::ModernLogger::getInstance().info(msg)
#define LOG_WARN(msg) moneybot::ModernLogger::getInstance().warn(msg)
#define LOG_ERROR(msg) moneybot::ModernLogger::getInstance().error(msg)
#define LOG_CRITICAL(msg) moneybot::ModernLogger::getInstance().critical(msg)
#define LOG_TRADE(msg) moneybot::ModernLogger::getInstance().trade(msg)
#define LOG_PERFORMANCE(msg) moneybot::ModernLogger::getInstance().performance(msg)

// Formatted logging macros using fmt::format
#define LOG_DEBUG_FMT(fmt, ...) moneybot::ModernLogger::getInstance().debug(fmt::format(fmt, __VA_ARGS__))
#define LOG_INFO_FMT(fmt, ...) moneybot::ModernLogger::getInstance().info(fmt::format(fmt, __VA_ARGS__))
#define LOG_WARN_FMT(fmt, ...) moneybot::ModernLogger::getInstance().warn(fmt::format(fmt, __VA_ARGS__))
#define LOG_ERROR_FMT(fmt, ...) moneybot::ModernLogger::getInstance().error(fmt::format(fmt, __VA_ARGS__))
#define LOG_CRITICAL_FMT(fmt, ...) moneybot::ModernLogger::getInstance().critical(fmt::format(fmt, __VA_ARGS__))
#define LOG_TRADE_FMT(fmt, ...) moneybot::ModernLogger::getInstance().trade(fmt::format(fmt, __VA_ARGS__))
#define LOG_PERFORMANCE_FMT(fmt, ...) moneybot::ModernLogger::getInstance().performance(fmt::format(fmt, __VA_ARGS__))

} // namespace moneybot
