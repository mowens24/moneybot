#include "moneybot.h"
#include "logger.h"
#include "data_analyzer.h"
#include "backtest_engine.h"
#include "strategy_factory.h"
#include "config_manager.h"
#include "market_data_simulator.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <signal.h>
#include <atomic>
#include <iomanip>

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
              << "  --multi-asset Run multi-asset trading mode\n"
              << std::endl;
}

int main(int argc, char* argv[]) {
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    
    std::string config_file = "config.json";
    bool analyze_mode = false;
    bool dry_run = false;
    bool backtest_mode = false;
    bool multi_asset_mode = false;
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
        } else if (arg == "--multi-asset") {
            multi_asset_mode = true;
        } else {
            std::cerr << "Unknown option: " << arg << std::endl;
            printUsage(argv[0]);
            return 1;
        }
    }

    // Initialize configuration manager
    auto& config_manager = moneybot::ConfigManager::getInstance();
    if (!config_manager.loadConfig(config_file)) {
        std::cerr << "Error: Failed to load configuration from: " << config_file << std::endl;
        return 1;
    }

    json config = config_manager.getConfig();

    // Validate API keys if not in dry-run mode
    if (!config_manager.isDryRunMode() && !config_manager.validateApiKeys()) {
        std::cerr << "Error: Invalid or missing API keys. Check environment variables." << std::endl;
        std::cerr << "Run './setup_env.sh' to set up API keys, then 'source load_env.sh'" << std::endl;
        return 1;
    }

    // Set dry-run mode if requested via command line
    if (dry_run) {
        config["dry_run"] = true;
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
    
    if (multi_asset_mode || config["strategy"]["type"].get<std::string>() == "multi_asset") {
        std::cout << "🚀 MULTI-ASSET TRADING MODE" << std::endl;
        std::cout << "Strategy: Multi-Asset Goldman Sachs Level" << std::endl;
        std::cout << "Exchanges: ";
        if (config.contains("multi_asset") && config["multi_asset"].contains("exchanges")) {
            for (const auto& exchange : config["multi_asset"]["exchanges"]) {
                if (exchange["enabled"].get<bool>()) {
                    std::cout << exchange["name"].get<std::string>() << " ";
                }
            }
        }
        std::cout << std::endl;
        std::cout << "Strategies: ";
        if (config.contains("strategies")) {
            for (const auto& [strategy_name, strategy_config] : config["strategies"].items()) {
                if (strategy_config["enabled"].get<bool>()) {
                    std::cout << strategy_name << " ";
                }
            }
        }
        std::cout << std::endl;
    } else {
        std::cout << "Strategy: " << config["strategy"]["type"].get<std::string>() << std::endl;
        std::cout << "Symbol: " << config["strategy"]["symbol"].get<std::string>() << std::endl;
        std::cout << "Exchange: " << config["exchange"]["rest_api"]["base_url"].get<std::string>() << std::endl;
    }
    
    std::cout << "Press Ctrl+C to stop" << std::endl;

    try {
        // Initialize market data simulator for demo/testing
        std::unique_ptr<moneybot::MarketDataSimulator> simulator;
        if (config_manager.isDryRunMode()) {
            std::cout << "🎯 Initializing Market Data Simulator (Demo Mode)" << std::endl;
            simulator = std::make_unique<moneybot::MarketDataSimulator>();
            
            // Configure simulation parameters
            simulator->setVolatility(0.02); // 2% daily volatility
            simulator->setTrendDirection(0.1); // Slight upward trend
            simulator->setUpdateInterval(std::chrono::milliseconds(100)); // 10 updates per second
            
            // Set up callbacks for live data display
            simulator->setTickCallback([](const moneybot::MarketDataTick& tick) {
                // Optional: Log every N-th tick to avoid spam
                static int tick_counter = 0;
                if (++tick_counter % 50 == 0) { // Log every 5 seconds at 100ms intervals
                    std::cout << "📊 " << tick.exchange << " " << tick.symbol 
                              << " | Bid: $" << std::fixed << std::setprecision(2) << tick.bid_price
                              << " | Ask: $" << tick.ask_price 
                              << " | Spread: " << std::setprecision(4) << ((tick.ask_price - tick.bid_price) / tick.bid_price * 10000) << " bps"
                              << std::endl;
                }
            });
            
            simulator->start();
            
            // Simulate some market events
            std::thread([&simulator]() {
                std::this_thread::sleep_for(std::chrono::seconds(30));
                if (simulator && simulator->isRunning()) {
                    simulator->simulateNewsEvent(2.5, std::chrono::seconds(60)); // 2.5% positive news
                }
                
                std::this_thread::sleep_for(std::chrono::seconds(120));
                if (simulator && simulator->isRunning()) {
                    simulator->simulateNewsEvent(-1.8, std::chrono::seconds(45)); // -1.8% negative news
                }
            }).detach();
        }

        moneybot::TradingEngine engine(config);
        engine.start();
        
        // Main loop - periodic status updates only
        while (running) {
            std::this_thread::sleep_for(std::chrono::seconds(10));
            
            // Get live metrics
            auto metrics = engine.getPerformanceMetrics();
            auto status = engine.getStatus();
            int64_t uptime = status["uptime_seconds"].get<int64_t>();
            double pnl = metrics["total_pnl"].get<double>();
            int trades = metrics["total_trades"].get<int>();
            double avg_pnl = metrics["avg_pnl_per_trade"].get<double>();
            
            // Connection status
            std::string ws_status = engine.isWsConnected() ? "🟢 Connected" : "🔴 Disconnected";
            
            // Print status every 10 seconds
            std::cout << "=== Status Update ===" << std::endl;
            std::cout << "Connection: " << ws_status << std::endl;
            std::cout << "Running: " << (status["running"].get<bool>() ? "Yes" : "No") << std::endl;
            std::cout << "Uptime: " << uptime << " seconds" << std::endl;
            std::cout << "Total PnL: $" << std::fixed << std::setprecision(2) << pnl << std::endl;
            std::cout << "Total Trades: " << trades << std::endl;
            std::cout << "Avg PnL per Trade: $" << std::fixed << std::setprecision(2) << avg_pnl << std::endl;
            
            // Order book info
            double bid = engine.getBestBid();
            double ask = engine.getBestAsk();
            double spread = ask > 0 && bid > 0 ? ask - bid : 0.0;
            std::cout << "Best Bid: " << bid << " | Best Ask: " << ask << " | Spread: " << spread << std::endl;
            
            if (status.contains("risk")) {
                auto risk = status["risk"];
                std::cout << "Emergency Stop: " << (risk["emergency_stopped"].get<bool>() ? "Yes" : "No") << std::endl;
                std::cout << "Drawdown: " << std::fixed << std::setprecision(2) << risk["drawdown"].get<double>() << "%" << std::endl;
            }
            
            // Show live market data if simulator is running
            if (simulator && simulator->isRunning()) {
                auto btc_tick = simulator->getLatestTick("BTCUSDT", "binance");
                if (!btc_tick.symbol.empty()) {
                    std::cout << "📈 Live Market: " << btc_tick.symbol 
                              << " $" << std::fixed << std::setprecision(2) << btc_tick.last_price
                              << " (24h Vol: $" << std::setprecision(0) << btc_tick.volume_24h << ")" << std::endl;
                }
            }
            
            std::cout << "---" << std::endl;
        }
        std::cout << "\nShutting down..." << std::endl;
        
        // Stop simulator first
        if (simulator) {
            simulator->stop();
        }
        
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