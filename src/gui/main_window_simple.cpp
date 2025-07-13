#include "gui/main_window.h"
#include "gui/portfolio_window.h"
#include "gui/market_data_window.h"
#include "gui/algorithm_window.h"
#include "core/portfolio_manager.h"
#include "core/exchange_manager.h"
#include "strategy/strategy_engine.h"
#include "risk_manager.h"
#include <iostream>

namespace moneybot {
namespace gui {

MainWindow::MainWindow() 
    : show_portfolio_window_(true),
      show_market_data_window_(true),
      show_algorithm_window_(true),
      show_risk_window_(true),
      show_exchange_window_(true),
      show_demo_window_(false),
      dockspace_flags_(0),
      dockspace_setup_done_(false) {
    
    std::cout << "MainWindow initialized" << std::endl;
}

MainWindow::~MainWindow() {
    std::cout << "MainWindow destroyed" << std::endl;
}

void MainWindow::initialize(
    std::shared_ptr<PortfolioManager> portfolio_manager,
    std::shared_ptr<ExchangeManager> exchange_manager,
    std::shared_ptr<StrategyEngine> strategy_engine,
    std::shared_ptr<RiskManager> risk_manager) {
    
    // Store component references
    portfolio_manager_ = portfolio_manager;
    exchange_manager_ = exchange_manager;
    strategy_engine_ = strategy_engine;
    risk_manager_ = risk_manager;
    
    // Initialize modular windows only if components are available
    if (portfolio_manager_) {
        portfolio_window_ = std::make_unique<PortfolioWindow>(portfolio_manager_);
    }
    if (exchange_manager_) {
        market_data_window_ = std::make_unique<MarketDataWindow>(exchange_manager_);
        exchange_window_ = std::make_unique<ExchangeWindow>(exchange_manager_);
    }
    if (strategy_engine_) {
        algorithm_window_ = std::make_unique<AlgorithmWindow>(strategy_engine_);
    }
    if (risk_manager_ && portfolio_manager_) {
        risk_window_ = std::make_unique<RiskWindow>(risk_manager_, portfolio_manager_);
    }
    
    std::cout << "MainWindow components initialized" << std::endl;
}

void MainWindow::render() {
    // Render menu bar
    renderMenuBar();
    
    // Render all windows
    renderWindows();
    
    // Render status bar
    renderStatusBar();
    
    // Show ImGui demo window if requested
    if (show_demo_window_) {
        ImGui::ShowDemoWindow(&show_demo_window_);
    }
}

void MainWindow::setupDockspace() {
    // Simplified docking setup without advanced features
    // This is a placeholder for future implementation
}

void MainWindow::renderMenuBar() {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("New Strategy")) {
                std::cout << "New Strategy requested" << std::endl;
            }
            if (ImGui::MenuItem("Load Configuration")) {
                std::cout << "Load Configuration requested" << std::endl;
            }
            if (ImGui::MenuItem("Save Configuration")) {
                std::cout << "Save Configuration requested" << std::endl;
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Exit")) {
                std::cout << "Exit requested" << std::endl;
            }
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("View")) {
            ImGui::MenuItem("Portfolio", nullptr, &show_portfolio_window_);
            ImGui::MenuItem("Market Data", nullptr, &show_market_data_window_);
            ImGui::MenuItem("Algorithms", nullptr, &show_algorithm_window_);
            ImGui::MenuItem("Risk Management", nullptr, &show_risk_window_);
            ImGui::MenuItem("Exchange", nullptr, &show_exchange_window_);
            ImGui::Separator();
            ImGui::MenuItem("Demo Window", nullptr, &show_demo_window_);
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("Trading")) {
            if (ImGui::MenuItem("Start All Strategies")) {
                std::cout << "Start All Strategies requested" << std::endl;
            }
            if (ImGui::MenuItem("Stop All Strategies")) {
                std::cout << "Stop All Strategies requested" << std::endl;
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Emergency Stop")) {
                std::cout << "🛑 EMERGENCY STOP TRIGGERED!" << std::endl;
            }
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("Tools")) {
            if (ImGui::MenuItem("Backtest Strategy")) {
                std::cout << "Backtest Strategy requested" << std::endl;
            }
            if (ImGui::MenuItem("Export Data")) {
                std::cout << "Export Data requested" << std::endl;
            }
            if (ImGui::MenuItem("Settings")) {
                std::cout << "Settings requested" << std::endl;
            }
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("Help")) {
            if (ImGui::MenuItem("Documentation")) {
                std::cout << "Documentation requested" << std::endl;
            }
            if (ImGui::MenuItem("About")) {
                std::cout << "About requested" << std::endl;
            }
            ImGui::EndMenu();
        }
        
