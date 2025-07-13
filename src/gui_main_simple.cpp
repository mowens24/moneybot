#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <chrono>
#include <thread>

int main(int argc, char** argv) {
    std::cout << "🚀 Starting MoneyBot Simple GUI..." << std::endl;
    
    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "❌ Failed to initialize GLFW" << std::endl;
        return -1;
    }
    
    std::cout << "✅ GLFW initialized" << std::endl;
    
    // Set OpenGL version
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    
    // Create window
    GLFWwindow* window = glfwCreateWindow(1280, 720, "MoneyBot - WORKING GUI", NULL, NULL);
    if (window == NULL) {
        std::cerr << "❌ Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    
    std::cout << "✅ Window created successfully" << std::endl;
    
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync
    
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    
    std::cout << "✅ ImGui context created" << std::endl;
    
    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    
    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 150");
    
    std::cout << "✅ ImGui backends initialized" << std::endl;
    std::cout << "🎉 GUI STARTED SUCCESSFULLY!" << std::endl;
    
    // Demo variables
    bool show_demo_window = true;
    bool show_another_window = false;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    
    double portfolio_value = 100000.0;
    double total_pnl = 1234.56;
    int trades_today = 42;
    double win_rate = 68.5;
    
    // Main loop
    while (!glfwWindowShouldClose(window)) {
        // Poll and handle events
        glfwPollEvents();
        
        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        
        // 1. Show the big demo window
        if (show_demo_window)
            ImGui::ShowDemoWindow(&show_demo_window);
        
        // 2. Show MoneyBot trading window
        {
            ImGui::Begin("🚀 MoneyBot Trading Dashboard");
            
            ImGui::Text("🎉 SUCCESS! MoneyBot GUI is working perfectly!");
            ImGui::Separator();
            
            // Portfolio metrics
            ImGui::Text("💼 Portfolio Overview:");
            ImGui::Text("Total Value: $%.2f", portfolio_value);
            ImGui::Text("P&L: $%.2f", total_pnl);
            ImGui::Text("Trades Today: %d", trades_today);
            ImGui::Text("Win Rate: %.1f%%", win_rate);
            
            ImGui::Separator();
            
            // Interactive elements
            if (ImGui::Button("Execute Trade")) {
                std::cout << "🔄 Trade executed!" << std::endl;
                trades_today++;
                total_pnl += (rand() % 200 - 100); // Random P&L change
            }
            
            ImGui::SameLine();
            if (ImGui::Button("Refresh Data")) {
                std::cout << "📊 Data refreshed!" << std::endl;
                portfolio_value += (rand() % 1000 - 500);
            }
            
            ImGui::SameLine();
            if (ImGui::Button("Risk Analysis")) {
                std::cout << "🛡️ Running risk analysis..." << std::endl;
            }
            
            // Market data simulation
            ImGui::Separator();
            ImGui::Text("📊 Live Market Data:");
            
            static double btc_price = 45000.0;
            static double eth_price = 3000.0;
            
            // Simulate price movement
            btc_price += (rand() % 200 - 100) * 0.1;
            eth_price += (rand() % 100 - 50) * 0.1;
            
            ImGui::Text("BTC/USD: $%.2f", btc_price);
            ImGui::Text("ETH/USD: $%.2f", eth_price);
            
            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 
                       1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
            ImGui::End();
        }
        
        // 3. Show another simple window
        if (show_another_window) {
            ImGui::Begin("Another Window", &show_another_window);
            ImGui::Text("Hello from another window!");
            if (ImGui::Button("Close Me"))
                show_another_window = false;
            ImGui::End();
        }
        
        // Rendering
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, 
                    clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        
        glfwSwapBuffers(window);
        
        // Small delay to prevent excessive CPU usage
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
    
    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    
    glfwDestroyWindow(window);
    glfwTerminate();
    
    std::cout << "🛑 MoneyBot GUI stopped cleanly" << std::endl;
    return 0;
}
