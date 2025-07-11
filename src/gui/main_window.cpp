#include "gui/main_window.h"
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
      dockspace_flags_(0) {
    
    std::cout << "MainWindow initialized" << std::endl;
}

MainWindow::~MainWindow() {
    std::cout << "MainWindow destroyed" << std::endl;
}

void MainWindow::render() {
    // Create main menu bar
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("New Strategy")) {
                // Handle new strategy
            }
            if (ImGui::MenuItem("Load Configuration")) {
                // Handle load config
            }
            if (ImGui::MenuItem("Save Configuration")) {
                // Handle save config
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Exit")) {
                // Handle exit
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
        
        if (ImGui::BeginMenu("Tools")) {
            if (ImGui::MenuItem("Strategy Backtester")) {
                // Handle backtester
            }
            if (ImGui::MenuItem("Performance Analyzer")) {
                // Handle performance analyzer
            }
            if (ImGui::MenuItem("Risk Calculator")) {
                // Handle risk calculator
            }
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("Help")) {
            if (ImGui::MenuItem("Documentation")) {
                // Handle documentation
            }
            if (ImGui::MenuItem("About")) {
                // Handle about
            }
            ImGui::EndMenu();
        }
        
        ImGui::EndMainMenuBar();
    }
    
    // Create central workspace
    renderCentralWorkspace();
    
    // Render individual windows
    renderWindows();
    
    // Render status bar
    renderStatusBar();
}

void MainWindow::renderCentralWorkspace() {
    // Create a large central window for the main workspace
    if (ImGui::Begin("MoneyBot Dashboard")) {
        ImGui::Text("Welcome to MoneyBot Trading Dashboard");
        ImGui::Separator();
        
        // Basic layout with columns
        ImGui::Columns(2, "MainColumns");
        
        // Left column - Portfolio and Risk
        ImGui::Text("Portfolio & Risk Management");
        if (ImGui::Button("View Portfolio", ImVec2(-1, 0))) {
            show_portfolio_window_ = true;
        }
        if (ImGui::Button("Risk Settings", ImVec2(-1, 0))) {
            show_risk_window_ = true;
        }
        if (ImGui::Button("Emergency Stop", ImVec2(-1, 0))) {
            // Handle emergency stop
        }
        
        ImGui::NextColumn();
        
        // Right column - Market Data and Algorithms
        ImGui::Text("Market Data & Algorithms");
        if (ImGui::Button("Live Market Data", ImVec2(-1, 0))) {
            show_market_data_window_ = true;
        }
        if (ImGui::Button("Algorithm Controls", ImVec2(-1, 0))) {
            show_algorithm_window_ = true;
        }
        if (ImGui::Button("Strategy Manager", ImVec2(-1, 0))) {
            // Handle strategy manager
        }
        
        ImGui::Columns(1);
        
        ImGui::End();
    }
}

void MainWindow::renderWindows() {
    // Render individual windows if they're open
    if (show_portfolio_window_) {
        renderPortfolioWindow();
    }
    
    if (show_market_data_window_) {
        renderMarketDataWindow();
    }
    
    if (show_algorithm_window_) {
        renderAlgorithmWindow();
    }
    
    if (show_risk_window_) {
        renderRiskWindow();
    }
    
    if (show_exchange_window_) {
        renderExchangeWindow();
    }
    
    if (show_demo_window_) {
        ImGui::ShowDemoWindow(&show_demo_window_);
    }
}

