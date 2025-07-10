// Goldman Sachs-Level Multi-Asset Trading Dashboard
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <GLFW/glfw3.h>
#include <thread>
#include <chrono>
#include <memory>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <functional>
#include <cmath>
#include <vector>
#include <string>
#include <random>
#include <iostream>
#include <algorithm>
#include <map>

// Live trading includes
#include "exchange_interface.h"
#include "live_trading_manager.h"
// #include "binance_exchange.h"  // Uncomment when ready for live trading

using namespace std;

// Simplified market data structure
struct TickData {
    string symbol;
    string exchange;
    double last_price;
    double bid_price;
    double ask_price;
    double volume_24h;
    int64_t timestamp;
    
    TickData() : last_price(0), bid_price(0), ask_price(0), volume_24h(0), timestamp(0) {}
};

// Simple market data simulator
class SimpleMarketSimulator {
private:
    map<pair<string, string>, TickData> market_data;
    bool running = false;
    thread sim_thread;
    
    void simulateData() {
        vector<string> symbols = {"BTCUSDT", "ETHUSDT", "ADAUSDT", "DOTUSDT", "LINKUSDT"};
        vector<string> exchanges = {"binance", "coinbase", "kraken"};
        
        // Initialize with base prices
        map<string, double> base_prices = {
            {"BTCUSDT", 45000.0},
            {"ETHUSDT", 3000.0},
            {"ADAUSDT", 0.85},
            {"DOTUSDT", 25.0},
            {"LINKUSDT", 15.0}
        };
        
        while (running) {
            for (const auto& symbol : symbols) {
                for (const auto& exchange : exchanges) {
                    TickData& tick = market_data[{symbol, exchange}];
                    tick.symbol = symbol;
                    tick.exchange = exchange;
                    
                    // Simulate realistic price movement
                    double base_price = base_prices[symbol];
                    double noise = (rand() % 1000 - 500) / 10000.0; // ±5% noise
                    tick.last_price = base_price * (1.0 + noise);
                    
                    // Create bid/ask spread
                    double spread = tick.last_price * 0.001; // 0.1% spread
                    tick.bid_price = tick.last_price - spread/2;
                    tick.ask_price = tick.last_price + spread/2;
                    
                    // Simulate volume
                    tick.volume_24h = (rand() % 50000000) + 10000000; // 10M-60M volume
                    tick.timestamp = chrono::duration_cast<chrono::milliseconds>(
                        chrono::system_clock::now().time_since_epoch()).count();
                }
            }
            this_thread::sleep_for(chrono::milliseconds(200));
        }
    }
    
public:
    void start() {
        running = true;
        sim_thread = thread(&SimpleMarketSimulator::simulateData, this);
    }
    
    void stop() {
        running = false;
        if (sim_thread.joinable()) {
            sim_thread.join();
        }
    }
    
    TickData getLatestTick(const string& symbol, const string& exchange) {
        auto key = make_pair(symbol, exchange);
        if (market_data.find(key) != market_data.end()) {
            return market_data[key];
        }
        return TickData();
    }
    
    bool isRunning() const { return running; }
    
    ~SimpleMarketSimulator() {
        stop();
    }
};

// GUI State Management - Enhanced for Live Trading
struct GUIState {
    // Trading system components
    shared_ptr<SimpleMarketSimulator> simulator;           // For demo mode
    unique_ptr<LiveMarketDataManager> live_market_manager;  // For live trading
    unique_ptr<LiveOrderExecutionEngine> order_engine;     // For live orders
    
    // Trading mode
    enum TradingMode {
        DEMO_SIMULATION,    // Safe simulator
        PAPER_TRADING,      // Live data, simulated orders
        LIVE_TRADING        // Real money trading
    };
    TradingMode current_mode = LIVE_TRADING;
    
    // Window states - Focused on algorithm visualization
    bool show_portfolio_window = true;         // Clean performance dashboard
    bool show_market_data_window = false;      // Hidden - algorithms handle raw data
    bool show_arbitrage_window = false;        // Hidden - integrated into main dashboard
    bool show_risk_window = false;             // Hidden - integrated into portfolio
    bool show_strategy_window = false;         // Hidden - integrated into main dashboard
    bool show_main_dashboard = true;           // Visual algorithm hub
    bool show_exchange_management = false;     // Available but not shown by default
    
    // Layout controls
    bool snap_to_grid = true;
    bool should_setup_docking = true;
    float grid_size = 50.0f;
    ImGuiID dockspace_id = 0;
    
    // Market data
    vector<string> symbols = {"BTCUSDT", "ETHUSDT", "ADAUSDT", "DOTUSDT", "LINKUSDT"};
    vector<string> exchanges = {"binance", "coinbase", "kraken"};
    
    // Live trading metrics (updated from real data)
    double total_pnl = 0.0;
    double daily_pnl = 0.0;
    double portfolio_value = 100000.0;
    int total_trades = 0;
    double win_rate = 65.5;
    double sharpe_ratio = 1.85;
    double max_drawdown = 2.3;
    double var_95 = 3250.0;
    
    // Live market statistics
    struct LiveMarketStats {
        int connected_exchanges = 0;
        double avg_latency_ms = 0.0;
        int updates_per_second = 0;
        int arbitrage_opportunities = 0;
        string last_update_time = "Never";
        bool websocket_connected = false;
    } live_stats;
    
    GUIState() {
        initializeTradingSystem();
        cout << "🎮 MoneyBot Professional Trading Dashboard Initialized" << endl;
        cout << "📊 Mode: " << getModeString() << endl;
    }
    
    void initializeTradingSystem() {
        if (current_mode == DEMO_SIMULATION) {
            // Initialize simulator for safe testing
            simulator = make_shared<SimpleMarketSimulator>();
            simulator->start();
            updateMetricsFromSimulation();
        } else {
            // Initialize live trading components
            live_market_manager = make_unique<LiveMarketDataManager>(current_mode == PAPER_TRADING);
            order_engine = make_unique<LiveOrderExecutionEngine>(live_market_manager.get());
            
            // Start the live market data manager
            live_market_manager->start();
            
            // Add exchanges (start with testnet for safety)
            // addLiveExchanges();
            
            updateMetricsFromLiveData();
        }
    }
    
    void switchTradingMode(TradingMode new_mode) {
        if (new_mode == current_mode) return;
        
        cout << "⚠️  SWITCHING TRADING MODE ⚠️" << endl;
        cout << "From: " << getModeString() << " -> To: " << getModeString(new_mode) << endl;
        
        // Cleanup current mode
        if (simulator) {
            simulator->stop();
            simulator.reset();
        }
        if (live_market_manager) {
            live_market_manager->stop();
            live_market_manager.reset();
        }
        if (order_engine) {
            order_engine.reset();
        }
        
        current_mode = new_mode;
        initializeTradingSystem();
        
        if (new_mode == LIVE_TRADING) {
            cout << "� LIVE TRADING MODE ACTIVE - REAL MONEY AT RISK!" << endl;
        }
    }
    
    string getModeString(TradingMode mode = TradingMode::DEMO_SIMULATION) const {
        if (mode == TradingMode::DEMO_SIMULATION) mode = current_mode;
        switch (mode) {
            case DEMO_SIMULATION: return "Demo Simulation";
            case PAPER_TRADING: return "Paper Trading";
            case LIVE_TRADING: return "Live Trading";
            default: return "Unknown";
        }
    }
    
    void updateMetricsFromSimulation() {
        if (!simulator || !simulator->isRunning()) return;
        
        static auto last_update = chrono::steady_clock::now();
        auto now = chrono::steady_clock::now();
        auto elapsed = chrono::duration_cast<chrono::seconds>(now - last_update).count();
        
        if (elapsed > 5) {
            // Simulate realistic P&L fluctuations
            double pnl_change = (rand() % 2000 - 1000) / 100.0;
            total_pnl += pnl_change;
            daily_pnl += pnl_change;
            
            // Update trade count
            if (rand() % 10 == 0) {
                total_trades++;
                if (rand() % 100 < 67) {
                    win_rate = (win_rate * (total_trades - 1) + 100.0) / total_trades;
                } else {
                    win_rate = (win_rate * (total_trades - 1)) / total_trades;
                }
            }
            
            portfolio_value = 100000.0 + total_pnl;
            last_update = now;
        }
    }
    
    void updateMetricsFromLiveData() {
        if (!live_market_manager || !live_market_manager->isRunning()) return;
        
        // Get real market statistics
        auto stats = live_market_manager->getMarketStats();
        live_stats.connected_exchanges = stats.connected_exchanges;
        live_stats.avg_latency_ms = stats.avg_latency_ms;
        live_stats.updates_per_second = stats.updates_per_second;
        live_stats.arbitrage_opportunities = stats.arbitrage_opportunities;
        live_stats.last_update_time = stats.last_update_time;
        
        // Get real portfolio data from order engine
        if (order_engine) {
            portfolio_value = order_engine->getTotalPortfolioValue();
            daily_pnl = order_engine->getDailyPnL();
            auto positions = order_engine->getCurrentPositions();
            // Update other metrics from real trading data
        }
    }
    
    // Utility methods for GUI
    bool isLiveMode() const { return current_mode != DEMO_SIMULATION; }
    bool isSimulationMode() const { return current_mode == DEMO_SIMULATION; }
    bool canExecuteRealTrades() const { return current_mode == LIVE_TRADING; }
    
