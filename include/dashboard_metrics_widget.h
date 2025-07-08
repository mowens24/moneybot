#pragma once
#include <nlohmann/json.hpp>

namespace moneybot {

// Renders the metrics widget section of the dashboard
// metrics: JSON containing performance metrics like PnL, position, etc.
// status: JSON containing connection status, risk checks, etc.
void RenderMetricsWidget(const nlohmann::json& metrics, const nlohmann::json& status);

} // namespace moneybot
