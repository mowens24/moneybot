#include "gui/market_data_window.h"
#include "core/exchange_manager.h"
#include "types.h"
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <iostream>

namespace moneybot {
namespace gui {

MarketDataWindow::MarketDataWindow(std::shared_ptr<ExchangeManager> exchange_manager)
    : exchange_manager_(exchange_manager) {
    // Initialize chart data
    price_history_.reserve(100);
    volume_history_.reserve(100);
    
    // Initialize some demo data
    price_data_["BTCUSD"] = {45000.0, 1000.0, 2.5, std::chrono::system_clock::now()};
    price_data_["ETHUSD"] = {3000.0, 2000.0, 3.2, std::chrono::system_clock::now()};
    price_data_["ADAUSD"] = {0.85, 5000.0, -1.5, std::chrono::system_clock::now()};
    
    updatePriceHistory();
}

void MarketDataWindow::render() {
    if (!ImGui::Begin("📊 Market Data")) {
        ImGui::End();
        return;
    }
    
    // Update data first
    updateData();
    
    // Header
    ImGui::TextColored(ImVec4(0.0f, 1.0f, 1.0f, 1.0f), "Live Market Data");
    ImGui::Separator();
    
    // Symbol selector
    renderSymbolSelector();
    
    // Main content in tabs
    if (ImGui::BeginTabBar("MarketDataTabs")) {
        
        // Price Overview tab
        if (ImGui::BeginTabItem("Price Overview")) {
            renderPriceTable();
            ImGui::EndTabItem();
        }
        
        // Order Book tab
        if (ImGui::BeginTabItem("Order Book")) {
            renderOrderBook();
            ImGui::EndTabItem();
        }
        
        // Recent Trades tab
        if (ImGui::BeginTabItem("Recent Trades")) {
            renderRecentTrades();
            ImGui::EndTabItem();
        }
        
        // Price Chart tab
        if (ImGui::BeginTabItem("Price Chart")) {
            renderPriceChart();
            ImGui::EndTabItem();
        }
        
        ImGui::EndTabBar();
    }
    
    ImGui::End();
}

void MarketDataWindow::updateData() {
    if (!exchange_manager_) return;
    
    // Update price data periodically
    static auto last_update = std::chrono::steady_clock::now();
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - last_update).count();
    
