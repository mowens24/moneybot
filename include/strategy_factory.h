#pragma once
#include "strategy.h"
#include <nlohmann/json.hpp>
#include <memory>
#include <string>

namespace moneybot {

std::shared_ptr<Strategy> createStrategyFromConfig(const nlohmann::json& config);

} // namespace moneybot
