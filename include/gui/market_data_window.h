#pragma once

#include <memory>
#include <vector>
#include <string>
#include <map>
#include <chrono>
#include <imgui.h>

namespace moneybot {

// Forward declarations
class ExchangeManager;
struct MarketData;
struct TickData;

namespace gui {

struct PriceData {
    double price = 0.0;
    double volume = 0.0;
    double change_24h = 0.0;
    std::chrono::system_clock::time_point timestamp;
};

class MarketDataWindow {
private:
    std::shared_ptr<ExchangeManager> exchange_manager_;
    
    // UI state
    bool show_price_table_ = true;
    bool show_orderbook_ = true;
    bool show_trades_ = true;
    bool show_charts_ = true;
    
    // Selected symbol for detailed view
    std::string selected_symbol_ = "BTCUSD";
    std::string selected_exchange_ = "binance";
    
    // Data storage
    std::map<std::string, PriceData> price_data_;
    std::vector<std::pair<double, double>> orderbook_bids_;
    std::vector<std::pair<double, double>> orderbook_asks_;
    std::vector<std::pair<double, double>> recent_trades_;
    
    // Chart data
    std::vector<float> price_history_;
    std::vector<float> volume_history_;
    
    // Table sorting
    int price_sort_column_ = 0;
    bool price_sort_ascending_ = false;
    
public:
    explicit MarketDataWindow(std::shared_ptr<ExchangeManager> exchange_manager);
    ~MarketDataWindow() = default;
    
    void render();
    void updateData();
    
    // Data update callbacks
    void onMarketData(const MarketData& data);
    void onTickData(const TickData& tick);
    
private:
    void renderPriceTable();
    void renderOrderBook();
    void renderRecentTrades();
    void renderPriceChart();
    void renderSymbolSelector();
    
    // Helper functions
    void drawPriceRow(const std::string& symbol, const PriceData& data, int row_id);
    void updatePriceHistory();
    ImVec4 getPriceChangeColor(double change) const;
    
    // Formatting helpers
    std::string formatPrice(double price, int decimals = 2) const;
    std::string formatVolume(double volume) const;
    std::string formatTime(const std::chrono::system_clock::time_point& time) const;
};

} // namespace gui
} // namespace moneybot
