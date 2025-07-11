#include "modern_logger.h"
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <filesystem>
#include <iostream>

namespace moneybot {

void ModernLogger::initialize(const std::string& log_level, 
                       const std::string& log_file,
                       size_t max_file_size,
                       size_t max_files) {
    try {
        // Create logs directory if it doesn't exist
        std::filesystem::path log_path(log_file);
        std::filesystem::create_directories(log_path.parent_path());

        // Create sinks
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
            log_file, max_file_size, max_files);

        // Console sink format (colored, shorter format)
        console_sink->set_pattern("[%^%l%$] %v");
        
        // File sink format (detailed with timestamp)
        file_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%t] [%^%l%$] %v");

        // Create main logger
        std::vector<spdlog::sink_ptr> sinks{console_sink, file_sink};
        logger_ = std::make_shared<spdlog::logger>("moneybot", sinks.begin(), sinks.end());
        logger_->set_level(spdlog::level::from_str(log_level));
        logger_->flush_on(spdlog::level::warn);

        // Create trade logger (separate file)
        auto trade_file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
            "logs/trades.log", max_file_size, max_files);
        trade_file_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] %v");
        trade_logger_ = std::make_shared<spdlog::logger>("trade", trade_file_sink);
        trade_logger_->set_level(spdlog::level::info);
        trade_logger_->flush_on(spdlog::level::info);

        // Create performance logger (separate file)
        auto perf_file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
            "logs/performance.log", max_file_size, max_files);
        perf_file_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] %v");
        performance_logger_ = std::make_shared<spdlog::logger>("performance", perf_file_sink);
        performance_logger_->set_level(spdlog::level::info);
        performance_logger_->flush_on(spdlog::level::info);

        // Register loggers
        spdlog::register_logger(logger_);
        spdlog::register_logger(trade_logger_);
        spdlog::register_logger(performance_logger_);

        info("Logger initialized successfully - Level: " + log_level);
        
    } catch (const std::exception& e) {
        std::cerr << "Failed to initialize logger: " << e.what() << std::endl;
        throw;
    }
}

void ModernLogger::debug(const std::string& message) {
    if (logger_) logger_->debug(message);
}

void ModernLogger::info(const std::string& message) {
    if (logger_) logger_->info(message);
}

void ModernLogger::warn(const std::string& message) {
    if (logger_) logger_->warn(message);
}

void ModernLogger::error(const std::string& message) {
    if (logger_) logger_->error(message);
}

void ModernLogger::critical(const std::string& message) {
    if (logger_) logger_->critical(message);
}

void ModernLogger::trade(const std::string& message) {
    if (trade_logger_) trade_logger_->info(message);
}

void ModernLogger::performance(const std::string& message) {
    if (performance_logger_) performance_logger_->info(message);
}

void ModernLogger::setLevel(const std::string& level) {
    if (logger_) {
        logger_->set_level(spdlog::level::from_str(level));
        info("Log level changed to: " + level);
    }
}

} // namespace moneybot
