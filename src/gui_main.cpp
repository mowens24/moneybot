// Minimal ImGui GUI for MoneyBot
#include "moneybot.h"


#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <GLFW/glfw3.h>
#include <thread>
#include <chrono>
#include <memory>
#include <fstream> // For std::ifstream
#include "dashboard_metrics_widget.h"
#include "dashboard_orderbook_widget.h"

using namespace moneybot;

int gui_main(int argc, char** argv) {
    // Initialize TradingEngine (reuse config.json)
    nlohmann::json config;
    {
        std::ifstream config_stream("/Users/mwo/moneybot/config.json");
        config_stream >> config;
    }
    auto engine = std::make_shared<TradingEngine>(config);
    engine->start();

    // Setup window
    if (!glfwInit()) return 1;
    // Request OpenGL 2.1 context for macOS compatibility
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    const char* glsl_version = "#version 120";
    GLFWwindow* window = glfwCreateWindow(900, 600, "MoneyBot Dashboard", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);




    // Main loop
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("MoneyBot HFT Dashboard");
        // Metrics widget
        auto metrics = engine->getPerformanceMetrics();
        auto status = engine->getStatus();
        moneybot::RenderMetricsWidget(metrics, status);

        // Order book widget (modular)
        std::vector<moneybot::OrderBookEntry> bids, asks;
        for (const auto& kv : engine->getTopBids(10))
            bids.push_back(moneybot::OrderBookEntry{kv.first, kv.second});
        for (const auto& kv : engine->getTopAsks(10))
            asks.push_back(moneybot::OrderBookEntry{kv.first, kv.second});
        moneybot::RenderOrderBookWidget(bids, asks);
        ImGui::End();




        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
    engine->stop();
    return 0;
}