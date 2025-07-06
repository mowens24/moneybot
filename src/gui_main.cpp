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

using namespace moneybot;

int main(int argc, char** argv) {
    // Initialize TradingEngine (reuse config.json)
    nlohmann::json config;
    {
        std::ifstream config_stream("config.json");
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
        auto metrics = engine->getPerformanceMetrics();
        auto status = engine->getStatus();
        double bid = engine->getBestBid();
        double ask = engine->getBestAsk();
        double spread = (ask > 0 && bid > 0) ? ask - bid : 0.0;
        ImGui::Text("PnL: $%.2f", metrics["total_pnl"].get<double>());
        ImGui::Text("Total Trades: %d", metrics["total_trades"].get<int>());
        ImGui::Text("Uptime: %llds", status["uptime_seconds"].get<int64_t>());
        ImGui::Text("Connection: %s", engine->isWsConnected() ? "Connected" : "Disconnected");
        ImGui::Text("Last Event: %s", engine->getLastEvent().c_str());
        ImGui::Separator();
        ImGui::Text("Order Book");
        ImGui::Text("Best Bid: %.2f", bid);
        ImGui::Text("Best Ask: %.2f", ask);
        ImGui::Text("Spread: %.2f", spread);
        // Spread bar visual
        float spread_norm = (spread > 0 && bid > 0) ? std::min(1.0f, float(spread / (bid * 0.001f))) : 0.0f;
        ImGui::ProgressBar(spread_norm, ImVec2(300, 0), "Spread");
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