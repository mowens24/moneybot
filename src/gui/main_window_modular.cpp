#include "gui/main_window.h"
#include "gui/portfolio_window.h"
#include "gui/market_data_window.h"
#include "gui/algorithm_window.h"
#include "gui/risk_window.h"
#include "gui/exchange_window.h"
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
    
    // Initialize modular windows
    portfolio_window_ = std::make_unique<PortfolioWindow>(portfolio_manager_);
    market_data_window_ = std::make_unique<MarketDataWindow>(exchange_manager_);
    algorithm_window_ = std::make_unique<AlgorithmWindow>(strategy_engine_);
    // risk_window_ = std::make_unique<RiskWindow>(risk_manager_);
    // exchange_window_ = std::make_unique<ExchangeWindow>(exchange_manager_);
    
    std::cout << "MainWindow components initialized" << std::endl;
}

void MainWindow::render() {
    // Setup dockspace
    setupDockspace();
    
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
    static bool opt_fullscreen = true;
    static bool opt_padding = false;
    static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

    // We are using the ImGuiWindowFlags_NoDocking flag to make the parent window not dockable into,
    // because it would be confusing to have two docking targets within each others.
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
    
    if (opt_fullscreen) {
        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        ImGui::SetNextWindowViewport(viewport->ID);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
        window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
    } else {
        dockspace_flags &= ~ImGuiDockNodeFlags_PassthruCentralNode;
    }

    // When using ImGuiDockNodeFlags_PassthruCentralNode, DockSpace() will render our background
    // and handle the pass-thru hole, so we ask Begin() to not render a background.
    if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
        window_flags |= ImGuiWindowFlags_NoBackground;

    // Important: note that we proceed even if Begin() returns false (aka window is collapsed).
    // This is because we want to keep our DockSpace() active. If a DockSpace() is inactive,
    // all active windows docked into it will lose their parent and become undocked.
    // We cannot preserve the docking relationship between an active window and an inactive docking, otherwise
    // any change of dockspace/settings would lead to windows being stuck in limbo and never being visible.
    if (!opt_padding)
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    
    ImGui::Begin("DockSpace Demo", nullptr, window_flags);
    
    if (!opt_padding)
        ImGui::PopStyleVar();

    if (opt_fullscreen)
        ImGui::PopStyleVar(2);

    // Submit the DockSpace
    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable) {
        ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
        
        // Setup initial layout
        if (!dockspace_setup_done_) {
            dockspace_setup_done_ = true;
            
            ImGui::DockBuilderRemoveNode(dockspace_id); // clear any previous layout
            ImGui::DockBuilderAddNode(dockspace_id, dockspace_flags | ImGuiDockNodeFlags_DockSpace);
            ImGui::DockBuilderSetNodeSize(dockspace_id, ImGui::GetMainViewport()->Size);

            // Split the dockspace into left, right, and bottom areas
            auto dock_id_left = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Left, 0.25f, nullptr, &dockspace_id);
            auto dock_id_right = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Right, 0.33f, nullptr, &dockspace_id);
            auto dock_id_bottom = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Down, 0.25f, nullptr, &dockspace_id);

            // Dock windows to specific areas
            ImGui::DockBuilderDockWindow("💼 Portfolio & Performance", dock_id_left);
            ImGui::DockBuilderDockWindow("🤖 Algorithm Dashboard", dockspace_id);
            ImGui::DockBuilderDockWindow("📊 Market Data", dock_id_right);
            ImGui::DockBuilderDockWindow("⚠️ Risk Management", dock_id_bottom);
            ImGui::DockBuilderDockWindow("🔄 Exchange Management", dock_id_bottom);
            
            ImGui::DockBuilderFinish(dockspace_id);
        }
    }

    ImGui::End();
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
    // Render portfolio window
    if (show_portfolio_window_ && portfolio_window_) {
        portfolio_window_->render();
    }
    
    // Render market data window
    if (show_market_data_window_ && market_data_window_) {
        market_data_window_->render();
    }
    
    // Render algorithm window
    if (show_algorithm_window_ && algorithm_window_) {
        algorithm_window_->render();
    }
    
    // Render risk window
    if (show_risk_window_ && risk_window_) {
        risk_window_->render();
    }
    
    // Render exchange window
    if (show_exchange_window_ && exchange_window_) {
        exchange_window_->render();
    }
}

void MainWindow::renderStatusBar() {
    // Status bar at the bottom
    if (ImGui::BeginViewportSideBar("##MainStatusBar", ImGui::GetMainViewport(), 
                                   ImGuiDir_Down, ImGui::GetFrameHeight(), 
                                   ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_MenuBar)) {
        
        if (ImGui::BeginMenuBar()) {
            // Connection status
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "🟢 Connected");
            
            ImGui::Separator();
            
            // System status
            ImGui::Text("Strategies: 2/3 Active");
            
            ImGui::Separator();
            
            // Performance summary
            if (portfolio_manager_) {
                ImGui::Text("P&L: $%.2f", portfolio_manager_->getTotalPnL());
            } else {
                ImGui::Text("P&L: $0.00");
            }
            
            ImGui::Separator();
            
            // Time
            auto now = std::chrono::system_clock::now();
            auto time_t = std::chrono::system_clock::to_time_t(now);
            auto tm = *std::localtime(&time_t);
            ImGui::Text("%02d:%02d:%02d", tm.tm_hour, tm.tm_min, tm.tm_sec);
            
            ImGui::EndMenuBar();
        }
        
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
