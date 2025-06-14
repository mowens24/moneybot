#include "moneybot.h"
#include "logger.h"
#include "data_analyzer.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>

using json = nlohmann::json;

int main(int argc, char* argv[]) {
    bool analyze_mode = (argc > 1 && std::string(argv[1]) == "--analyze");

    std::ifstream config_file("config.json");
    json config;
    config_file >> config;

    if (analyze_mode) {
        auto logger = std::make_shared<moneybot::Logger>();
        moneybot::DataAnalyzer analyzer(logger);
        std::cout << "Average Spread: " << analyzer.computeAverageSpread() << std::endl;
        std::cout << "VWAP (Bids): " << analyzer.computeVWAP(true) << std::endl;
        auto ticks = analyzer.getTicksByTime("BTCUSDT", 0, std::numeric_limits<int64_t>::max());
        for (const auto& [ts, price, qty] : ticks) {
            std::cout << "Time: " << ts << ", Price: " << price << ", Qty: " << qty << std::endl;
        }
        return 0;
    }

    moneybot::TradingEngine engine(config);
    engine.start();
    std::this_thread::sleep_for(std::chrono::seconds(30)); // Collect data for 30s
    engine.stop();

    return 0;
}