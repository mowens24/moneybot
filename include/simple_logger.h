#ifndef SIMPLE_LOGGER_H
#define SIMPLE_LOGGER_H

#include <iostream>
#include <string>
#include <memory>
#include <chrono>
#include <iomanip>
#include <sstream>

namespace moneybot {
    
enum class LogLevel {
    DEBUG = 0,
    INFO = 1,
    WARNING = 2,
    ERROR = 3
};

class SimpleLogger {
private:
    LogLevel min_level_ = LogLevel::INFO;
    
    std::string get_timestamp() {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
        ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
        return ss.str();
    }
    
    std::string level_to_string(LogLevel level) {
        switch (level) {
            case LogLevel::DEBUG:   return "DEBUG";
            case LogLevel::INFO:    return "INFO";
            case LogLevel::WARNING: return "WARN";
            case LogLevel::ERROR:   return "ERROR";
            default:                return "UNKNOWN";
        }
    }
    
public:
    SimpleLogger() = default;
    
    void set_level(LogLevel level) {
        min_level_ = level;
    }
    
    void log(LogLevel level, const std::string& message) {
        if (level >= min_level_) {
            std::cout << "[" << get_timestamp() << "] [" 
                      << level_to_string(level) << "] " 
                      << message << std::endl;
        }
    }
    
    void debug(const std::string& message) {
        log(LogLevel::DEBUG, message);
    }
    
    void info(const std::string& message) {
        log(LogLevel::INFO, message);
    }
    
    void warning(const std::string& message) {
        log(LogLevel::WARNING, message);
    }
    
    void error(const std::string& message) {
        log(LogLevel::ERROR, message);
    }
};

// Alias for compatibility with existing code
using Logger = SimpleLogger;

} // namespace moneybot

#endif // SIMPLE_LOGGER_H
