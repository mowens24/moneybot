# --- Force Apple Clang on macOS ---

# --- Force Apple Clang and set PTY include path on macOS ---
if(APPLE)
    # Find Apple Clang (not Homebrew LLVM)
    execute_process(
        COMMAND xcrun --find clang
        OUTPUT_VARIABLE APPLE_CLANG_PATH
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    execute_process(
        COMMAND xcrun --find clang++
        OUTPUT_VARIABLE APPLE_CLANGXX_PATH
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    if(APPLE_CLANG_PATH AND APPLE_CLANGXX_PATH)
        set(CMAKE_C_COMPILER "${APPLE_CLANG_PATH}" CACHE STRING "Apple Clang C compiler" FORCE)
        set(CMAKE_CXX_COMPILER "${APPLE_CLANGXX_PATH}" CACHE STRING "Apple Clang C++ compiler" FORCE)
        message(STATUS "Forcing Apple Clang: ${APPLE_CLANG_PATH}, ${APPLE_CLANGXX_PATH}")
    else()
        message(FATAL_ERROR "Apple Clang not found via xcrun!")
    endif()
    # Set macOS SDK sysroot for system headers (e.g., pty.h)
    execute_process(
        COMMAND xcrun --sdk macosx --show-sdk-path
        OUTPUT_VARIABLE MACOS_SDK_PATH
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    if(MACOS_SDK_PATH)
        set(CMAKE_OSX_SYSROOT "${MACOS_SDK_PATH}" CACHE STRING "macOS SDK root" FORCE)
        message(STATUS "Setting macOS SDK sysroot: ${MACOS_SDK_PATH}")
    else()
        message(WARNING "Could not determine macOS SDK path for sysroot!")
    endif()

    # On macOS, we need to link against the frameworks
    link_directories(/System/Library/Frameworks)
    link_directories(/usr/local/lib)
endif()

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
    src/gui_main.cpp
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
    src/dashboard_metrics_widget.cpp
    src/dashboard_orderbook_widget.cpp
    # ImGui core files
    lib/imgui/imgui.cpp
    lib/imgui/imgui_demo.cpp
    lib/imgui/imgui_draw.cpp
    lib/imgui/imgui_tables.cpp
    lib/imgui/imgui_widgets.cpp
    # ImGui backends for GLFW and OpenGL3
    lib/imgui/backends/imgui_impl_glfw.cpp
    lib/imgui/backends/imgui_impl_opengl3.cpp
)

# --- Workaround for missing pty.h on macOS ---
if(APPLE)
    # Always add our local compat header for pty.h
    target_include_directories(moneybot PRIVATE ${CMAKE_SOURCE_DIR}/include/compat)
    # Silence OpenGL deprecation warnings on macOS
    target_compile_definitions(moneybot PRIVATE GL_SILENCE_DEPRECATION)
    message(STATUS "Added ${CMAKE_SOURCE_DIR}/include/compat to moneybot for pty.h workaround")
endif()

# Explicitly add Homebrew's lib path for GLFW
link_directories(/opt/homebrew/lib)

# Link with all dependencies (console + GUI)
if(APPLE)
    target_link_libraries(moneybot
        ${Boost_LIBRARIES}
        ${CURL_LIBRARIES}
        ${OPENSSL_LIBRARIES}
        ${SQLite3_LIBRARIES}
        spdlog::spdlog
        "-framework OpenGL"
        "-framework Cocoa"
        "-framework IOKit"
        "-framework CoreVideo"
        "/opt/homebrew/lib/libglfw.dylib"
    )
else()
    target_link_libraries(moneybot
        ${Boost_LIBRARIES}
        ${CURL_LIBRARIES}
        ${OPENSSL_LIBRARIES}
        ${SQLite3_LIBRARIES}
        spdlog::spdlog
        ${OPENGL_LIBRARIES}
        ${GLFW_LIBRARIES}
    )
endif()

add_custom_command(TARGET moneybot POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/logs
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/data
    COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_SOURCE_DIR}/config.json
            ${CMAKE_BINARY_DIR}/config.json
)

install(FILES include/dummy_strategy.h DESTINATION include)