        ImGui::EndMainMenuBar();
    }
}

void MainWindow::renderWindows() {
    // Render portfolio window with error handling
    if (show_portfolio_window_ && portfolio_window_) {
        try {
            portfolio_window_->render();
        } catch (const std::exception& e) {
            std::cerr << "Error in portfolio window: " << e.what() << std::endl;
            show_portfolio_window_ = false; // Disable on error
        }
    }
    
    // Render market data window with error handling
    if (show_market_data_window_ && market_data_window_) {
        try {
            market_data_window_->render();
        } catch (const std::exception& e) {
            std::cerr << "Error in market data window: " << e.what() << std::endl;
            show_market_data_window_ = false; // Disable on error
        }
    }
    
    // Render algorithm window with error handling
    if (show_algorithm_window_ && algorithm_window_) {
        try {
            algorithm_window_->render();
        } catch (const std::exception& e) {
            std::cerr << "Error in algorithm window: " << e.what() << std::endl;
            show_algorithm_window_ = false; // Disable on error
        }
    }
    
    // Render risk window with error handling
    if (show_risk_window_ && risk_window_) {
        try {
            risk_window_->render();
        } catch (const std::exception& e) {
            std::cerr << "Error in risk window: " << e.what() << std::endl;
            show_risk_window_ = false; // Disable on error
        }
    }
    
    // Render exchange window with error handling
    if (show_exchange_window_ && exchange_window_) {
        try {
            exchange_window_->render();
        } catch (const std::exception& e) {
            std::cerr << "Error in exchange window: " << e.what() << std::endl;
            show_exchange_window_ = false; // Disable on error
        }
    }
}

void MainWindow::renderStatusBar() {
    // Simple status bar at bottom
    ImGui::SetNextWindowPos(ImVec2(0, ImGui::GetIO().DisplaySize.y - 25));
    ImGui::SetNextWindowSize(ImVec2(ImGui::GetIO().DisplaySize.x, 25));
    
    if (ImGui::Begin("##StatusBar", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar)) {
        
        // Connection status
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "🟢 Connected");
        
        ImGui::SameLine();
        ImGui::Text(" | ");
        ImGui::SameLine();
        
        // System status
        ImGui::Text("Strategies: 2/3 Active");
        
        ImGui::SameLine();
        ImGui::Text(" | ");
        ImGui::SameLine();
        
        // Performance summary
        if (portfolio_manager_) {
            double total_pnl = portfolio_manager_->getTotalPnL();
            ImVec4 pnl_color = (total_pnl >= 0) ? ImVec4(0.0f, 1.0f, 0.0f, 1.0f) : ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
            ImGui::TextColored(pnl_color, "P&L: $%.2f", total_pnl);
        } else {
            ImGui::Text("P&L: $0.00");
        }
        
        ImGui::SameLine();
        ImGui::Text(" | ");
        ImGui::SameLine();
        
        // Time
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        auto tm = *std::localtime(&time_t);
        ImGui::Text("%02d:%02d:%02d", tm.tm_hour, tm.tm_min, tm.tm_sec);
        
        ImGui::End();
    }
}

void MainWindow::renderWindowFlags() {
    // This can be used for debugging/configuration
    // Not implemented in this basic version
}

void MainWindow::showWindow(const std::string& window_name) {
    if (window_name == "portfolio") {
        show_portfolio_window_ = true;
    } else if (window_name == "market_data") {
        show_market_data_window_ = true;
    } else if (window_name == "algorithm") {
        show_algorithm_window_ = true;
    } else if (window_name == "risk") {
        show_risk_window_ = true;
    } else if (window_name == "exchange") {
        show_exchange_window_ = true;
    }
}

void MainWindow::hideWindow(const std::string& window_name) {
    if (window_name == "portfolio") {
        show_portfolio_window_ = false;
    } else if (window_name == "market_data") {
        show_market_data_window_ = false;
    } else if (window_name == "algorithm") {
        show_algorithm_window_ = false;
    } else if (window_name == "risk") {
        show_risk_window_ = false;
    } else if (window_name == "exchange") {
        show_exchange_window_ = false;
    }
}

void MainWindow::toggleWindow(const std::string& window_name) {
    if (window_name == "portfolio") {
        show_portfolio_window_ = !show_portfolio_window_;
    } else if (window_name == "market_data") {
        show_market_data_window_ = !show_market_data_window_;
    } else if (window_name == "algorithm") {
        show_algorithm_window_ = !show_algorithm_window_;
    } else if (window_name == "risk") {
        show_risk_window_ = !show_risk_window_;
    } else if (window_name == "exchange") {
        show_exchange_window_ = !show_exchange_window_;
    }
}

} // namespace gui
} // namespace moneybot
