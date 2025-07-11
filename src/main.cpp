#include "logger.h"
#include "modern_logger.h"
#include "application_state.h"
#include "event_manager.h"
#include "data_analyzer.h"
#include "backtest_engine.h"
#include "strategy_factory.h"
#include "config_manager.h"
#include "market_data_simulator.h"
#include "core/strategy_controller.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <signal.h>
#include <atomic>
#include <iomanip>

using json = nlohmann::json;

// Forward declare GUI main function
int gui_main(int argc, char** argv);
int gui_main_new(int argc, char** argv);

std::atomic<bool> running(true);

void signalHandler(int signum) {
    LOG_INFO("Received signal. Initiating graceful shutdown...");
    
    auto& state_manager = moneybot::ApplicationStateManager::getInstance();
    state_manager.requestShutdown();
    state_manager.setState(moneybot::AppState::DISCONNECTING);
    
    auto& event_manager = moneybot::EventManager::getInstance();
    event_manager.stop();
    
    running = false;
}

void printUsage(const char* program) {
    std::cout << "Usage: " << program << " [OPTIONS]\n"
              << "Options:\n"
              << "  --console, -c Launch in console mode (GUI is default)\n"
              << "  --simulator, -s Use market data simulator (live data is default)\n"
              << "  --analyze     Run in analysis mode (no trading)\n"
              << "  --config FILE Use custom config file (default: config.json)\n"
              << "  --help        Show this help message\n"
              << "  --dry-run     Run without placing real orders (implies --simulator)\n"
              << "  --backtest    Run in backtest mode with historical data\n"
              << "  --multi-asset Run multi-asset trading mode\n"
              << std::endl;
}

