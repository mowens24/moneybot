#include "moneybot.h"
#include "logger.h"
#include "data_analyzer.h"
#include "backtest_engine.h"
#include "strategy_factory.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <signal.h>
#include <atomic>

using json = nlohmann::json;

// Forward declare GUI main function
int gui_main(int argc, char** argv);

std::atomic<bool> running(true);

void signalHandler(int signum) {
    std::cout << "\nReceived signal " << signum << ". Shutting down gracefully..." << std::endl;
    running = false;
}

void printUsage(const char* program) {
    std::cout << "Usage: " << program << " [OPTIONS]\n"
              << "Options:\n"
              << "  --gui, -g     Launch GUI dashboard mode\n"
              << "  --analyze     Run in analysis mode (no trading)\n"
              << "  --config FILE Use custom config file (default: config.json)\n"
              << "  --help        Show this help message\n"
              << "  --dry-run     Run without placing real orders\n"
              << "  --backtest    Run in backtest mode with historical data\n"
              << std::endl;
}

int main(int argc, char* argv[]) {
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    
    std::string config_file = "config.json";
    bool analyze_mode = false;
    bool dry_run = false;
    bool backtest_mode = false;
    std::string backtest_data = "data/ticks.db";

    // Parse command line arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--help" || arg == "-h") {
            printUsage(argv[0]);
            return 0;
        } else if (arg == "--gui" || arg == "-g") {
            std::cout << "Starting MoneyBot GUI mode..." << std::endl;
            return gui_main(argc, argv);
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
        } else if (arg == "--backtest") {
            backtest_mode = true;
            if (i + 1 < argc && argv[i+1][0] != '-') {
                backtest_data = argv[++i];
            }
        } else {
            std::cerr << "Unknown option: " << arg << std::endl;
            printUsage(argv[0]);
            return 1;
        }
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

    if (backtest_mode) {
        auto logger = std::make_shared<moneybot::Logger>();
        moneybot::BacktestEngine backtester(config);
        backtester.setLogger(logger);
        auto strat = moneybot::createStrategyFromConfig(config);
        backtester.setStrategy(strat);
        std::cout << "=== Backtest Mode ===" << std::endl;
        std::cout << "Loading data from: " << backtest_data << std::endl;
        auto result = backtester.run(config["strategy"]["symbol"], backtest_data);
        std::cout << "Backtest complete!" << std::endl;
        std::cout << "Total PnL: $" << result.total_pnl << std::endl;
        std::cout << "Max Drawdown: " << result.max_drawdown * 100 << "%" << std::endl;
        std::cout << "Total Trades: " << result.total_trades << std::endl;
        std::cout << "Win/Loss: " << result.wins << "/" << result.losses << std::endl;
        std::cout << "Sharpe Ratio: " << result.sharpe_ratio << std::endl;
        return 0;
    }

    std::cout << "=== MoneyBot HFT Trading System ===" << std::endl;
    std::cout << "Strategy: " << config["strategy"]["type"].get<std::string>() << std::endl;
    std::cout << "Symbol: " << config["strategy"]["symbol"].get<std::string>() << std::endl;
    std::cout << "Exchange: " << config["exchange"]["rest_api"]["base_url"].get<std::string>() << std::endl;
    std::cout << "Press Ctrl+C to stop" << std::endl;

    try {
        moneybot::TradingEngine engine(config);
        engine.start();
        
        // --- Enhanced Spinner and Order Book Spread Visual ---
        const char* spinner[] = {"⠋", "⠙", "⠹", "⠸", "⠼", "⠴", "⠦", "⠧", "⠇", "⠏", "⠋", "⠙", "⠚", "⠞", "⠖", "⠦", "⠴", "⠲", "⠳", "⠓"};
        int spinner_idx = 0;
        std::string last_status_line;
        // Main loop
        while (running) {
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            // Get live metrics
            auto metrics = engine.getPerformanceMetrics();
            auto status = engine.getStatus();
            int64_t uptime = status["uptime_seconds"].get<int64_t>();
            double pnl = metrics["total_pnl"].get<double>();
            int trades = metrics["total_trades"].get<int>();
            double avg_pnl = metrics["avg_pnl_per_trade"].get<double>();
            // --- Order book spread visual ---
            double bid = engine.getBestBid();
            double ask = engine.getBestAsk();
            double spread = ask > 0 && bid > 0 ? ask - bid : 0.0;
            // Visual: |====|    |====|, gap proportional to spread (max 20 spaces)
            int gap = (spread > 0 && bid > 0) ? std::min(20, static_cast<int>(spread / (bid * 0.0001))) : 5;
            std::string spread_bar = "|" + std::string(5, '=') + "|" + std::string(gap, ' ') + "|" + std::string(5, '=') + "|";
            // --- Connection status ---
            std::string ws_status = engine.isWsConnected() ? "🟢" : "🔴";
            // --- Last event ---
            std::string last_event = engine.getLastEvent();
            // --- Compose status line ---
            std::ostringstream oss;
            oss << "[" << spinner[spinner_idx] << "] " << ws_status << " "
                << "Trades: " << trades
                << " | PnL: $" << std::fixed << std::setprecision(2) << pnl
                << " | Avg/trade: $" << std::fixed << std::setprecision(2) << avg_pnl
                << " | Uptime: " << uptime << "s "
                << "| Last: " << last_event
                << " | Bid: " << bid << " Ask: " << ask << " Spread: " << spread
                << "\n    " << spread_bar << "   ";
            std::string status_line = oss.str();
            // Overwrite previous line
            std::cout << "\r" << status_line << std::flush;
            last_status_line = status_line;
            spinner_idx = (spinner_idx + 1) % 20;
            // Print status every 30 seconds
            static int counter = 0;
            if (++counter % 150 == 0) {
                std::cout << "\n=== Status Update ===" << std::endl;
                std::cout << "Running: " << (status["running"].get<bool>() ? "Yes" : "No") << std::endl;
                std::cout << "Uptime: " << uptime << " seconds" << std::endl;
                std::cout << "Total PnL: $" << std::fixed << std::setprecision(2) << pnl << std::endl;
                std::cout << "Total Trades: " << trades << std::endl;
                if (status.contains("risk")) {
                    auto risk = status["risk"];
                    std::cout << "Emergency Stop: " << (risk["emergency_stopped"].get<bool>() ? "Yes" : "No") << std::endl;
                    std::cout << "Drawdown: " << std::fixed << std::setprecision(2) << risk["drawdown"].get<double>() << "%" << std::endl;
                }
            }
        }
        // Clear spinner line before shutdown
        std::cout << "\r" << std::string(last_status_line.size(), ' ') << "\r";
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