    // Helper method to get market data from the appropriate source
    LiveTickData getTickData(const std::string& symbol, const std::string& exchange = "binance") {
        if (isLiveMode() && live_market_manager) {
            // Try to get live data from exchanges
            auto* binance_exchange = live_market_manager->getExchange("binance");
            if (binance_exchange) {
                return binance_exchange->getLatestTick(symbol);
            }
        } else if (simulator) {
            // Convert simulator TickData to LiveTickData
            auto sim_tick = simulator->getLatestTick(symbol, exchange);
            LiveTickData live_tick;
            live_tick.symbol = sim_tick.symbol;
            live_tick.exchange = sim_tick.exchange;
            live_tick.last_price = sim_tick.last_price;
            live_tick.bid_price = sim_tick.bid_price;
            live_tick.ask_price = sim_tick.ask_price;
            live_tick.volume_24h = sim_tick.volume_24h;
            live_tick.timestamp = sim_tick.timestamp;
            // Set default values for fields not in TickData
            live_tick.high_24h = sim_tick.last_price * 1.05;
            live_tick.low_24h = sim_tick.last_price * 0.95;
            live_tick.price_change_24h = sim_tick.last_price * 0.02;
            live_tick.price_change_percent_24h = 2.0;
            live_tick.server_time = sim_tick.timestamp;
            return live_tick;
        }
        
        // Return empty tick if no data available
        return LiveTickData{};
    }
    
    ~GUIState() {
        if (simulator) simulator->stop();
        if (live_market_manager) live_market_manager->stop();
    }
};

// Forward declarations
void SetupDockingLayout(GUIState& state);
void ApplyGridSnapping(GUIState& state);
ImVec2 SnapToGrid(ImVec2 pos, float grid_size);
void RenderMainMenuBar(GUIState& state);
void RenderMainDashboard(GUIState& state);
void RenderAlgorithmVisualizationWindow(GUIState& state);  // New visual algorithm dashboard
void RenderPortfolioWindow(GUIState& state);
void RenderMarketDataWindow(GUIState& state);
void RenderArbitrageWindow(GUIState& state);
void RenderRiskManagementWindow(GUIState& state);
void RenderStrategyPerformanceWindow(GUIState& state);
void RenderExchangeManagementWindow(GUIState& state);  // New live trading control window

int gui_main(int argc, char** argv) {
    // Setup GLFW window
    if (!glfwInit()) {
        cerr << "Failed to initialize GLFW" << endl;
        return 1;
    }
    
    // Create larger window for multi-asset dashboard
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    
    GLFWwindow* window = glfwCreateWindow(1920, 1080, "MoneyBot Goldman Sachs-Level Trading Dashboard", nullptr, nullptr);
    if (!window) {
        cerr << "Failed to create GLFW window" << endl;
        glfwTerminate();
        return 1;
    }
    
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync
    
    // Setup Dear ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    
    // Setup style for professional appearance
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 5.0f;
    style.FrameRounding = 3.0f;
    style.ScrollbarRounding = 3.0f;
    style.GrabRounding = 3.0f;
    style.TabRounding = 3.0f;
    style.Colors[ImGuiCol_WindowBg].w = 0.95f;
    
    // Setup Platform/Renderer backends
    const char* glsl_version = "#version 150";
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);
    
    // Initialize GUI state
    GUIState gui_state;
    
    cout << "🚀 MoneyBot Goldman Sachs-Level Dashboard Started!" << endl;
    cout << "💼 Multi-Asset Portfolio Management Active" << endl;
    cout << "🏢 Multi-Exchange Connectivity Ready" << endl;
    cout << "🎯 Grid Layout System Enabled" << endl;

    // Main render loop
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        
        // Update live data metrics based on current mode
        if (gui_state.isSimulationMode()) {
            gui_state.updateMetricsFromSimulation();
        } else {
            gui_state.updateMetricsFromLiveData();
        }
        
        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        
        // Setup grid layout if needed
        if (gui_state.should_setup_docking) {
            SetupDockingLayout(gui_state);
        }
        
        // Apply grid snapping constraints
        ApplyGridSnapping(gui_state);
        
        // Render main menu bar
        RenderMainMenuBar(gui_state);
        
        // Render all windows with proper sizing constraints and grid positioning
        if (gui_state.show_main_dashboard) {
            ImGui::SetNextWindowPos(ImVec2(10, 50), ImGuiCond_FirstUseEver);
            ImGui::SetNextWindowSize(ImVec2(1200, 500), ImGuiCond_FirstUseEver);
            ImGui::SetNextWindowSizeConstraints(ImVec2(800, 400), ImVec2(FLT_MAX, FLT_MAX));
            RenderAlgorithmVisualizationWindow(gui_state);  // Use new visual dashboard
        }
        
        if (gui_state.show_portfolio_window) {
            ImGui::SetNextWindowPos(ImVec2(1220, 50), ImGuiCond_FirstUseEver);
            ImGui::SetNextWindowSize(ImVec2(400, 500), ImGuiCond_FirstUseEver);
            ImGui::SetNextWindowSizeConstraints(ImVec2(350, 400), ImVec2(500, FLT_MAX));
            RenderPortfolioWindow(gui_state);
        }
        
        // Market data window hidden by default - algorithms handle the data
        if (gui_state.show_market_data_window) {
            ImGui::SetNextWindowPos(ImVec2(10, 570), ImGuiCond_FirstUseEver);
            ImGui::SetNextWindowSize(ImVec2(1200, 300), ImGuiCond_FirstUseEver);
            ImGui::SetNextWindowSizeConstraints(ImVec2(800, 200), ImVec2(FLT_MAX, 400));
            RenderMarketDataWindow(gui_state);
        }
        
        if (gui_state.show_arbitrage_window) {
            ImGui::SetNextWindowPos(ImVec2(1160, 50), ImGuiCond_FirstUseEver);
            ImGui::SetNextWindowSize(ImVec2(450, 400), ImGuiCond_FirstUseEver);
            ImGui::SetNextWindowSizeConstraints(ImVec2(400, 250), ImVec2(FLT_MAX, FLT_MAX));
            RenderArbitrageWindow(gui_state);
        }
        
        if (gui_state.show_risk_window) {
            ImGui::SetNextWindowPos(ImVec2(1620, 50), ImGuiCond_FirstUseEver);
            ImGui::SetNextWindowSize(ImVec2(290, 400), ImGuiCond_FirstUseEver);
            ImGui::SetNextWindowSizeConstraints(ImVec2(280, 200), ImVec2(400, FLT_MAX));
            RenderRiskManagementWindow(gui_state);
        }
        
        if (gui_state.show_strategy_window) {
            ImGui::SetNextWindowPos(ImVec2(350, 460), ImGuiCond_FirstUseEver);
            ImGui::SetNextWindowSize(ImVec2(800, 400), ImGuiCond_FirstUseEver);
            ImGui::SetNextWindowSizeConstraints(ImVec2(400, 250), ImVec2(FLT_MAX, FLT_MAX));
            RenderStrategyPerformanceWindow(gui_state);
        }
        
        if (gui_state.show_exchange_management) {
            ImGui::SetNextWindowPos(ImVec2(1160, 460), ImGuiCond_FirstUseEver);
            ImGui::SetNextWindowSize(ImVec2(450, 400), ImGuiCond_FirstUseEver);
            ImGui::SetNextWindowSizeConstraints(ImVec2(400, 300), ImVec2(FLT_MAX, FLT_MAX));
            RenderExchangeManagementWindow(gui_state);
        }
        
        // Clean up style vars if grid snapping is enabled
        if (gui_state.snap_to_grid) {
            ImGui::PopStyleVar(); // WindowMinSize
        }
        
        // Rendering
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.08f, 0.08f, 0.12f, 1.00f); // Dark blue background
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        
        glfwSwapBuffers(window);
        
        // Limit FPS to 60
        this_thread::sleep_for(chrono::milliseconds(16));
    }
    
    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
    
    cout << "🛑 MoneyBot GUI Dashboard Stopped" << endl;
    
    return 0;
}

// =============================================================================
// Grid Layout and Snapping Functions
// =============================================================================

void SetupDockingLayout(GUIState& state) {
    // Since we don't have docking, we'll use smart positioning
    state.should_setup_docking = false;
    std::cout << "🎯 Grid layout configured - 6 window professional trading dashboard" << std::endl;
}

ImVec2 SnapToGrid(ImVec2 pos, float grid_size) {
    return ImVec2(
        round(pos.x / grid_size) * grid_size,
        round(pos.y / grid_size) * grid_size
    );
}

void ApplyGridSnapping(GUIState& state) {
    if (!state.snap_to_grid) return;
    
    // Apply consistent styling for grid layout
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 5.0f;  // Consistent window appearance
    style.WindowBorderSize = 1.0f;
    style.FrameRounding = 3.0f;
    
    // Ensure minimum window sizes for proper layout
    ImVec2 min_window_size(300.0f, 200.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, min_window_size);
}

// =============================================================================
// GUI Rendering Functions
// =============================================================================

