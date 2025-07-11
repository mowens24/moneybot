#pragma once

#include <memory>
#include <string>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

namespace moneybot {
namespace gui {

class MainWindow {
private:
    bool show_portfolio_window_ = true;
    bool show_market_data_window_ = true;
    bool show_algorithm_window_ = true;
    bool show_risk_window_ = true;
    bool show_exchange_window_ = true;
    bool show_demo_window_ = false;
    
    int dockspace_flags_ = 0;
    
    // Strategy states
    struct StrategyStates {
        bool market_maker = false;
        bool arbitrage = false;
        bool trend_following = false;
    } strategy_states_;
    
    // Risk limits
    struct RiskLimits {
        float max_position = 1000.0f;
        float max_order = 100.0f;
        float max_daily_loss = 500.0f;
    } risk_limits_;
    
public:
    MainWindow();
    ~MainWindow();
    
    // Main rendering loop
    void render();
    
private:
    void renderCentralWorkspace();
    void renderWindows();
    void renderPortfolioWindow();
    void renderMarketDataWindow();
    void renderAlgorithmWindow();
    void renderRiskWindow();
    void renderExchangeWindow();
    void renderStatusBar();
};

} // namespace gui
} // namespace moneybot
