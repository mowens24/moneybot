#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <GLFW/glfw3.h>
#include <memory>
#include <iostream>

// Include the new modular GUI
#include "gui/main_window.h"

int gui_main_new(int argc, char** argv) {
    // Setup GLFW window
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return 1;
    }
    
    // Create window for the trading dashboard
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    
    GLFWwindow* window = glfwCreateWindow(1920, 1080, "MoneyBot - Modular Trading Dashboard", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
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
    
    // Initialize the main window with null pointers for now (just testing the GUI)
    auto main_window = std::make_unique<moneybot::gui::MainWindow>();
    main_window->initialize(nullptr, nullptr, nullptr, nullptr);
    
    std::cout << "🚀 MoneyBot Modular GUI Started!" << std::endl;
    
    // Main loop
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        
        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        
        // Simple test window
        ImGui::Begin("MoneyBot - Modular GUI Test");
        ImGui::Text("✅ Modular GUI Framework Working!");
        ImGui::Text("📊 Portfolio Window: Ready");
        ImGui::Text("📈 Market Data Window: Ready");
        ImGui::Text("🤖 Algorithm Window: Ready");
        ImGui::Text("⚠️ Risk Window: Ready");
        ImGui::Text("🔄 Exchange Window: Ready");
        ImGui::Separator();
        ImGui::Text("This is a test of the new modular GUI system.");
        ImGui::Text("Each window is now implemented as a separate class.");
        ImGui::End();
        
        // Render the main window (it will handle null pointers gracefully)
        try {
            main_window->render();
        } catch (const std::exception& e) {
            std::cerr << "Error in main window render: " << e.what() << std::endl;
        }
        
        // Show demo window for testing
        static bool show_demo = true;
        if (show_demo) {
            ImGui::ShowDemoWindow(&show_demo);
        }
        
        // Rendering
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        
        glfwSwapBuffers(window);
    }
    
    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    
    glfwDestroyWindow(window);
    glfwTerminate();
    
    return 0;
}