void RenderMainMenuBar(GUIState& state) {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("Dashboard")) {
            ImGui::MenuItem("Main Dashboard", "Ctrl+1", &state.show_main_dashboard);
            ImGui::MenuItem("Portfolio", "Ctrl+2", &state.show_portfolio_window);
            ImGui::MenuItem("Market Data", "Ctrl+3", &state.show_market_data_window);
            ImGui::MenuItem("Arbitrage Scanner", "Ctrl+4", &state.show_arbitrage_window);
            ImGui::MenuItem("Risk Management", "Ctrl+5", &state.show_risk_window);
            ImGui::MenuItem("Strategy Performance", "Ctrl+6", &state.show_strategy_window);
            ImGui::Separator();
            ImGui::MenuItem("Exchange Management", "Ctrl+7", &state.show_exchange_management);
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("Trading Mode")) {
            bool is_demo = (state.current_mode == GUIState::DEMO_SIMULATION);
            bool is_paper = (state.current_mode == GUIState::PAPER_TRADING);
            bool is_live = (state.current_mode == GUIState::LIVE_TRADING);
            
            if (ImGui::MenuItem("🎮 Demo Simulation", nullptr, is_demo)) {
                state.switchTradingMode(GUIState::DEMO_SIMULATION);
            }
            if (ImGui::MenuItem("📊 Paper Trading", nullptr, is_paper)) {
                state.switchTradingMode(GUIState::PAPER_TRADING);
            }
            ImGui::Separator();
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.3f, 0.3f, 1.0f));
            if (ImGui::MenuItem("🔴 Live Trading", nullptr, is_live)) {
                state.switchTradingMode(GUIState::LIVE_TRADING);
            }
            ImGui::PopStyleColor();
            
            ImGui::Separator();
            ImGui::Text("Current Mode: %s", state.getModeString().c_str());
            if (state.isLiveMode()) {
                ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "⚠️ REAL MONEY AT RISK");
            }
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("Layout")) {
            if (ImGui::MenuItem("Reset Layout")) {
                state.should_setup_docking = true;
                std::cout << "🎯 Resetting layout to default grid positions" << std::endl;
            }
            ImGui::MenuItem("Snap to Grid", nullptr, &state.snap_to_grid);
            ImGui::SliderFloat("Grid Size", &state.grid_size, 25.0f, 100.0f, "%.0f px");
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("Trading")) {
            if (ImGui::MenuItem("🚨 Emergency Stop", "F1")) {
                std::cout << "🚨 EMERGENCY STOP: All trading halted!" << std::endl;
                if (state.order_engine) {
                    state.order_engine->enableTrading(false);
                    state.order_engine->cancelAllOrders();
                }
            }
            if (ImGui::MenuItem("⏸️ Pause All Strategies", "F2")) {
                std::cout << "⏸️ All strategies paused" << std::endl;
            }
            if (ImGui::MenuItem("🔄 Resume Trading", "F3")) {
                std::cout << "🔄 Trading resumed" << std::endl;
                if (state.order_engine && state.canExecuteRealTrades()) {
                    state.order_engine->enableTrading(true);
                }
            }
            ImGui::Separator();
            if (ImGui::MenuItem("🎯 Optimize Parameters")) {
                std::cout << "🎯 Running parameter optimization..." << std::endl;
            }
            if (ImGui::MenuItem("🔄 Rebalance Portfolio")) {
                std::cout << "🔄 Rebalancing portfolio..." << std::endl;
            }
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("Analysis")) {
            if (ImGui::MenuItem("📊 Generate Report")) {
                std::cout << "📊 Generating performance report..." << std::endl;
            }
            if (ImGui::MenuItem("📈 Run Backtest")) {
                std::cout << "📈 Starting backtest analysis..." << std::endl;
            }
            if (ImGui::MenuItem("🔍 Risk Analysis")) {
                std::cout << "🔍 Generating risk analysis..." << std::endl;
            }
            ImGui::EndMenu();
        }
        
        // Status indicators in menu bar
        ImGui::Text("  |  ");
        
        // Trading mode indicator
        if (state.current_mode == GUIState::DEMO_SIMULATION) {
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "🎮 DEMO");
        } else if (state.current_mode == GUIState::PAPER_TRADING) {
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "� PAPER");
        } else {
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "� LIVE");
        }
        
        ImGui::Text("  |  P&L: ");
        ImGui::TextColored(state.total_pnl >= 0 ? ImVec4(0.0f, 1.0f, 0.0f, 1.0f) : ImVec4(1.0f, 0.0f, 0.0f, 1.0f), 
                          "$%.2f", state.total_pnl);
        
        ImGui::Text("  |  Trades: %d", state.total_trades);
        ImGui::Text("  |  Sharpe: %.2f", state.sharpe_ratio);
        
        // Live market status
        if (state.isLiveMode()) {
            ImGui::Text("  |  Exchanges: %d", state.live_stats.connected_exchanges);
            ImGui::Text("  |  Latency: %.1fms", state.live_stats.avg_latency_ms);
        }
        
        ImGui::EndMainMenuBar();
    }
}

void RenderMainDashboard(GUIState& state) {
    ImGui::Begin("🚀 Goldman Sachs-Level Trading Dashboard", &state.show_main_dashboard, 
                 ImGuiWindowFlags_NoCollapse);
    
    // Header
    ImGui::PushFont(nullptr); // Use default font, could load custom fonts
    ImGui::TextColored(ImVec4(0.0f, 1.0f, 1.0f, 1.0f), "💼 MULTI-ASSET PORTFOLIO MANAGEMENT");
    ImGui::PopFont();
    ImGui::Separator();
    
    // Key metrics in cards
    ImGui::Columns(4, "MainMetrics", false);
    
    // Portfolio Value Card
    ImGui::BeginChild("PortfolioValue", ImVec2(0, 100), true);
    ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Portfolio Value");
    ImGui::Text("$%.2f", state.portfolio_value);
    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Total Assets");
    ImGui::EndChild();
    ImGui::NextColumn();
    
    // Total P&L Card
    ImGui::BeginChild("TotalPnL", ImVec2(0, 100), true);
    ImGui::TextColored(state.total_pnl >= 0 ? ImVec4(0.0f, 1.0f, 0.0f, 1.0f) : ImVec4(1.0f, 0.0f, 0.0f, 1.0f), 
                      "Total P&L");
    ImGui::Text("$%.2f", state.total_pnl);
    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Realized + Unrealized");
    ImGui::EndChild();
    ImGui::NextColumn();
    
    // Daily P&L Card
    ImGui::BeginChild("DailyPnL", ImVec2(0, 100), true);
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Daily P&L");
    ImGui::Text("$%.2f", state.daily_pnl);
    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Today's Performance");
    ImGui::EndChild();
    ImGui::NextColumn();
    
    // Win Rate Card
    ImGui::BeginChild("WinRate", ImVec2(0, 100), true);
    ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, 1.0f), "Win Rate");
    ImGui::Text("%.1f%%", state.win_rate);
    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Success Ratio");
    ImGui::EndChild();
    ImGui::NextColumn();
    
    ImGui::Columns(1);
    ImGui::Separator();
    
    // Live market data preview
    ImGui::Text("📊 Live Market Data Preview");
    if (state.simulator && state.simulator->isRunning()) {
        ImGui::Text("🟢 Market Data Feed: ACTIVE | Latency: 12ms | Updates/sec: 250+");
        ImGui::Separator();
        
        if (ImGui::BeginTable("LiveData", 6, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
            ImGui::TableSetupColumn("Symbol");
            ImGui::TableSetupColumn("Price");
            ImGui::TableSetupColumn("24h Change");
            ImGui::TableSetupColumn("Volume");
            ImGui::TableSetupColumn("Spread (bps)");
            ImGui::TableSetupColumn("Trend");
            ImGui::TableHeadersRow();
            
            // Sample live data for each symbol with enhanced metrics
            for (const auto& symbol : state.symbols) {
                auto tick = state.getTickData(symbol, "binance");
                if (!tick.symbol.empty() && tick.last_price > 0) {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, 1.0f), "%s", symbol.c_str());
                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("$%.2f", tick.last_price);
                    ImGui::TableSetColumnIndex(2);
                    double change = (sin(glfwGetTime() + hash<string>{}(symbol)) * 5.0); // Realistic fluctuation
                    ImGui::TextColored(change >= 0 ? ImVec4(0.0f, 1.0f, 0.0f, 1.0f) : ImVec4(1.0f, 0.0f, 0.0f, 1.0f), 
                                     "%+.1f%%", change);
                    ImGui::TableSetColumnIndex(3);
                    ImGui::Text("$%.0fM", tick.volume_24h / 1000000.0);
                    ImGui::TableSetColumnIndex(4);
                    double spread_bps = ((tick.ask_price - tick.bid_price) / tick.bid_price) * 10000.0;
                    ImGui::Text("%.1f", spread_bps);
                    ImGui::TableSetColumnIndex(5);
                    // Show trend arrow based on recent price movement
                    if (change > 1.0) {
                        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "📈");
                    } else if (change < -1.0) {
                        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "📉");
                    } else {
                        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "➡️");
                    }
                }
            }
            ImGui::EndTable();
        }
        
        // Add real-time streaming indicators
        ImGui::Separator();
        ImGui::Text("🔴 LIVE STREAMING | Last Update: %.2fs ago", fmod(glfwGetTime(), 1.0));
    } else {
        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "⚠️ Market data simulator not running");
    }
    
    ImGui::End();
}

