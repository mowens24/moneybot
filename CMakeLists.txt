cmake_minimum_required(VERSION 3.10)
project(MoneyBot)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Boost REQUIRED COMPONENTS system thread)
find_package(CURL REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(SQLite3 REQUIRED)
find_package(spdlog REQUIRED)

include_directories(
    ${Boost_INCLUDE_DIRS}
    ${CURL_INCLUDE_DIRS}
    ${OPENSSL_INCLUDE_DIR}
    ${SQLite3_INCLUDE_DIRS}
    ${spdlog_INCLUDE_DIRS}
    include
    lib/json/include
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