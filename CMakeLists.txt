cmake_minimum_required(VERSION 3.10)
project(MoneyBot)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Boost REQUIRED COMPONENTS system thread)
find_package(CURL REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(SQLite3 REQUIRED)
find_package(spdlog REQUIRED)
find_package(OpenGL REQUIRED)
find_package(PkgConfig REQUIRED)

pkg_search_module(GLFW REQUIRED glfw3)

include_directories(
    ${Boost_INCLUDE_DIRS}
    ${CURL_INCLUDE_DIRS}
    ${OPENSSL_INCLUDE_DIR}
    ${SQLite3_INCLUDE_DIRS}
    ${spdlog_INCLUDE_DIRS}
    include
    lib/json/include
    lib/imgui
    lib/imgui/backends
    ${GLFW_INCLUDE_DIRS}
)

add_executable(moneybot
    src/main.cpp
    src/moneybot.cpp
    src/network.cpp
    src/order_book.cpp
    src/logger.cpp
    src/data_analyzer.cpp
    src/types.cpp
    src/risk_manager.cpp
    src/market_maker_strategy.cpp
    src/order_manager.cpp
    src/backtest_engine.cpp
    src/strategy_factory.cpp
)

target_link_libraries(moneybot
    ${Boost_LIBRARIES}
    ${CURL_LIBRARIES}
    ${OPENSSL_LIBRARIES}
    ${SQLite3_LIBRARIES}
    spdlog::spdlog
)

add_custom_command(TARGET moneybot POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/logs
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/data
)

# --- ImGui/GLFW/OpenGL3 GUI build ---
# You must download ImGui to lib/imgui and GLFW to your system or as a submodule.
# This block assumes pkg-config for GLFW3 and OpenGL is available.

add_executable(moneybot_gui
    src/gui_main.cpp
    src/moneybot.cpp
    src/network.cpp
    src/order_book.cpp
    src/logger.cpp
    src/types.cpp
    src/risk_manager.cpp
    src/market_maker_strategy.cpp
    src/order_manager.cpp
)

# ImGui backend sources for OpenGL3+GLFW
set(IMGUI_BACKEND_SRC
    lib/imgui/imgui.cpp
    lib/imgui/imgui_draw.cpp
    lib/imgui/imgui_tables.cpp
    lib/imgui/imgui_widgets.cpp
    lib/imgui/imgui_demo.cpp
    lib/imgui/backends/imgui_impl_glfw.cpp
    lib/imgui/backends/imgui_impl_opengl3.cpp
)
target_sources(moneybot_gui PRIVATE ${IMGUI_BACKEND_SRC})

# Explicitly add Homebrew's lib path for GLFW
link_directories(/opt/homebrew/lib)

# Link with GLFW and other dependencies
# Try both the variable and the direct path for robustness
# (If you have a different version, adjust the .dylib name accordingly)
target_link_libraries(moneybot_gui
    /opt/homebrew/lib/libglfw.dylib
    OpenGL::GL
    pthread
    sqlite3
    ${Boost_LIBRARIES}
    ${CURL_LIBRARIES}
    ${OPENSSL_LIBRARIES}
    ${SQLite3_LIBRARIES}
    spdlog::spdlog
)

install(FILES include/dummy_strategy.h DESTINATION include)