    if (elapsed > 1) {
        updatePriceHistory();
        last_update = now;
    }
}

void MarketDataWindow::renderSymbolSelector() {
    ImGui::Text("Selected Symbol:");
    ImGui::SameLine();
    
    // Symbol combo box
    if (ImGui::BeginCombo("##SymbolSelect", selected_symbol_.c_str())) {
        for (const auto& [symbol, data] : price_data_) {
            bool is_selected = (selected_symbol_ == symbol);
            if (ImGui::Selectable(symbol.c_str(), is_selected)) {
                selected_symbol_ = symbol;
            }
            if (is_selected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
    
    ImGui::SameLine();
    ImGui::Text("Exchange:");
    ImGui::SameLine();
    
    // Exchange combo box
    if (ImGui::BeginCombo("##ExchangeSelect", selected_exchange_.c_str())) {
        std::vector<std::string> exchanges = {"binance", "coinbase", "kraken", "ftx"};
        for (const auto& exchange : exchanges) {
            bool is_selected = (selected_exchange_ == exchange);
            if (ImGui::Selectable(exchange.c_str(), is_selected)) {
                selected_exchange_ = exchange;
            }
            if (is_selected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
    
    ImGui::Separator();
}

void MarketDataWindow::renderPriceTable() {
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "💰 Price Overview");
    
    // Connection status
    ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "🟢 Connected");
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Last Update: %s", 
                       formatTime(std::chrono::system_clock::now()).c_str());
    
    // Price table
    if (ImGui::BeginTable("PriceTable", 6, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Sortable)) {
        ImGui::TableSetupColumn("Symbol", ImGuiTableColumnFlags_DefaultSort);
        ImGui::TableSetupColumn("Price", ImGuiTableColumnFlags_None);
        ImGui::TableSetupColumn("24h Change", ImGuiTableColumnFlags_None);
        ImGui::TableSetupColumn("24h Volume", ImGuiTableColumnFlags_None);
        ImGui::TableSetupColumn("Exchange", ImGuiTableColumnFlags_None);
        ImGui::TableSetupColumn("Time", ImGuiTableColumnFlags_None);
        ImGui::TableHeadersRow();
        
        // Handle sorting
        if (ImGuiTableSortSpecs* sort_specs = ImGui::TableGetSortSpecs()) {
            if (sort_specs->SpecsDirty) {
                // TODO: Implement sorting logic
                sort_specs->SpecsDirty = false;
            }
        }
        
        // Render price rows
        int row_id = 0;
        for (const auto& [symbol, data] : price_data_) {
            drawPriceRow(symbol, data, row_id++);
        }
        
        ImGui::EndTable();
    }
}

void MarketDataWindow::renderOrderBook() {
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "📋 Order Book - %s", selected_symbol_.c_str());
    
    // Generate mock order book data
    orderbook_bids_.clear();
    orderbook_asks_.clear();
    
    double mid_price = 45000.0;
    if (price_data_.find(selected_symbol_) != price_data_.end()) {
        mid_price = price_data_[selected_symbol_].price;
    }
    
    // Generate bids (below mid price)
    for (int i = 0; i < 10; ++i) {
        double price = mid_price - (i + 1) * 0.5;
        double size = (rand() % 1000) / 100.0;
        orderbook_bids_.push_back({price, size});
    }
    
    // Generate asks (above mid price)
    for (int i = 0; i < 10; ++i) {
        double price = mid_price + (i + 1) * 0.5;
        double size = (rand() % 1000) / 100.0;
        orderbook_asks_.push_back({price, size});
    }
    
    // Display order book
    ImGui::Columns(2, "OrderBook", true);
    
    // Asks (sell orders)
    ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Asks (Sell)");
    if (ImGui::BeginTable("AsksTable", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
        ImGui::TableSetupColumn("Price");
        ImGui::TableSetupColumn("Size");
        ImGui::TableHeadersRow();
        
        for (const auto& [price, size] : orderbook_asks_) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "%s", formatPrice(price).c_str());
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%.2f", size);
        }
        
        ImGui::EndTable();
    }
    
    ImGui::NextColumn();
    
    // Bids (buy orders)
    ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Bids (Buy)");
    if (ImGui::BeginTable("BidsTable", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
        ImGui::TableSetupColumn("Price");
        ImGui::TableSetupColumn("Size");
        ImGui::TableHeadersRow();
        
        for (const auto& [price, size] : orderbook_bids_) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "%s", formatPrice(price).c_str());
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%.2f", size);
        }
        
        ImGui::EndTable();
    }
    
    ImGui::Columns(1);
}

void MarketDataWindow::renderRecentTrades() {
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "📈 Recent Trades - %s", selected_symbol_.c_str());
    
    // Generate mock trade data
    recent_trades_.clear();
    double mid_price = 45000.0;
    if (price_data_.find(selected_symbol_) != price_data_.end()) {
        mid_price = price_data_[selected_symbol_].price;
    }
    
    for (int i = 0; i < 20; ++i) {
        double price = mid_price + ((rand() % 100) - 50) * 0.1;
        double size = (rand() % 1000) / 100.0;
        recent_trades_.push_back({price, size});
    }
    
    // Display trades table
    if (ImGui::BeginTable("TradesTable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
        ImGui::TableSetupColumn("Price");
        ImGui::TableSetupColumn("Size");
        ImGui::TableSetupColumn("Time");
        ImGui::TableHeadersRow();
        
        int trade_id = 0;
        for (const auto& [price, size] : recent_trades_) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%s", formatPrice(price).c_str());
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%.2f", size);
            ImGui::TableSetColumnIndex(2);
            ImGui::Text("%02d:%02d:%02d", 
                       (int)(std::chrono::system_clock::now().time_since_epoch().count() / 3600) % 24,
                       (int)(std::chrono::system_clock::now().time_since_epoch().count() / 60) % 60,
                       (int)(std::chrono::system_clock::now().time_since_epoch().count()) % 60);
            ++trade_id;
        }
        
        ImGui::EndTable();
    }
}