int main(int argc, char* argv[]) {
    // Initialize application state
    auto& state_manager = moneybot::ApplicationStateManager::getInstance();
    state_manager.setState(moneybot::AppState::INITIALIZING);
    
    // Initialize logging first
    auto& logger = moneybot::ModernLogger::getInstance();
    logger.initialize("info", "logs/moneybot.log");
    
    LOG_INFO("MoneyBot starting up...");
    
    // Initialize event system
    auto& event_manager = moneybot::EventManager::getInstance();
    event_manager.start();
    
    // Set up signal handlers
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    
    std::string config_file = "config.json";
    bool analyze_mode = false;
    bool dry_run = false;
    bool backtest_mode = false;
    bool multi_asset_mode = false;
    bool console_mode = false;  // Changed: GUI is now default
    bool use_simulator = false; // Changed: Live data is now default
    std::string backtest_data = "data/ticks.db";

    // Parse command line arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--help" || arg == "-h") {
            printUsage(argv[0]);
            return 0;
        } else if (arg == "--console" || arg == "-c") {
            console_mode = true;
        } else if (arg == "--simulator" || arg == "-s") {
            use_simulator = true;
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
            use_simulator = true; // Dry run implies simulator
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

    // Default to GUI mode unless console mode is explicitly requested
    if (!console_mode && !analyze_mode && !backtest_mode) {
        LOG_INFO("Starting GUI mode");
        state_manager.setTradingMode(use_simulator ? moneybot::TradingMode::DEMO : 
                                    (dry_run ? moneybot::TradingMode::PAPER : moneybot::TradingMode::LIVE));
        return gui_main_new(argc, argv);
    }

    // Initialize configuration manager
    auto& config_manager = moneybot::ConfigManager::getInstance();
    if (!config_manager.loadConfig(config_file)) {
        LOG_ERROR("Failed to load configuration from: " + config_file);
        state_manager.setError("Configuration load failed");
        state_manager.setState(moneybot::AppState::ERROR);
        return 1;
    }

    json config = config_manager.getConfig();

    // Validate API keys if not in dry-run mode
    if (!config_manager.isDryRunMode() && !config_manager.validateApiKeys()) {
        LOG_ERROR("Invalid or missing API keys. Check environment variables.");
        LOG_INFO("Run './setup_env.sh' to set up API keys, then 'source load_env.sh'");
        state_manager.setError("API keys validation failed");
        state_manager.setState(moneybot::AppState::ERROR);
        return 1;
    }

    // Set trading mode
    if (dry_run) {
        state_manager.setTradingMode(moneybot::TradingMode::PAPER);
        LOG_INFO("Running in PAPER TRADING mode (no real orders will be placed)");
    } else if (use_simulator) {
        state_manager.setTradingMode(moneybot::TradingMode::DEMO);
        LOG_INFO("Running in DEMO mode with simulated data");
    } else {
        state_manager.setTradingMode(moneybot::TradingMode::LIVE);
        LOG_WARN("Running in LIVE TRADING mode - REAL MONEY AT RISK!");
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
    
    // Initialize StrategyController
    auto strategy_controller = std::make_unique<moneybot::StrategyController>();
    
    if (multi_asset_mode || (config.contains("strategy") && config["strategy"].contains("type") && config["strategy"]["type"].get<std::string>() == "multi_asset")) {
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
    
    // Initialize and start strategy controller
    std::cout << "🧠 Initializing Strategy Controller..." << std::endl;
    if (!strategy_controller->initialize(config)) {
        std::cerr << "❌ Failed to initialize StrategyController" << std::endl;
        return 1;
    }
    
    strategy_controller->start();
    std::cout << "✅ Strategy Controller started successfully" << std::endl;
    
    std::cout << "Press Ctrl+C to stop" << std::endl;

    try {
        // Initialize market data simulator only if requested or in dry-run mode
        std::unique_ptr<moneybot::MarketDataSimulator> simulator;
        if (use_simulator || config_manager.isDryRunMode()) {
            std::cout << "🎯 Initializing Market Data Simulator" << std::endl;
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
        } else {
            std::cout << "🌐 Connecting to live market data feeds..." << std::endl;
            // TODO: Initialize real exchange connections here
            std::cout << "⚠️  Live data not yet implemented. Use --simulator for demo mode." << std::endl;
        }

        // Main loop - periodic status updates only
        while (running) {
            std::this_thread::sleep_for(std::chrono::seconds(10));
            
            // Get performance metrics from StrategyController
            auto metrics = strategy_controller->getPerformanceMetrics();
            
            // Print status every 10 seconds
            std::cout << "=== Status Update ===" << std::endl;
            std::cout << "Running: " << (strategy_controller->isRunning() ? "Yes" : "No") << std::endl;
            std::cout << "Emergency Stop: " << (strategy_controller->isEmergencyStopped() ? "Yes" : "No") << std::endl;
            std::cout << "Total Portfolio Value: $" << std::fixed << std::setprecision(2) << metrics.total_portfolio_value << std::endl;
            std::cout << "Total PnL: $" << std::fixed << std::setprecision(2) << metrics.total_pnl << std::endl;
            std::cout << "Daily PnL: $" << std::fixed << std::setprecision(2) << metrics.daily_pnl << std::endl;
            std::cout << "Total Trades: " << metrics.total_trades << std::endl;
            std::cout << "Overall Win Rate: " << std::fixed << std::setprecision(1) << metrics.overall_win_rate << "%" << std::endl;
            std::cout << "Overall Sharpe Ratio: " << std::fixed << std::setprecision(2) << metrics.overall_sharpe_ratio << std::endl;
            std::cout << "Max Drawdown: " << std::fixed << std::setprecision(2) << metrics.max_drawdown << "%" << std::endl;
            std::cout << "VaR (95%): $" << std::fixed << std::setprecision(2) << metrics.var_95 << std::endl;
            
            // Strategy details
            if (!metrics.strategy_statuses.empty()) {
                std::cout << "Active Strategies:" << std::endl;
                for (const auto& status : metrics.strategy_statuses) {
                    std::cout << "  - " << status.name << " (" << status.type << "): ";
                    if (status.is_active) {
                        std::cout << "🟢 ACTIVE | ";
                    } else if (status.is_enabled) {
                        std::cout << "🟡 ENABLED | ";
                    } else {
                        std::cout << "🔴 DISABLED | ";
                    }
                    std::cout << "PnL: $" << std::fixed << std::setprecision(2) << status.total_pnl;
                    std::cout << " | Trades: " << status.total_trades;
                    std::cout << " | Win Rate: " << std::fixed << std::setprecision(1) << status.win_rate << "%";
                    if (!status.last_error.empty()) {
                        std::cout << " | Error: " << status.last_error;
                    }
                    std::cout << std::endl;
                }
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
        
        // Stop strategy controller
        strategy_controller->stop();
        
        // Stop simulator
        if (simulator) {
            simulator->stop();
        }
        
        // Final status report
        auto final_metrics = strategy_controller->getPerformanceMetrics();
        std::cout << "\n=== Final Report ===" << std::endl;
        std::cout << "Total Portfolio Value: $" << std::fixed << std::setprecision(2) << final_metrics.total_portfolio_value << std::endl;
        std::cout << "Total PnL: $" << std::fixed << std::setprecision(2) << final_metrics.total_pnl << std::endl;
        std::cout << "Daily PnL: $" << std::fixed << std::setprecision(2) << final_metrics.daily_pnl << std::endl;
        std::cout << "Total Trades: " << final_metrics.total_trades << std::endl;
        std::cout << "Overall Win Rate: " << std::fixed << std::setprecision(1) << final_metrics.overall_win_rate << "%" << std::endl;
        std::cout << "Overall Sharpe Ratio: " << std::fixed << std::setprecision(2) << final_metrics.overall_sharpe_ratio << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}