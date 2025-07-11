#pragma once

#include <memory>
#include <string>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

// Include specific window headers to avoid incomplete type errors
#include "portfolio_window.h"
#include "market_data_window.h"
#include "algorithm_window.h"
#include "risk_window.h"
#include "exchange_window.h"

namespace moneybot {

// Forward declarations
class PortfolioManager;
class ExchangeManager;
class StrategyEngine;
class RiskManager;

namespace gui {

class MainWindow {
private:
    // Window visibility flags
    bool show_portfolio_window_ = true;
    bool show_market_data_window_ = true;
    bool show_algorithm_window_ = true;
    bool show_risk_window_ = true;
    bool show_exchange_window_ = true;
    bool show_demo_window_ = false;
    
    // Docking setup
    int dockspace_flags_ = 0;
    bool dockspace_setup_done_ = false;
    
    // Modular window instances
    std::unique_ptr<PortfolioWindow> portfolio_window_;
    std::unique_ptr<MarketDataWindow> market_data_window_;
    std::unique_ptr<AlgorithmWindow> algorithm_window_;
    std::unique_ptr<RiskWindow> risk_window_;
    std::unique_ptr<ExchangeWindow> exchange_window_;
    
    // Core system components
    std::shared_ptr<PortfolioManager> portfolio_manager_;
    std::shared_ptr<ExchangeManager> exchange_manager_;
    std::shared_ptr<StrategyEngine> strategy_engine_;
    std::shared_ptr<RiskManager> risk_manager_;

public:
    MainWindow();
    ~MainWindow();
    
    // Initialization
    void initialize(
        std::shared_ptr<PortfolioManager> portfolio_manager,
        std::shared_ptr<ExchangeManager> exchange_manager,
        std::shared_ptr<StrategyEngine> strategy_engine,
        std::shared_ptr<RiskManager> risk_manager
    );
    
    // Main rendering loop
    void render();
    
private:
    void setupDockspace();
    void renderMenuBar();
    void renderWindows();
    void renderStatusBar();
    void renderWindowFlags();
    
    // Window management
    void showWindow(const std::string& window_name);
    void hideWindow(const std::string& window_name);
    void toggleWindow(const std::string& window_name);
};

} // namespace gui
} // namespace moneybot