void MarketDataWindow::renderPriceChart() {
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "📊 Price Chart - %s", selected_symbol_.c_str());
    
    if (price_history_.empty()) {
        ImGui::Text("No price data available");
        return;
    }
    
    // Price chart
    ImGui::Text("Price History");
    ImGui::PlotLines("##PriceChart", price_history_.data(), price_history_.size(), 0, nullptr, 
                     FLT_MAX, FLT_MAX, ImVec2(0, 200));
    
    // Volume chart
    ImGui::Text("Volume History");
    ImGui::PlotHistogram("##VolumeChart", volume_history_.data(), volume_history_.size(), 0, nullptr, 
                         FLT_MAX, FLT_MAX, ImVec2(0, 150));
}

void MarketDataWindow::drawPriceRow(const std::string& symbol, const PriceData& data, int row_id) {
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::Text("%s", symbol.c_str());
    
    ImGui::TableSetColumnIndex(1);
    ImGui::Text("%s", formatPrice(data.price).c_str());
    
    ImGui::TableSetColumnIndex(2);
    ImGui::TextColored(getPriceChangeColor(data.change_24h), "%+.2f%%", data.change_24h);
    
    ImGui::TableSetColumnIndex(3);
    ImGui::Text("%s", formatVolume(data.volume).c_str());
    
    ImGui::TableSetColumnIndex(4);
    ImGui::Text("%s", selected_exchange_.c_str());
    
    ImGui::TableSetColumnIndex(5);
    ImGui::Text("%s", formatTime(data.timestamp).c_str());
}

void MarketDataWindow::updatePriceHistory() {
    if (price_data_.find(selected_symbol_) == price_data_.end()) return;
    
    const auto& data = price_data_[selected_symbol_];
    
    // Add new data points
    price_history_.push_back(static_cast<float>(data.price));
    volume_history_.push_back(static_cast<float>(data.volume));
    
    // Keep only last 100 data points
    if (price_history_.size() > 100) {
        price_history_.erase(price_history_.begin());
    }
    if (volume_history_.size() > 100) {
        volume_history_.erase(volume_history_.begin());
    }
    
    // Simulate price changes
    for (auto& [symbol, price_data] : price_data_) {
        double change = ((rand() % 200) - 100) / 10000.0; // ±1% change
        price_data.price *= (1.0 + change);
        price_data.change_24h += change * 100;
        price_data.timestamp = std::chrono::system_clock::now();
    }
}

ImVec4 MarketDataWindow::getPriceChangeColor(double change) const {
    if (change > 0) {
        return ImVec4(0.0f, 1.0f, 0.0f, 1.0f); // Green for positive
    } else if (change < 0) {
        return ImVec4(1.0f, 0.0f, 0.0f, 1.0f); // Red for negative
    } else {
        return ImVec4(1.0f, 1.0f, 1.0f, 1.0f); // White for neutral
    }
}

std::string MarketDataWindow::formatPrice(double price, int decimals) const {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(decimals) << price;
    return oss.str();
}

std::string MarketDataWindow::formatVolume(double volume) const {
    if (volume >= 1000000) {
        return formatPrice(volume / 1000000, 1) + "M";
    } else if (volume >= 1000) {
        return formatPrice(volume / 1000, 1) + "K";
    } else {
        return formatPrice(volume, 0);
    }
}

std::string MarketDataWindow::formatTime(const std::chrono::system_clock::time_point& time) const {
    auto time_t = std::chrono::system_clock::to_time_t(time);
    auto tm = *std::localtime(&time_t);
    std::ostringstream oss;
    oss << std::put_time(&tm, "%H:%M:%S");
    return oss.str();
}

} // namespace gui
} // namespace moneybot
