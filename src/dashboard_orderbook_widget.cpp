#include "dashboard_orderbook_widget.h"
#include <imgui.h>
#include <algorithm>
#include <iomanip>
#include <sstream>

namespace moneybot {

// Helper to format double with fixed decimals
static std::string format_double(double value, int decimals = 2) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(decimals) << value;
    return oss.str();
}

void RenderOrderBookWidget(const std::vector<OrderBookEntry>& bids, const std::vector<OrderBookEntry>& asks) {
    ImGui::BeginChild("Order Book", ImVec2(0, 300), true);
    ImGui::Text("Order Book");
    ImGui::Separator();
    ImGui::Columns(3, nullptr, false);
    ImGui::Text("Bid Size"); ImGui::NextColumn();
    ImGui::Text("Price"); ImGui::NextColumn();
    ImGui::Text("Ask Size"); ImGui::NextColumn();
    ImGui::Separator();

    size_t max_rows = std::max(bids.size(), asks.size());
    for (size_t i = 0; i < max_rows; ++i) {
        // Bid size
        if (i < bids.size())
            ImGui::Text("%s", format_double(bids[i].size).c_str());
        else
            ImGui::Text("");
        ImGui::NextColumn();
        // Price (show bid price if available, else ask price)
        if (i < bids.size())
            ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.2f, 1.0f), "%s", format_double(bids[i].price, 2).c_str());
        else if (i < asks.size())
            ImGui::TextColored(ImVec4(0.8f, 0.2f, 0.2f, 1.0f), "%s", format_double(asks[i].price, 2).c_str());
        else
            ImGui::Text("");
        ImGui::NextColumn();
        // Ask size
        if (i < asks.size())
            ImGui::Text("%s", format_double(asks[i].size).c_str());
        else
            ImGui::Text("");
        ImGui::NextColumn();
    }
    ImGui::Columns(1);
    ImGui::EndChild();
}

} // namespace moneybot