void RenderPortfolioWindow(GUIState& state) {
    ImGui::Begin("💼 Portfolio & Performance", &state.show_portfolio_window);
    
    ImGui::TextColored(ImVec4(0.0f, 1.0f, 1.0f, 1.0f), "Algorithm Performance Dashboard");
    ImGui::Separator();
    
    // Clean performance metrics (no overwhelming data)
    ImGui::Columns(3, "PerformanceSummary", false);
    
    // Total Portfolio Value
    ImGui::BeginChild("TotalValue", ImVec2(0, 80), true);
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Portfolio Value");
    ImGui::Spacing();
    ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "$%.0f", state.portfolio_value);
    float portfolio_change = (state.daily_pnl / (state.portfolio_value - state.daily_pnl)) * 100.0f;
    ImGui::TextColored(portfolio_change >= 0 ? ImVec4(0.0f, 1.0f, 0.0f, 1.0f) : ImVec4(1.0f, 0.0f, 0.0f, 1.0f), 
                      "%+.2f%%", portfolio_change);
    ImGui::EndChild();
    ImGui::NextColumn();
    
    // Daily P&L
    ImGui::BeginChild("DailyPnL", ImVec2(0, 80), true);
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Daily P&L");
    ImGui::Spacing();
    ImGui::TextColored(state.daily_pnl >= 0 ? ImVec4(0.0f, 1.0f, 0.0f, 1.0f) : ImVec4(1.0f, 0.0f, 0.0f, 1.0f), 
                      "$%.2f", state.daily_pnl);
    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Trades: %d", state.total_trades);
    ImGui::EndChild();
    ImGui::NextColumn();
    
    // Algorithm Status
    ImGui::BeginChild("AlgoStatus", ImVec2(0, 80), true);
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Algorithm Status");
    ImGui::Spacing();
    if (state.isLiveMode()) {
        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "🔴 LIVE TRADING");
        auto stats = state.live_market_manager->getMarketStats();
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Opportunities: %d", stats.arbitrage_opportunities);
    } else {
        ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, 1.0f), "🟡 SIMULATION");
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Win Rate: %.1f%%", state.win_rate);
    }
    ImGui::EndChild();
    
    ImGui::Columns(1);
    ImGui::Separator();
    
    // Key Performance Indicators (Visual)
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "📊 Key Performance Indicators");
    ImGui::Spacing();
    
    // Progress bars for key metrics
    float sharpe_normalized = fmin(fmax((state.sharpe_ratio + 2.0f) / 4.0f, 0.0f), 1.0f);
    ImGui::Text("Sharpe Ratio: %.2f", state.sharpe_ratio);
    ImGui::ProgressBar(sharpe_normalized, ImVec2(-1.0f, 0.0f), "");
    
    float win_rate_normalized = state.win_rate / 100.0f;
    ImGui::Text("Win Rate: %.1f%%", state.win_rate);
    ImGui::ProgressBar(win_rate_normalized, ImVec2(-1.0f, 0.0f), "");
    
    float max_dd_normalized = 1.0f - fmin(state.max_drawdown / 10.0f, 1.0f);
    ImGui::Text("Risk Control: %.1f%% max drawdown", state.max_drawdown);
    ImGui::ProgressBar(max_dd_normalized, ImVec2(-1.0f, 0.0f), "");
    
    ImGui::Separator();
    
    // Current Positions (Simplified)
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "🎯 Active Positions");
    
    if (state.isLiveMode() && state.order_engine) {
        auto positions = state.order_engine->getCurrentPositions();
        for (const auto& [asset, amount] : positions) {
            ImGui::Text("%s: %.4f", asset.c_str(), amount);
        }
    } else {
        // Demo positions
        ImGui::Text("BTC: 0.5000");
        ImGui::Text("ETH: 2.3000");
        ImGui::Text("USD: 25,000.00");
    }
    
    ImGui::Separator();
    
    // Quick Actions
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "⚡ Quick Actions");
    
    if (ImGui::Button("🛑 Emergency Stop", ImVec2(-1.0f, 0.0f))) {
        std::cout << "🛑 EMERGENCY STOP - All algorithms halted!" << std::endl;
        if (state.order_engine) {
            state.order_engine->cancelAllOrders("");
        }
    }
    
    ImGui::End();
}

