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
#include <unistd.h>
#include <pty.h>
#include <sys/types.h>
#include <sys/wait.h>

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


    // --- Interactive Zsh Terminal State ---
    static int pty_master = -1;
    static pid_t shell_pid = -1;
    static std::string terminal_output;
    static char input_buffer[256] = "";
    static bool terminal_running = false;

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


        // --- Interactive Zsh Terminal Window ---
        ImGui::Begin("Interactive Zsh Terminal (PTY)");
        if (!terminal_running) {
            if (ImGui::Button("Start Interactive Zsh")) {
                int master_fd;
                int slave_fd;
                struct winsize ws = {24, 80, 0, 0};
                pid_t pid = forkpty(&master_fd, NULL, NULL, &ws);
                if (pid == 0) {
                    // Child: exec zsh
                    execlp("zsh", "zsh", (char*)NULL);
                    _exit(1);
                } else if (pid > 0) {
                    // Parent
                    // Set PTY master to non-blocking
                    #include <fcntl.h>
                    int flags = fcntl(master_fd, F_GETFL, 0);
                    fcntl(master_fd, F_SETFL, flags | O_NONBLOCK);
                    pty_master = master_fd;
                    shell_pid = pid;
                    terminal_running = true;
                    terminal_output.clear();
                    memset(input_buffer, 0, sizeof(input_buffer));
                }
            }
        } else {
            // --- Robust PTY read and shell monitoring ---
            bool shell_alive = true;
            if (shell_pid > 0) {
                int status = 0;
                pid_t result = waitpid(shell_pid, &status, WNOHANG);
                if (result == shell_pid) {
                    // Shell exited
                    shell_alive = false;
                }
            } else {
                shell_alive = false;
            }
            if (pty_master >= 0 && shell_alive) {
                char buf[256];
                ssize_t n;
                while ((n = read(pty_master, buf, sizeof(buf)-1)) > 0) {
                    buf[n] = 0;
                    terminal_output += buf;
                }
                // Input box
                ImGui::InputText("Input", input_buffer, sizeof(input_buffer), ImGuiInputTextFlags_EnterReturnsTrue);
                if (ImGui::IsItemDeactivatedAfterEdit() && strlen(input_buffer) > 0) {
                    std::string cmd = std::string(input_buffer) + "\n";
                    write(pty_master, cmd.c_str(), cmd.size());
                    memset(input_buffer, 0, sizeof(input_buffer));
                }
                if (ImGui::Button("Stop Terminal")) {
                    kill(shell_pid, SIGKILL);
                    close(pty_master);
                    pty_master = -1;
                    shell_pid = -1;
                    terminal_running = false;
                }
            } else {
                // Shell exited or PTY closed
                if (pty_master >= 0) {
                    close(pty_master);
                    pty_master = -1;
                }
                shell_pid = -1;
                terminal_running = false;
                ImGui::TextColored(ImVec4(1,0,0,1), "[Shell exited]");
            }
        }
        ImGui::BeginChild("TerminalOutput", ImVec2(0, 300), true);
        ImGui::TextUnformatted(terminal_output.c_str());
        ImGui::SetScrollHereY(1.0f); // Auto-scroll to bottom
        ImGui::EndChild();
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