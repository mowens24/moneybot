#include "logger.h"
#include <spdlog/sinks/rotating_file_sink.h>
#include <iostream>

namespace moneybot {
    Logger::Logger() {
        try {
            logger_ = spdlog::rotating_logger_mt("moneybot", "logs/moneybot.log", 1024 * 1024 * 5, 3);
            logger_->set_level(spdlog::level::info);
            logger_->info("Logger initialized.");
        } catch (const spdlog::spdlog_ex& ex) {
            std::cerr << "Logger initialization failed: " << ex.what() << std::endl;
        }
    }
} // namespace moneybot