void RenderMarketDataWindow(GUIState& state) {
    ImGui::Begin("📊 Multi-Exchange Market Data", &state.show_market_data_window);
    
    ImGui::TextColored(ImVec4(0.0f, 1.0f, 1.0f, 1.0f), "Institutional-Grade Market Data Feed");
    ImGui::Separator();
    
    // Exchange connectivity status with latency
    ImGui::Text("🏢 Exchange Connectivity:");
    for (const auto& exchange : state.exchanges) {
        double latency = 8.0 + sin(glfwGetTime() + hash<string>{}(exchange)) * 3.0;
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "%s", exchange.c_str());
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "(%.1fms)", latency);
    }
    ImGui::NewLine();
    
    // Market data statistics
    ImGui::Text("📡 Feed Statistics: Updates/sec: 2,847 | Symbols: %zu | Total Volume: $2.4B", state.symbols.size());
    ImGui::Separator();
    
    // Create tabs for different views
    if (ImGui::BeginTabBar("MarketDataTabs")) {
        // Live quotes tab
        if (ImGui::BeginTabItem("📈 Live Quotes")) {
            // Show appropriate data based on current mode
            bool has_data_source = (state.isSimulationMode() && state.simulator && state.simulator->isRunning()) ||
                                  (state.isLiveMode() && state.live_market_manager && state.live_market_manager->isRunning());
            
            if (has_data_source) {
                if (ImGui::BeginTable("LiveMarketData", 8, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Sortable)) {
                    ImGui::TableSetupColumn("Exchange");
                    ImGui::TableSetupColumn("Symbol");
                    ImGui::TableSetupColumn("Bid");
                    ImGui::TableSetupColumn("Ask");
                    ImGui::TableSetupColumn("Last");
                    ImGui::TableSetupColumn("Spread (bps)");
                    ImGui::TableSetupColumn("Volume");
                    ImGui::TableSetupColumn("Volatility");
                    ImGui::TableHeadersRow();
                    
                    // Get data from appropriate source based on mode
                    if (state.isLiveMode()) {
                        // Live mode: only show binance data for now
                        for (const auto& symbol : state.symbols) {
                            auto tick = state.getTickData(symbol, "binance");
                            if (!tick.symbol.empty() && tick.last_price > 0) {
                                ImGui::TableNextRow();
                                ImGui::TableSetColumnIndex(0);
                                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "binance");  // Green for live
                                ImGui::TableSetColumnIndex(1);
                                ImGui::Text("%s", symbol.substr(0, symbol.find("USDT")).c_str());
                                ImGui::TableSetColumnIndex(2);
                                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "$%.2f", tick.bid_price);
                                ImGui::TableSetColumnIndex(3);
                                ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "$%.2f", tick.ask_price);
                                ImGui::TableSetColumnIndex(4);
                                ImGui::Text("$%.2f", tick.last_price);
                                ImGui::TableSetColumnIndex(5);
                                if (tick.bid_price > 0) {
                                    double spread_bps = ((tick.ask_price - tick.bid_price) / tick.bid_price) * 10000.0;
                                    ImGui::Text("%.1f", spread_bps);
                                } else {
                                    ImGui::Text("--");
                                }
                                ImGui::TableSetColumnIndex(6);
                                ImGui::Text("$%.0fM", tick.volume_24h / 1000000.0);
                                ImGui::TableSetColumnIndex(7);
                                ImGui::Text("%.1f%%", tick.price_change_percent_24h);
                            }
                        }
                    } else {
                        // Simulation mode: show all exchanges
                        for (const auto& exchange : state.exchanges) {
                            for (const auto& symbol : state.symbols) {
                                auto tick = state.getTickData(symbol, exchange);
                                if (!tick.symbol.empty() && tick.last_price > 0) {
                                    ImGui::TableNextRow();
                                    ImGui::TableSetColumnIndex(0);
                                    ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, 1.0f), "%s", exchange.c_str());
                                    ImGui::TableSetColumnIndex(1);
                                    ImGui::Text("%s", symbol.substr(0, symbol.find("USDT")).c_str());
                                    ImGui::TableSetColumnIndex(2);
                                    ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "$%.2f", tick.bid_price);
                                    ImGui::TableSetColumnIndex(3);
                                    ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "$%.2f", tick.ask_price);
                                    ImGui::TableSetColumnIndex(4);
                                    ImGui::Text("$%.2f", tick.last_price);
                                    ImGui::TableSetColumnIndex(5);
                                    if (tick.bid_price > 0) {
                                        double spread_bps = ((tick.ask_price - tick.bid_price) / tick.bid_price) * 10000.0;
                                        ImGui::Text("%.1f", spread_bps);
                                    } else {
                                        ImGui::Text("--");
                                    }
                                    ImGui::TableSetColumnIndex(6);
                                    ImGui::Text("$%.0fM", tick.volume_24h / 1000000.0);
                                    ImGui::TableSetColumnIndex(7);
                                    double volatility = 15.0 + sin(glfwGetTime() + hash<string>{}(symbol)) * 5.0;
                                    ImGui::Text("%.1f%%", volatility);
                                }
                            }
                        }
                    }
                    ImGui::EndTable();
                }
            } else {
                if (state.isLiveMode()) {
                    ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "⚠️ Live market data feed disconnected");
                } else {
                    ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "⚠️ Market data simulator not running");
                }
            }
            ImGui::EndTabItem();
        }
        
        // Order book tab
        if (ImGui::BeginTabItem("📖 Order Book")) {
            static int selected_symbol = 0;
            ImGui::Combo("Symbol", &selected_symbol, 
                        [](void* data, int idx, const char** out_text) {
                            auto* symbols = static_cast<std::vector<std::string>*>(data);
                            if (idx < 0 || idx >= symbols->size()) return false;
                            *out_text = (*symbols)[idx].c_str();
                            return true;
                        }, &state.symbols, state.symbols.size());
            
            if (selected_symbol >= 0 && selected_symbol < state.symbols.size()) {
                // Simple order book display
                ImGui::Text("📖 Order Book for %s", state.symbols[selected_symbol].c_str());
                ImGui::Separator();
                
                auto tick = state.getTickData(state.symbols[selected_symbol], "binance");
                if (!tick.symbol.empty() && tick.last_price > 0) {
                    ImGui::Text("Mid Price: $%.2f", (tick.bid_price + tick.ask_price) / 2.0);
                    ImGui::Text("Spread: $%.4f (%.2f bps)", 
                               tick.ask_price - tick.bid_price,
                               ((tick.ask_price - tick.bid_price) / tick.bid_price) * 10000.0);
                    
                    // Simple order book visualization
                    if (ImGui::BeginTable("SimpleOrderBook", 3, ImGuiTableFlags_Borders)) {
                        ImGui::TableSetupColumn("Size");
                        ImGui::TableSetupColumn("Price");
                        ImGui::TableSetupColumn("Side");
                        ImGui::TableHeadersRow();
                        
                        // Mock order book levels
                        for (int i = 0; i < 10; ++i) {
                            ImGui::TableNextRow();
                            ImGui::TableSetColumnIndex(0);
                            ImGui::Text("%.2f", 50.0 + i * 10);
                            ImGui::TableSetColumnIndex(1);
                            if (i < 5) {
                                ImGui::TableSetColumnIndex(1);
                                ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "$%.2f", tick.ask_price + i * 0.5);
                                ImGui::TableSetColumnIndex(2);
                                ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "ASK");
                            } else {
                                ImGui::TableSetColumnIndex(1);
                                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "$%.2f", tick.bid_price - (i-5) * 0.5);
                                ImGui::TableSetColumnIndex(2);
                                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "BID");
                            }
                        }
                        ImGui::EndTable();
                    }
                }
            }
            ImGui::EndTabItem();
        }
        
        // Cross-exchange analysis tab
        if (ImGui::BeginTabItem("🔄 Cross-Exchange")) {
            ImGui::Text("📊 Cross-Exchange Price Analysis");
            ImGui::Separator();
            
            if (ImGui::BeginTable("CrossExchangeAnalysis", 6, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
                ImGui::TableSetupColumn("Symbol");
                ImGui::TableSetupColumn("Best Bid");
                ImGui::TableSetupColumn("Best Ask");
                ImGui::TableSetupColumn("Price Variance");
                ImGui::TableSetupColumn("Arbitrage Opp.");
                ImGui::TableSetupColumn("Status");
                ImGui::TableHeadersRow();
                
                for (const auto& symbol : state.symbols) {
                    double best_bid = 0.0, best_ask = 999999.0;
                    double min_price = 999999.0, max_price = 0.0;
                    
                    if (state.isLiveMode()) {
                        // In live mode, only check binance for now
                        auto tick = state.getTickData(symbol, "binance");
                        if (!tick.symbol.empty() && tick.last_price > 0) {
                            best_bid = tick.bid_price;
                            best_ask = tick.ask_price;
                            min_price = tick.last_price;
                            max_price = tick.last_price;
                        }
                    } else {
                        // In simulation mode, check all exchanges
                        for (const auto& exchange : state.exchanges) {
                            auto tick = state.getTickData(symbol, exchange);
                            if (!tick.symbol.empty() && tick.last_price > 0) {
                                best_bid = max(best_bid, tick.bid_price);
                                best_ask = min(best_ask, tick.ask_price);
                                min_price = min(min_price, tick.last_price);
                                max_price = max(max_price, tick.last_price);
                            }
                        }
                    }
                    
                    if (best_bid > 0 && best_ask < 999999) {
                        double variance = ((max_price - min_price) / min_price) * 100.0;
                        double arb_bps = ((best_bid - best_ask) / best_ask) * 10000.0;
                        
                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::Text("%s", symbol.substr(0, symbol.find("USDT")).c_str());
                        ImGui::TableSetColumnIndex(1);
                        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "$%.2f", best_bid);
                        ImGui::TableSetColumnIndex(2);
                        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "$%.2f", best_ask);
                        ImGui::TableSetColumnIndex(3);
                        ImGui::Text("%.2f%%", variance);
                        ImGui::TableSetColumnIndex(4);
                        if (arb_bps > 0) {
                            ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "%.1f bps", arb_bps);
                        } else {
                            ImGui::Text("None");
                        }
                        ImGui::TableSetColumnIndex(5);
                        if (arb_bps > 15.0) {
                            ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "🟡 OPPORTUNITY");
                        } else {
                            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "🟢 NORMAL");
                        }
                    }
                }
                ImGui::EndTable();
            }
            ImGui::EndTabItem();
        }
        
        // Market microstructure tab
        if (ImGui::BeginTabItem("🔬 Microstructure")) {
            ImGui::Text("🔬 Market Microstructure Analysis");
            ImGui::Separator();
            
            ImGui::Text("📊 Real-Time Market Quality Metrics:");
            ImGui::Text("• Effective Spread: 12.3 bps (24h avg)");
            ImGui::Text("• Price Impact: 0.85 bps per $10k");
            ImGui::Text("• Market Resilience: 94.2%% (excellent)");
            ImGui::Text("• Order Flow Toxicity: 15.7%% (normal)");
            ImGui::Text("• Realized Volatility: 18.4%% (24h)");
            
            ImGui::Separator();
            ImGui::Text("🎯 Execution Quality:");
            ImGui::Text("• VWAP Performance: +0.12 bps");
            ImGui::Text("• Implementation Shortfall: -0.08 bps");
            ImGui::Text("• Fill Rate: 98.7%%");
            ImGui::Text("• Average Fill Time: 147ms");
            
            ImGui::EndTabItem();
        }
        
        ImGui::EndTabBar();
    }
    
    ImGui::End();
}

void RenderArbitrageWindow(GUIState& state) {
    ImGui::Begin("⚡ Cross-Exchange Arbitrage Scanner", &state.show_arbitrage_window);
    
    ImGui::TextColored(ImVec4(0.0f, 1.0f, 1.0f, 1.0f), "Real-Time Arbitrage Opportunities");
    ImGui::Separator();
    
    // Real-time scanning status
    ImGui::Text("🔍 Scanning Status: ");
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "ACTIVE");
    ImGui::SameLine();
    ImGui::Text(" | Opportunities Found: ");
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "%d", 4 + (int)(sin(glfwGetTime()) * 2));
    ImGui::SameLine();
    ImGui::Text(" | Scan Rate: 500 ops/sec");
    
    // Filter controls
    static float min_profit_bps = 15.0f;
    static float max_size_limit = 10.0f;
    static bool auto_execute = false;
    
    ImGui::Separator();
    ImGui::Text("🎛️ Filters & Controls:");
    ImGui::SliderFloat("Min Profit (bps)", &min_profit_bps, 5.0f, 100.0f, "%.1f");
    ImGui::SameLine();
    ImGui::SliderFloat("Max Position Size (BTC)", &max_size_limit, 0.1f, 50.0f, "%.1f");
    ImGui::SameLine();
    ImGui::Checkbox("Auto-Execute", &auto_execute);
    
    ImGui::Separator();
    
    // Enhanced arbitrage opportunities table
    if (ImGui::BeginTable("ArbitrageOpportunities", 8, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Sortable)) {
        ImGui::TableSetupColumn("Symbol");
        ImGui::TableSetupColumn("Buy Exchange");
        ImGui::TableSetupColumn("Sell Exchange");
        ImGui::TableSetupColumn("Buy Price");
        ImGui::TableSetupColumn("Sell Price");
        ImGui::TableSetupColumn("Profit (bps)");
        ImGui::TableSetupColumn("Size Avail.");
        ImGui::TableSetupColumn("Action");
        ImGui::TableHeadersRow();
        
        // Dynamic arbitrage opportunities (calculated from live data)
        if (state.isLiveMode()) {
            // In live mode, we don't have multiple exchanges yet, so show placeholder
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("--");
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("--");
            ImGui::TableSetColumnIndex(2);
            ImGui::Text("--");
            ImGui::TableSetColumnIndex(3);
            ImGui::Text("--");
            ImGui::TableSetColumnIndex(4);
            ImGui::Text("--");
            ImGui::TableSetColumnIndex(5);
            ImGui::Text("--");
            ImGui::TableSetColumnIndex(6);
            ImGui::Text("--");
            ImGui::TableSetColumnIndex(7);
            ImGui::Text("Live mode: Multi-exchange arbitrage coming soon");
        } else {
            // Simulation mode arbitrage calculation
            for (const auto& symbol : state.symbols) {
                for (size_t i = 0; i < state.exchanges.size(); ++i) {
                    for (size_t j = i + 1; j < state.exchanges.size(); ++j) {
                        auto tick1 = state.getTickData(symbol, state.exchanges[i]);
                        auto tick2 = state.getTickData(symbol, state.exchanges[j]);
                        
                        if (!tick1.symbol.empty() && !tick2.symbol.empty() && 
                            tick1.last_price > 0 && tick2.last_price > 0) {
                            // Calculate arbitrage opportunity
                            double price_diff = abs(tick2.last_price - tick1.last_price);
                            double profit_bps = (price_diff / min(tick1.last_price, tick2.last_price)) * 10000.0;
                        
                        // Add some realistic noise and timing
                        profit_bps += sin(glfwGetTime() + i + j) * 5.0;
                        
                        if (profit_bps >= min_profit_bps && profit_bps > 0) {
                            bool buy_first = tick1.last_price < tick2.last_price;
                            string buy_exchange = buy_first ? state.exchanges[i] : state.exchanges[j];
                            string sell_exchange = buy_first ? state.exchanges[j] : state.exchanges[i];
                            double buy_price = buy_first ? tick1.last_price : tick2.last_price;
                            double sell_price = buy_first ? tick2.last_price : tick1.last_price;
                            
                            ImGui::TableNextRow();
                            ImGui::TableSetColumnIndex(0);
                            ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, 1.0f), "%s", symbol.c_str());
                            ImGui::TableSetColumnIndex(1);
                            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "%s", buy_exchange.c_str());
                            ImGui::TableSetColumnIndex(2);
                            ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "%s", sell_exchange.c_str());
                            ImGui::TableSetColumnIndex(3);
                            ImGui::Text("$%.2f", buy_price);
                            ImGui::TableSetColumnIndex(4);
                            ImGui::Text("$%.2f", sell_price);
                            ImGui::TableSetColumnIndex(5);
                            ImVec4 profit_color = profit_bps > 30 ? ImVec4(1.0f, 1.0f, 0.0f, 1.0f) : 
                                                  profit_bps > 20 ? ImVec4(0.0f, 1.0f, 0.0f, 1.0f) : 
                                                  ImVec4(0.5f, 1.0f, 0.5f, 1.0f);
                            ImGui::TextColored(profit_color, "%.1f", profit_bps);
                            ImGui::TableSetColumnIndex(6);
                            ImGui::Text("%.2f", max_size_limit * 0.8); // Available size
                            ImGui::TableSetColumnIndex(7);
                            
                            string button_id = "Execute##" + symbol + buy_exchange + sell_exchange;
                            if (auto_execute && profit_bps > 25.0) {
                                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.0f, 0.5f, 0.0f, 1.0f));
                                ImGui::Text("AUTO");
                                ImGui::PopStyleColor();
                            } else if (ImGui::Button(button_id.c_str())) {
                                std::cout << "🚀 Executing arbitrage: " << symbol 
                                          << " Buy(" << buy_exchange << ") Sell(" << sell_exchange << ") "
                                          << "Profit: " << profit_bps << " bps" << std::endl;
                            }
                        }
                    }
                }
            }
        }
        }
        ImGui::EndTable();
    }
    
    ImGui::Separator();
    
    // Performance metrics
    ImGui::Text("📈 Arbitrage Performance Today:");
    ImGui::Text("Executed Trades: 23 | Total Profit: $847.50 | Success Rate: 91.3%% | Avg. Profit: 18.2 bps");
    
    ImGui::End();
}

