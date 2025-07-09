#include "strategy_factory.h"
#include "market_maker_strategy.h"
#include "dummy_strategy.h"
#include "logger.h"
#include <stdexcept>

namespace moneybot {

std::shared_ptr<Strategy> createStrategyFromConfig(const nlohmann::json& config) {
    std::string type = config["strategy"]["type"].get<std::string>();
    
    if (type == "market_maker") {
        // Temporarily use dummy strategy to isolate the issue
        return std::make_shared<DummyStrategy>(config["strategy"]);
    } else if (type == "multi_asset") {
        // For multi-asset mode, we'll use a dummy strategy for now
        // In the future, this will create a Multi-Asset Strategy Manager
        // that coordinates multiple strategies across exchanges
        return std::make_shared<DummyStrategy>(config["strategy"]);
    }
    
    // Fallback: dummy strategy
    return std::make_shared<DummyStrategy>(config["strategy"]);
}

} // namespace moneybot
