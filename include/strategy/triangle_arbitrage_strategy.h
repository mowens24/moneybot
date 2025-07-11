#ifndef TRIANGLE_ARBITRAGE_STRATEGY_H
#define TRIANGLE_ARBITRAGE_STRATEGY_H

#include "strategy/strategy_engine.h"
#include <unordered_map>
#include <vector>
#include <chrono>

namespace moneybot {

struct TriangleOpportunity {
    std::string symbol_a;  // e.g., "BTCUSD"
    std::string symbol_b;  // e.g., "ETHUSD"
    std::string symbol_c;  // e.g., "BTCETH"
    
    double price_a;
    double price_b;
    double price_c;
    
    double profit_percentage;
    double max_quantity;
    
    std::chrono::system_clock::time_point timestamp;
    bool is_profitable;
    
    TriangleOpportunity() : price_a(0), price_b(0), price_c(0), 
                           profit_percentage(0), max_quantity(0), is_profitable(false) {}
};

class TriangleArbitrageStrategy : public BaseStrategy {
public:
    TriangleArbitrageStrategy(const std::string& name, const nlohmann::json& config,
                              std::shared_ptr<Logger> logger);
    
    // Strategy lifecycle
    bool initialize() override;
    void start() override;
    void stop() override;
    void pause() override;
    void resume() override;
    
    // Strategy execution
    void onTick(const std::string& symbol, const Trade& tick) override;
    void onOrderFill(const OrderFill& fill) override;
    void onOrderReject(const OrderReject& reject) override;
    void onBalanceUpdate(const Balance& balance) override;
    void onPositionUpdate(const Position& position) override;
    
    // Triangle arbitrage specific
    void addTrianglePair(const std::string& symbol_a, const std::string& symbol_b, 
                        const std::string& symbol_c);
    std::vector<TriangleOpportunity> findOpportunities();
    
private:
    struct TrianglePair {
        std::string symbol_a;
        std::string symbol_b;
        std::string symbol_c;
        
        double last_price_a;
        double last_price_b;
        double last_price_c;
        
        std::chrono::system_clock::time_point last_update_a;
        std::chrono::system_clock::time_point last_update_b;
        std::chrono::system_clock::time_point last_update_c;
        
        TrianglePair(const std::string& a, const std::string& b, const std::string& c)
            : symbol_a(a), symbol_b(b), symbol_c(c), 
              last_price_a(0), last_price_b(0), last_price_c(0) {}
    };
    
    struct PendingArbitrage {
        std::string opportunity_id;
        std::vector<std::string> order_ids;
        std::chrono::system_clock::time_point start_time;
        double expected_profit;
        int stage; // 0, 1, 2 for the three legs
        
        PendingArbitrage(const std::string& id, double profit)
            : opportunity_id(id), expected_profit(profit), stage(0) {
            start_time = std::chrono::system_clock::now();
        }
    };
    
    // Triangle calculation
    TriangleOpportunity calculateTriangleArbitrage(const TrianglePair& pair);
    bool isOpportunityValid(const TriangleOpportunity& opportunity);
    
    // Execution
    bool executeArbitrage(const TriangleOpportunity& opportunity);
    void executeArbitrageStage(const std::string& opportunity_id, int stage);
    void cancelPendingArbitrage(const std::string& opportunity_id);
    
    // Data management
    void updatePrice(const std::string& symbol, double price);
    bool hasRecentPrices(const TrianglePair& pair);
    
    // Configuration
    double min_profit_percentage_;
    double max_position_size_;
    double max_execution_time_ms_;
    double price_staleness_threshold_ms_;
    bool enable_execution_;
    
    // State
    std::vector<TrianglePair> triangle_pairs_;
    std::unordered_map<std::string, double> latest_prices_;
    std::unordered_map<std::string, std::chrono::system_clock::time_point> price_timestamps_;
    std::unordered_map<std::string, PendingArbitrage> pending_arbitrages_;
    
    // Statistics
    int opportunities_found_;
    int opportunities_executed_;
    int successful_arbitrages_;
    double total_arbitrage_profit_;
    
    // Thread safety
    mutable std::mutex prices_mutex_;
    mutable std::mutex arbitrage_mutex_;
    mutable std::mutex stats_mutex_;
    
    // Timing
    std::chrono::system_clock::time_point last_opportunity_check_;
    std::chrono::milliseconds opportunity_check_interval_;
};

} // namespace moneybot

#endif // TRIANGLE_ARBITRAGE_STRATEGY_H