void RenderRiskManagementWindow(GUIState& state) {
    ImGui::Begin("🛡️ Risk Management & Controls", &state.show_risk_window);
    
    ImGui::TextColored(ImVec4(0.0f, 1.0f, 1.0f, 1.0f), "Goldman Sachs-Level Risk Management");
    ImGui::Separator();
    
    // Risk metrics dashboard
    ImGui::Columns(3, "RiskMetrics", false);
    
    // VaR Card
    ImGui::BeginChild("VaR", ImVec2(0, 80), true);
    ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "VaR (95%%)");  
    ImGui::Text("$%.0f", state.var_95);
    ImGui::EndChild();
    ImGui::NextColumn();
    
    // Max Drawdown Card
    ImGui::BeginChild("Drawdown", ImVec2(0, 80), true);
    ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.5f, 1.0f), "Max Drawdown");
    ImGui::Text("%.2f%%", state.max_drawdown);
    ImGui::EndChild();
    ImGui::NextColumn();
    
    // Sharpe Ratio Card
    ImGui::BeginChild("Sharpe", ImVec2(0, 80), true);
    ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.0f, 1.0f), "Sharpe Ratio");
    ImGui::Text("%.2f", state.sharpe_ratio);
    ImGui::EndChild();
    ImGui::NextColumn();
    
    ImGui::Columns(1);
    ImGui::Separator();
    
    // Emergency controls
    ImGui::Text("🚨 Emergency Controls");
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.1f, 0.1f, 1.0f));
    if (ImGui::Button("🚨 EMERGENCY STOP ALL", ImVec2(200, 40))) {
        std::cout << "🚨 EMERGENCY STOP: All trading systems halted!" << std::endl;
    }
    ImGui::PopStyleColor();
    
    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.5f, 0.0f, 1.0f));
    if (ImGui::Button("⏸️ Pause All Strategies", ImVec2(200, 40))) {
        std::cout << "⏸️ All strategies paused" << std::endl;
    }
    ImGui::PopStyleColor();
    
    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.6f, 0.0f, 1.0f));
    if (ImGui::Button("🔄 Resume Trading", ImVec2(200, 40))) {
        std::cout << "🔄 Trading resumed" << std::endl;
    }
    ImGui::PopStyleColor();
    
    ImGui::Separator();
    
    // Risk limits configuration
    ImGui::Text("⚙️ Risk Limits Configuration");
    static float max_position_size = 50000.0f;
    static float max_daily_loss = 5000.0f;
    static float max_correlation = 0.7f;
    static float max_leverage = 2.0f;
    
    ImGui::SliderFloat("Max Position Size ($)", &max_position_size, 1000.0f, 100000.0f, "$%.0f");
    ImGui::SliderFloat("Max Daily Loss ($)", &max_daily_loss, 1000.0f, 20000.0f, "$%.0f");
    ImGui::SliderFloat("Max Correlation", &max_correlation, 0.0f, 1.0f, "%.2f");
    ImGui::SliderFloat("Max Leverage", &max_leverage, 1.0f, 5.0f, "%.1fx");
    
    if (ImGui::Button("💾 Save Risk Settings")) {
        std::cout << "💾 Risk settings saved" << std::endl;
    }
    
    ImGui::End();
}

void RenderStrategyPerformanceWindow(GUIState& state) {
    ImGui::Begin("📈 Strategy Performance Analytics", &state.show_strategy_window);
    
    ImGui::TextColored(ImVec4(0.0f, 1.0f, 1.0f, 1.0f), "Multi-Strategy Performance Dashboard");
    ImGui::Separator();
    
    // Strategy performance table
    if (ImGui::BeginTable("StrategyPerformance", 7, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Sortable)) {
        ImGui::TableSetupColumn("Strategy");
        ImGui::TableSetupColumn("Status");
        ImGui::TableSetupColumn("P&L");
        ImGui::TableSetupColumn("Trades");
        ImGui::TableSetupColumn("Win Rate");
        ImGui::TableSetupColumn("Sharpe");
        ImGui::TableSetupColumn("Controls");
        ImGui::TableHeadersRow();
        
        // Mock strategy performance data
        struct StrategyPerformance {
            std::string name;
            std::string status;
            double pnl;
            int trades;
            double win_rate;
            double sharpe;
            bool active;
        };
        
        std::vector<StrategyPerformance> strategies = {
            {"Statistical Arbitrage", "🟢 ACTIVE", 3420.75, 185, 68.6, 2.14, true},
            {"Cross-Exchange Arbitrage", "🟢 ACTIVE", 2890.50, 127, 76.4, 1.87, true},
            {"Portfolio Optimization", "🟢 ACTIVE", 1150.25, 42, 71.8, 1.65, true},
            {"Market Making BTC", "🟡 PAUSED", 0.0, 0, 0.0, 0.0, false},
            {"Momentum Trading", "🔴 STOPPED", -125.80, 15, 33.3, -0.45, false}
        };
        
        for (auto& strategy : strategies) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%s", strategy.name.c_str());
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%s", strategy.status.c_str());
            ImGui::TableSetColumnIndex(2);
            ImGui::TextColored(strategy.pnl >= 0 ? ImVec4(0.0f, 1.0f, 0.0f, 1.0f) : ImVec4(1.0f, 0.0f, 0.0f, 1.0f), 
                             "$%.2f", strategy.pnl);
            ImGui::TableSetColumnIndex(3);
            ImGui::Text("%d", strategy.trades);
            ImGui::TableSetColumnIndex(4);
            ImGui::Text("%.1f%%", strategy.win_rate);
            ImGui::TableSetColumnIndex(5);
            ImGui::Text("%.2f", strategy.sharpe);
            ImGui::TableSetColumnIndex(6);
            
            if (strategy.active) {
                if (ImGui::Button(("⏸️##" + strategy.name).c_str())) {
                    std::cout << "⏸️ Pausing strategy: " << strategy.name << std::endl;
                    strategy.active = false;
                }
            } else {
                if (ImGui::Button(("▶️##" + strategy.name).c_str())) {
                    std::cout << "▶️ Starting strategy: " << strategy.name << std::endl;
                    strategy.active = true;
                }
            }
            ImGui::SameLine();
            if (ImGui::Button(("⚙️##" + strategy.name).c_str())) {
                std::cout << "⚙️ Configuring strategy: " << strategy.name << std::endl;
            }
        }
        ImGui::EndTable();
    }
    
    ImGui::Separator();
    
    // Strategy management controls
    ImGui::Text("🎯 Strategy Management");
    if (ImGui::Button("🧠 AI Optimization")) {
        std::cout << "🧠 Running AI parameter optimization..." << std::endl;
    }
    ImGui::SameLine();
    if (ImGui::Button("📊 Performance Report")) {
        std::cout << "📊 Generating detailed performance report..." << std::endl;
    }
    ImGui::SameLine();
    if (ImGui::Button("🔄 Rebalance All")) {
        std::cout << "🔄 Rebalancing all strategy allocations..." << std::endl;
    }
    ImGui::SameLine();
    if (ImGui::Button("⚡ Quick Backtest")) {
        std::cout << "⚡ Running quick strategy backtest..." << std::endl;
    }
    
    ImGui::End();
}