void MainWindow::renderPortfolioWindow() {
    if (ImGui::Begin("Portfolio", &show_portfolio_window_)) {
        ImGui::Text("Portfolio Management");
        ImGui::Separator();
        
        // Portfolio summary
        ImGui::Text("Total Value: $10,000.00");
        ImGui::Text("P&L: +$500.00 (5.0%%)");
        ImGui::Text("Available Cash: $2,000.00");
        
        ImGui::Separator();
        
        // Holdings table
        if (ImGui::BeginTable("Holdings", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
            ImGui::TableSetupColumn("Asset");
            ImGui::TableSetupColumn("Quantity");
            ImGui::TableSetupColumn("Price");
            ImGui::TableSetupColumn("Value");
            ImGui::TableHeadersRow();
            
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("BTC");
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("0.1");
            ImGui::TableSetColumnIndex(2);
            ImGui::Text("$50,000");
            ImGui::TableSetColumnIndex(3);
            ImGui::Text("$5,000");
            
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("ETH");
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("2.0");
            ImGui::TableSetColumnIndex(2);
            ImGui::Text("$3,000");
            ImGui::TableSetColumnIndex(3);
            ImGui::Text("$6,000");
            
            ImGui::EndTable();
        }
        
        ImGui::End();
    }
}

void MainWindow::renderMarketDataWindow() {
    if (ImGui::Begin("Market Data", &show_market_data_window_)) {
        ImGui::Text("Live Market Data");
        ImGui::Separator();
        
        // Market data table
        if (ImGui::BeginTable("MarketData", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
            ImGui::TableSetupColumn("Symbol");
            ImGui::TableSetupColumn("Price");
            ImGui::TableSetupColumn("Change");
            ImGui::TableSetupColumn("Volume");
            ImGui::TableSetupColumn("Time");
            ImGui::TableHeadersRow();
            
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("BTC/USD");
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("$50,000");
            ImGui::TableSetColumnIndex(2);
            ImGui::TextColored(ImVec4(0, 1, 0, 1), "+2.5%%");
            ImGui::TableSetColumnIndex(3);
            ImGui::Text("1,234 BTC");
            ImGui::TableSetColumnIndex(4);
            ImGui::Text("12:34:56");
            
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("ETH/USD");
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("$3,000");
            ImGui::TableSetColumnIndex(2);
            ImGui::TextColored(ImVec4(1, 0, 0, 1), "-1.2%%");
            ImGui::TableSetColumnIndex(3);
            ImGui::Text("5,678 ETH");
            ImGui::TableSetColumnIndex(4);
            ImGui::Text("12:34:56");
            
            ImGui::EndTable();
        }
        
        ImGui::End();
    }
}

void MainWindow::renderAlgorithmWindow() {
    if (ImGui::Begin("Algorithms", &show_algorithm_window_)) {
        ImGui::Text("Algorithm Management");
        ImGui::Separator();
        
        // Algorithm controls
        ImGui::Text("Active Strategies:");
        ImGui::Checkbox("Market Maker", &strategy_states_.market_maker);
        ImGui::Checkbox("Arbitrage", &strategy_states_.arbitrage);
        ImGui::Checkbox("Trend Following", &strategy_states_.trend_following);
        
        ImGui::Separator();
        
        // Strategy performance
        ImGui::Text("Performance Summary:");
        ImGui::Text("Total Trades: 42");
        ImGui::Text("Win Rate: 67.5%%");
        ImGui::Text("Average Profit: $12.50");
        
        ImGui::End();
    }
}

void MainWindow::renderRiskWindow() {
    if (ImGui::Begin("Risk Management", &show_risk_window_)) {
        ImGui::Text("Risk Controls");
        ImGui::Separator();
        
        // Risk limits
        ImGui::Text("Position Limits:");
        ImGui::SliderFloat("Max Position Size", &risk_limits_.max_position, 0.0f, 10000.0f, "%.2f");
        ImGui::SliderFloat("Max Order Size", &risk_limits_.max_order, 0.0f, 1000.0f, "%.2f");
        ImGui::SliderFloat("Max Daily Loss", &risk_limits_.max_daily_loss, 0.0f, 5000.0f, "%.2f");
        
        ImGui::Separator();
        
        // Current exposure
        ImGui::Text("Current Exposure:");
        ImGui::ProgressBar(0.3f, ImVec2(0.0f, 0.0f), "30%%");
        ImGui::Text("Daily P&L: +$150.00");
        
        // Emergency controls
        ImGui::Separator();
        if (ImGui::Button("Emergency Stop All", ImVec2(-1, 0))) {
            // Handle emergency stop
        }
        
        ImGui::End();
    }
}

void MainWindow::renderExchangeWindow() {
    if (ImGui::Begin("Exchange", &show_exchange_window_)) {
        ImGui::Text("Exchange Connections");
        ImGui::Separator();
        
        // Exchange status
        ImGui::Text("Binance:");
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0, 1, 0, 1), "Connected");
        
        ImGui::Text("Coinbase:");
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(1, 0, 0, 1), "Disconnected");
        
        ImGui::Separator();
        
        // Connection controls
        if (ImGui::Button("Connect All")) {
            // Handle connect all
        }
        ImGui::SameLine();
        if (ImGui::Button("Disconnect All")) {
            // Handle disconnect all
        }
        
        ImGui::End();
    }
}

void MainWindow::renderStatusBar() {
    // Create status bar at the bottom
    if (ImGui::Begin("Status", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove)) {
        ImGui::Text("Status: Running | Portfolio: $10,000.00 | P&L: +$500.00 | Connected: Binance");
        ImGui::End();
    }
}

} // namespace gui
} // namespace moneybot
