#include "moneybot.h"
#include "logger.h"
#include "data_analyzer.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <signal.h>
#include <atomic>

using json = nlohmann::json;

std::atomic<bool> running(true);

void signalHandler(int signum) {
    std::cout << "\nReceived signal " << signum << ". Shutting down gracefully..." << std::endl;
    running = false;
}

void printUsage(const char* program) {
    std::cout << "Usage: " << program << " [OPTIONS]\n"
              << "Options:\n"
              << "  --analyze     Run in analysis mode (no trading)\n"
              << "  --config FILE Use custom config file (default: config.json)\n"
              << "  --help        Show this help message\n"
              << "  --dry-run     Run without placing real orders\n"
              << std::endl;
}

int main(int argc, char* argv[]) {
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    
    std::string config_file = "config.json";
    bool analyze_mode = false;
    bool dry_run = false;
    
    // Parse command line arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--help" || arg == "-h") {
            printUsage(argv[0]);
            return 0;
        } else if (arg == "--analyze") {
            analyze_mode = true;
        } else if (arg == "--config") {
            if (i + 1 < argc) {
                config_file = argv[++i];
            } else {
                std::cerr << "Error: --config requires a filename" << std::endl;
                return 1;
            }
        } else if (arg == "--dry-run") {
            dry_run = true;
        } else {
            std::cerr << "Unknown option: " << arg << std::endl;
            printUsage(argv[0]);
            return 1;
        }
    }

    if (analyze_mode) {
        auto logger = std::make_shared<moneybot::Logger>();
        moneybot::DataAnalyzer analyzer(logger);
        std::cout << "=== Data Analysis Mode ===" << std::endl;
        std::cout << "Average Spread: " << analyzer.computeAverageSpread() << std::endl;
        std::cout << "VWAP (Bids): " << analyzer.computeVWAP(true) << std::endl;
        auto ticks = analyzer.getTicksByTime("BTCUSDT", 0, std::numeric_limits<int64_t>::max());
        std::cout << "Total ticks: " << ticks.size() << std::endl;
        return 0;
    }

    // Load configuration
    std::ifstream config_stream(config_file);
    if (!config_stream.is_open()) {
        std::cerr << "Error: Could not open config file: " << config_file << std::endl;
        return 1;
    }

    json config;
    try {
        config_stream >> config;
    } catch (const std::exception& e) {
        std::cerr << "Error: Invalid JSON in config file: " << e.what() << std::endl;
        return 1;
    }

    // Set dry-run mode if requested
    if (dry_run) {
        config["exchange"]["rest_api"]["api_key"] = "dry_run_mode";
        config["exchange"]["rest_api"]["secret_key"] = "dry_run_mode";
        std::cout << "Running in DRY-RUN mode (no real orders will be placed)" << std::endl;
    }

    std::cout << "=== MoneyBot HFT Trading System ===" << std::endl;
    std::cout << "Strategy: " << config["strategy"]["type"].get<std::string>() << std::endl;
    std::cout << "Symbol: " << config["strategy"]["symbol"].get<std::string>() << std::endl;
    std::cout << "Exchange: " << config["exchange"]["rest_api"]["base_url"].get<std::string>() << std::endl;
    std::cout << "Press Ctrl+C to stop" << std::endl;

    try {
        moneybot::TradingEngine engine(config);
        engine.start();
        
        // Main loop
        while (running) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            
            // Print status every 30 seconds
            static int counter = 0;
            if (++counter % 30 == 0) {
                auto status = engine.getStatus();
                auto metrics = engine.getPerformanceMetrics();
                
                std::cout << "\n=== Status Update ===" << std::endl;
                std::cout << "Running: " << (status["running"].get<bool>() ? "Yes" : "No") << std::endl;
                std::cout << "Uptime: " << status["uptime_seconds"].get<int64_t>() << " seconds" << std::endl;
                std::cout << "Total PnL: $" << std::fixed << std::setprecision(2) << metrics["total_pnl"].get<double>() << std::endl;
                std::cout << "Total Trades: " << metrics["total_trades"].get<int>() << std::endl;
                
                if (status.contains("risk")) {
                    auto risk = status["risk"];
                    std::cout << "Emergency Stop: " << (risk["emergency_stopped"].get<bool>() ? "Yes" : "No") << std::endl;
                    std::cout << "Drawdown: " << std::fixed << std::setprecision(2) << risk["drawdown"].get<double>() << "%" << std::endl;
                }
            }
        }
        
        std::cout << "\nShutting down..." << std::endl;
        engine.stop();
        
        // Final status report
        auto final_metrics = engine.getPerformanceMetrics();
        std::cout << "\n=== Final Report ===" << std::endl;
        std::cout << "Total PnL: $" << std::fixed << std::setprecision(2) << final_metrics["total_pnl"].get<double>() << std::endl;
        std::cout << "Total Trades: " << final_metrics["total_trades"].get<int>() << std::endl;
        std::cout << "Avg PnL per Trade: $" << std::fixed << std::setprecision(2) << final_metrics["avg_pnl_per_trade"].get<double>() << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}