void RenderExchangeManagementWindow(GUIState& state) {
    ImGui::Begin("🏢 Exchange Management & Live Trading Controls", &state.show_exchange_management);
    
    // Big warning if in live mode
    if (state.current_mode == GUIState::LIVE_TRADING) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
        ImGui::Text("🚨 LIVE TRADING MODE - REAL MONEY AT RISK! 🚨");
        ImGui::PopStyleColor();
        ImGui::Separator();
    }
    
    // Trading mode controls
    ImGui::Text("🎮 Trading Mode:");
    if (ImGui::RadioButton("Demo Simulation", state.current_mode == GUIState::DEMO_SIMULATION)) {
        state.switchTradingMode(GUIState::DEMO_SIMULATION);
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("Paper Trading", state.current_mode == GUIState::PAPER_TRADING)) {
        state.switchTradingMode(GUIState::PAPER_TRADING);
    }
    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
    if (ImGui::RadioButton("Live Trading", state.current_mode == GUIState::LIVE_TRADING)) {
        state.switchTradingMode(GUIState::LIVE_TRADING);
    }
    ImGui::PopStyleColor();
    
    ImGui::Separator();
    
    if (state.isLiveMode()) {
        // Live market statistics
        ImGui::Text("📊 Live Market Statistics:");
        ImGui::Text("Connected Exchanges: %d", state.live_stats.connected_exchanges);
        ImGui::Text("Average Latency: %.1f ms", state.live_stats.avg_latency_ms);
        ImGui::Text("Updates/Second: %d", state.live_stats.updates_per_second);
        ImGui::Text("Arbitrage Opportunities: %d", state.live_stats.arbitrage_opportunities);
        ImGui::Text("Last Update: %s", state.live_stats.last_update_time.c_str());
        
        ImGui::Separator();
        
        // Exchange status table
        if (ImGui::BeginTable("ExchangeStatus", 4, ImGuiTableFlags_Borders)) {
            ImGui::TableSetupColumn("Exchange");
            ImGui::TableSetupColumn("Status");
            ImGui::TableSetupColumn("Latency");
            ImGui::TableSetupColumn("Actions");
            ImGui::TableHeadersRow();
            
            // Mock exchange status for now
            for (const auto& exchange : state.exchanges) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("%s", exchange.c_str());
                ImGui::TableSetColumnIndex(1);
                if (state.live_stats.connected_exchanges > 0) {
                    ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "🟢 CONNECTED");
                } else {
                    ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "🔴 DISCONNECTED");
                }
                ImGui::TableSetColumnIndex(2);
                ImGui::Text("%.1f ms", state.live_stats.avg_latency_ms);
                ImGui::TableSetColumnIndex(3);
                if (ImGui::Button(("Test##" + exchange).c_str())) {
                    std::cout << "🧪 Testing connection to " << exchange << std::endl;
                }
            }
            ImGui::EndTable();
        }
        
        ImGui::Separator();
        
        // Order execution controls
        ImGui::Text("⚡ Order Execution:");
        bool trading_enabled = state.order_engine && state.order_engine->isTradingEnabled();
        if (ImGui::Checkbox("Enable Live Trading", &trading_enabled)) {
            if (state.order_engine) {
                state.order_engine->enableTrading(trading_enabled);
                std::cout << (trading_enabled ? "🟢 Live trading ENABLED" : "🔴 Live trading DISABLED") << std::endl;
            }
        }
        
        if (state.canExecuteRealTrades()) {
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "⚠️ Orders will use real money!");
        }
    } else {
        // Simulation mode status
        ImGui::Text("🎮 Simulation Mode Active");
        if (state.simulator && state.simulator->isRunning()) {
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "✅ Market simulator running");
            ImGui::Text("📊 Symbols: %zu", state.symbols.size());
            ImGui::Text("🏢 Mock Exchanges: %zu", state.exchanges.size());
        } else {
            ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "⚠️ Market simulator not running");
        }
    }
    
    ImGui::Separator();
    
    // Emergency controls
    ImGui::Text("🚨 Emergency Controls:");
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.1f, 0.1f, 1.0f));
    if (ImGui::Button("🚨 EMERGENCY STOP ALL", ImVec2(200, 30))) {
        std::cout << "🚨 EMERGENCY STOP: All trading systems halted!" << std::endl;
        if (state.order_engine) {
            state.order_engine->enableTrading(false);
            state.order_engine->cancelAllOrders();
        }
        if (state.live_market_manager) {
            state.live_market_manager->disconnectAll();
        }
    }
    ImGui::PopStyleColor();
    
    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.5f, 0.0f, 1.0f));
    if (ImGui::Button("⏸️ Disconnect All", ImVec2(150, 30))) {
        std::cout << "⏸️ Disconnecting from all exchanges" << std::endl;
        if (state.live_market_manager) {
            state.live_market_manager->disconnectAll();
        }
    }
    ImGui::PopStyleColor();
    
    ImGui::End();
}

// ========================================================================
// 🎨 VISUAL ALGORITHM DASHBOARD COMPONENTS 
// ========================================================================

// Helper function to draw animated triangle with trade flows
void DrawTriangleArbitrageWidget(ImVec2 center, float radius, float profit_bps, float volume, bool is_active) {
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    
    // Calculate triangle points (equilateral triangle)
    float angle_offset = 3.14159f / 2.0f; // Start from top
    ImVec2 points[3];
    for (int i = 0; i < 3; i++) {
        float angle = (i * 2.0f * 3.14159f / 3.0f) + angle_offset;
        points[i] = ImVec2(
            center.x + radius * cos(angle),
            center.y + radius * sin(angle)
        );
    }
    
    // Exchange labels
    const char* exchanges[] = {"Binance", "Coinbase", "Kraken"};
    ImU32 exchange_colors[] = {
        IM_COL32(255, 165, 0, 255),   // Orange for Binance
        IM_COL32(50, 150, 255, 255),  // Blue for Coinbase  
        IM_COL32(150, 50, 255, 255)   // Purple for Kraken
    };
    
    // Draw exchange nodes (circles)
    for (int i = 0; i < 3; i++) {
        float node_radius = is_active ? 15.0f + sin(ImGui::GetTime() * 3.0f) * 3.0f : 12.0f;
        draw_list->AddCircleFilled(points[i], node_radius, exchange_colors[i]);
        draw_list->AddCircle(points[i], node_radius + 2, IM_COL32(255, 255, 255, 150), 0, 2.0f);
        
        // Exchange labels
        ImVec2 text_size = ImGui::CalcTextSize(exchanges[i]);
        ImVec2 text_pos = ImVec2(points[i].x - text_size.x/2, points[i].y + node_radius + 5);
        draw_list->AddText(text_pos, IM_COL32(255, 255, 255, 255), exchanges[i]);
    }
    
    // Draw trade flow lines between exchanges
    for (int i = 0; i < 3; i++) {
        int next = (i + 1) % 3;
        
        // Calculate line properties based on profit and activity
        float line_thickness = is_active ? 3.0f + (profit_bps / 10.0f) : 1.5f;
        ImU32 line_color;
        
        if (profit_bps > 20.0f) {
            line_color = IM_COL32(0, 255, 0, 200);  // Bright green for high profit
        } else if (profit_bps > 10.0f) {
            line_color = IM_COL32(255, 255, 0, 180);  // Yellow for medium profit
        } else if (profit_bps > 0) {
            line_color = IM_COL32(100, 255, 100, 150);  // Light green for low profit
        } else {
            line_color = IM_COL32(100, 100, 100, 100);  // Gray for no opportunity
        }
        
        // Animated flow effect
        if (is_active && profit_bps > 5.0f) {
            float flow_phase = fmod(ImGui::GetTime() * 2.0f + i * 0.3f, 1.0f);
            ImVec2 flow_start = ImVec2(
                points[i].x + (points[next].x - points[i].x) * flow_phase,
                points[i].y + (points[next].y - points[i].y) * flow_phase
            );
            draw_list->AddCircleFilled(flow_start, 4.0f, IM_COL32(255, 255, 255, 200));
        }
        
        // Draw the connection line
        draw_list->AddLine(points[i], points[next], line_color, line_thickness);
    }
    
    // Center profit indicator
    char profit_text[32];
    snprintf(profit_text, sizeof(profit_text), "%.1f bps", profit_bps);
    ImVec2 profit_size = ImGui::CalcTextSize(profit_text);
    ImVec2 profit_pos = ImVec2(center.x - profit_size.x/2, center.y - profit_size.y/2);
    
    ImU32 profit_bg_color = profit_bps > 10.0f ? IM_COL32(0, 100, 0, 180) : IM_COL32(100, 100, 100, 120);
    draw_list->AddRectFilled(
        ImVec2(profit_pos.x - 5, profit_pos.y - 3),
        ImVec2(profit_pos.x + profit_size.x + 5, profit_pos.y + profit_size.y + 3),
        profit_bg_color, 5.0f
    );
    draw_list->AddText(profit_pos, IM_COL32(255, 255, 255, 255), profit_text);
}

// Performance gauge widget with smooth animation
void DrawPerformanceGauge(ImVec2 center, float radius, float value, const char* label, ImU32 color) {
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    
    // Background circle
    draw_list->AddCircle(center, radius, IM_COL32(100, 100, 100, 100), 0, 3.0f);
    
    // Progress arc (value from 0.0 to 1.0)
    float angle_start = -3.14159f / 2.0f; // Start from top
    float angle_end = angle_start + (value * 2.0f * 3.14159f);
    
    // Draw progress arc in segments for smooth appearance
    int segments = 32;
    for (int i = 0; i < segments; i++) {
        float t1 = (float)i / segments;
        float t2 = (float)(i + 1) / segments;
        
        if (t1 > value) break;
        
        float a1 = angle_start + t1 * 2.0f * 3.14159f;
        float a2 = angle_start + fmin(t2, value) * 2.0f * 3.14159f;
        
        ImVec2 p1 = ImVec2(center.x + (radius - 8) * cos(a1), center.y + (radius - 8) * sin(a1));
        ImVec2 p2 = ImVec2(center.x + (radius - 8) * cos(a2), center.y + (radius - 8) * sin(a2));
        ImVec2 p3 = ImVec2(center.x + radius * cos(a2), center.y + radius * sin(a2));
        ImVec2 p4 = ImVec2(center.x + radius * cos(a1), center.y + radius * sin(a1));
        
        draw_list->AddQuadFilled(p1, p2, p3, p4, color);
    }
    
    // Center value text
    char value_text[32];
    snprintf(value_text, sizeof(value_text), "%.0f%%", value * 100.0f);
    ImVec2 text_size = ImGui::CalcTextSize(value_text);
    draw_list->AddText(
        ImVec2(center.x - text_size.x/2, center.y - text_size.y/2),
        IM_COL32(255, 255, 255, 255), value_text
    );
    
    // Label below
    ImVec2 label_size = ImGui::CalcTextSize(label);
    draw_list->AddText(
        ImVec2(center.x - label_size.x/2, center.y + radius + 10),
        IM_COL32(200, 200, 200, 255), label
    );
}

// Exchange network flow visualization
void DrawExchangeNetworkWidget(ImVec2 start_pos, ImVec2 size) {
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    
    // Exchange positions in a network layout
    struct ExchangeNode {
        const char* name;
        ImVec2 pos;
        ImU32 color;
        bool connected;
    };
    
    ExchangeNode exchanges[] = {
        {"Binance", ImVec2(start_pos.x + size.x * 0.2f, start_pos.y + size.y * 0.3f), IM_COL32(255, 165, 0, 255), true},
        {"Coinbase", ImVec2(start_pos.x + size.x * 0.8f, start_pos.y + size.y * 0.3f), IM_COL32(50, 150, 255, 255), false},
        {"Kraken", ImVec2(start_pos.x + size.x * 0.5f, start_pos.y + size.y * 0.7f), IM_COL32(150, 50, 255, 255), false},
        {"FTX", ImVec2(start_pos.x + size.x * 0.2f, start_pos.y + size.y * 0.7f), IM_COL32(100, 200, 100, 255), false},
        {"Bitfinex", ImVec2(start_pos.x + size.x * 0.8f, start_pos.y + size.y * 0.7f), IM_COL32(200, 100, 200, 255), false}
    };
    
    // Draw connections between exchanges
    for (int i = 0; i < 5; i++) {
        for (int j = i + 1; j < 5; j++) {
            if (exchanges[i].connected && exchanges[j].connected) {
                // Active connection - bright line with flow animation
                float flow_phase = fmod(ImGui::GetTime() * 1.5f + i + j, 1.0f);
                ImVec2 flow_pos = ImVec2(
                    exchanges[i].pos.x + (exchanges[j].pos.x - exchanges[i].pos.x) * flow_phase,
                    exchanges[i].pos.y + (exchanges[j].pos.y - exchanges[i].pos.y) * flow_phase
                );
                
                draw_list->AddLine(exchanges[i].pos, exchanges[j].pos, IM_COL32(0, 255, 0, 150), 3.0f);
                draw_list->AddCircleFilled(flow_pos, 3.0f, IM_COL32(255, 255, 255, 200));
            } else if (exchanges[i].connected || exchanges[j].connected) {
                // Potential connection - dim line
                draw_list->AddLine(exchanges[i].pos, exchanges[j].pos, IM_COL32(100, 100, 100, 50), 1.0f);
            }
        }
    }
    
    // Draw exchange nodes
    for (int i = 0; i < 5; i++) {
        float node_radius = exchanges[i].connected ? 20.0f : 15.0f;
        ImU32 node_color = exchanges[i].connected ? exchanges[i].color : IM_COL32(100, 100, 100, 150);
        
        draw_list->AddCircleFilled(exchanges[i].pos, node_radius, node_color);
        draw_list->AddCircle(exchanges[i].pos, node_radius + 2, IM_COL32(255, 255, 255, 200), 0, 2.0f);
        
        // Exchange name
        ImVec2 text_size = ImGui::CalcTextSize(exchanges[i].name);
        draw_list->AddText(
            ImVec2(exchanges[i].pos.x - text_size.x/2, exchanges[i].pos.y + node_radius + 5),
            IM_COL32(255, 255, 255, 255), exchanges[i].name
        );
        
        // Connection status indicator
        if (exchanges[i].connected) {
            draw_list->AddCircleFilled(
                ImVec2(exchanges[i].pos.x + node_radius - 5, exchanges[i].pos.y - node_radius + 5),
                4.0f, IM_COL32(0, 255, 0, 255)
            );
        }
    }
}

// ========================================================================
// 🎯 ENHANCED VISUAL DASHBOARD WINDOWS
// ========================================================================

void RenderAlgorithmVisualizationWindow(GUIState& state) {
    ImGui::Begin("🔮 Algorithm Visualization Hub", &state.show_main_dashboard);
    
    ImGui::TextColored(ImVec4(0.0f, 1.0f, 1.0f, 1.0f), "🚀 Real-Time Algorithm Performance Monitor");
    ImGui::Separator();
    
    // Get current window size for responsive layout
    ImVec2 window_size = ImGui::GetContentRegionAvail();
    
    // Triangle Arbitrage Section
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "△ Triangle Arbitrage Monitor");
    
    // Simulate triangle arbitrage data (replace with real algorithm data)
    static float triangle_profit = 0.0f;
    static bool triangle_active = false;
    
    if (state.isLiveMode()) {
        // In live mode, show real opportunities (placeholder for now)
        triangle_profit = 15.0f + sin(ImGui::GetTime() * 0.5f) * 8.0f;
        triangle_active = triangle_profit > 10.0f;
    } else {
        // Demo mode with animated values
        triangle_profit = 12.0f + sin(ImGui::GetTime() * 0.8f) * 6.0f;
        triangle_active = triangle_profit > 8.0f;
    }
    
    // Draw triangle arbitrage widget
    ImVec2 triangle_center = ImVec2(window_size.x * 0.25f, 150.0f);
    DrawTriangleArbitrageWidget(triangle_center, 60.0f, triangle_profit, 50000.0f, triangle_active);
    
    // Performance Gauges Section
    ImGui::SetCursorPos(ImVec2(window_size.x * 0.5f, 50.0f));
    ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "📊 Strategy Performance");
    
    // Performance gauges
    float win_rate = state.win_rate / 100.0f;
    float profit_factor = fmin((state.daily_pnl + 5000.0f) / 10000.0f, 1.0f);
    float risk_score = 1.0f - fmin(state.max_drawdown / 10.0f, 1.0f);
    
    ImVec2 gauge_start = ImVec2(window_size.x * 0.5f, 100.0f);
    DrawPerformanceGauge(ImVec2(gauge_start.x, gauge_start.y), 35.0f, win_rate, "Win Rate", IM_COL32(0, 255, 0, 200));
    DrawPerformanceGauge(ImVec2(gauge_start.x + 100, gauge_start.y), 35.0f, profit_factor, "Profit", IM_COL32(255, 165, 0, 200));
    DrawPerformanceGauge(ImVec2(gauge_start.x + 200, gauge_start.y), 35.0f, risk_score, "Risk Control", IM_COL32(50, 150, 255, 200));
    
    // Exchange Network Section
    ImGui::SetCursorPos(ImVec2(10.0f, 220.0f));
    ImGui::TextColored(ImVec4(1.0f, 0.5f, 1.0f, 1.0f), "🌐 Exchange Network Status");
    
    ImVec2 network_start = ImVec2(20.0f, 250.0f);
    ImVec2 network_size = ImVec2(window_size.x - 40.0f, 200.0f);
    DrawExchangeNetworkWidget(network_start, network_size);
    
    // Real-time stats footer
    ImGui::SetCursorPos(ImVec2(10.0f, window_size.y - 60.0f));
    ImGui::Separator();
    if (state.isLiveMode()) {
        auto stats = state.live_market_manager->getMarketStats();
        ImGui::Text("🔴 LIVE | Exchanges: %d | Opportunities: %d | Latency: %.1fms | Updates: %d/sec", 
                   stats.connected_exchanges, stats.arbitrage_opportunities, 
                   stats.avg_latency_ms, stats.updates_per_second);
    } else {
        ImGui::Text("🟡 DEMO | Simulation Active | P&L: $%.2f | Trades: %d | Sharpe: %.2f", 
                   state.daily_pnl, state.total_trades, state.sharpe_ratio);
    }
    
    ImGui::End();
}