#include "strategy/triangle_arbitrage_strategy.h"
#include <cmath>
#include <iostream>
#include <algorithm>

namespace moneybot {

TriangleArbitrageStrategy::TriangleArbitrageStrategy(const std::string& name, 
                                                     const nlohmann::json& config,
                                                     std::shared_ptr<Logger> logger)
    : BaseStrategy(name, config, logger),
      min_profit_percentage_(0.1),
      max_position_size_(1000.0),
      max_execution_time_ms_(5000.0),
      price_staleness_threshold_ms_(1000.0),
      enable_execution_(false),
      opportunities_found_(0),
      opportunities_executed_(0),
      successful_arbitrages_(0),
      total_arbitrage_profit_(0.0),
      opportunity_check_interval_(std::chrono::milliseconds(100)) {
    
    // Load configuration
    min_profit_percentage_ = getConfigValue<double>("min_profit_percentage", 0.1);
    max_position_size_ = getConfigValue<double>("max_position_size", 1000.0);
    max_execution_time_ms_ = getConfigValue<double>("max_execution_time_ms", 5000.0);
    price_staleness_threshold_ms_ = getConfigValue<double>("price_staleness_threshold_ms", 1000.0);
    enable_execution_ = getConfigValue<bool>("enable_execution", false);
    
    std::cout << "Triangle Arbitrage Strategy '" << name << "' created" << std::endl;
    std::cout << "Min profit: " << min_profit_percentage_ << "%" << std::endl;
    std::cout << "Execution enabled: " << (enable_execution_ ? "Yes" : "No") << std::endl;
}

bool TriangleArbitrageStrategy::initialize() {
    // Add default triangle pairs
    addTrianglePair("BTCUSD", "ETHUSD", "BTCETH");
    addTrianglePair("BTCUSD", "ADAUSD", "BTCADA");
    addTrianglePair("ETHUSD", "ADAUSD", "ETHADA");
    
    setStatus(StrategyStatus::STOPPED);
    std::cout << "Triangle Arbitrage Strategy initialized with " 
              << triangle_pairs_.size() << " triangle pairs" << std::endl;
    
    return true;
}

void TriangleArbitrageStrategy::start() {
    if (getStatus() == StrategyStatus::RUNNING) {
        std::cout << "Triangle Arbitrage Strategy already running" << std::endl;
        return;
    }
    
    setStatus(StrategyStatus::STARTING);
    
    // Reset statistics
    {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        opportunities_found_ = 0;
        opportunities_executed_ = 0;
        successful_arbitrages_ = 0;
        total_arbitrage_profit_ = 0.0;
    }
    
    setStatus(StrategyStatus::RUNNING);
    last_opportunity_check_ = std::chrono::system_clock::now();
    
    std::cout << "Triangle Arbitrage Strategy started" << std::endl;
}

void TriangleArbitrageStrategy::stop() {
    if (getStatus() == StrategyStatus::STOPPED) {
        return;
    }
    
    setStatus(StrategyStatus::STOPPING);
    
    // Cancel all pending arbitrages
    {
        std::lock_guard<std::mutex> lock(arbitrage_mutex_);
        for (auto& [id, pending] : pending_arbitrages_) {
            cancelPendingArbitrage(id);
        }
        pending_arbitrages_.clear();
    }
    
    setStatus(StrategyStatus::STOPPED);
    std::cout << "Triangle Arbitrage Strategy stopped" << std::endl;
}

void TriangleArbitrageStrategy::pause() {
    if (getStatus() == StrategyStatus::RUNNING) {
        setStatus(StrategyStatus::STOPPED);
        std::cout << "Triangle Arbitrage Strategy paused" << std::endl;
    }
}

void TriangleArbitrageStrategy::resume() {
    if (getStatus() == StrategyStatus::STOPPED) {
        setStatus(StrategyStatus::RUNNING);
        std::cout << "Triangle Arbitrage Strategy resumed" << std::endl;
    }
}

void TriangleArbitrageStrategy::onTick(const std::string& symbol, const Trade& tick) {
    if (getStatus() != StrategyStatus::RUNNING) {
        return;
    }
    
    // Update price
    updatePrice(symbol, tick.price);
    
    // Check for opportunities periodically
    auto now = std::chrono::system_clock::now();
    if (now - last_opportunity_check_ >= opportunity_check_interval_) {
        auto opportunities = findOpportunities();
        
        if (!opportunities.empty()) {
            std::lock_guard<std::mutex> lock(stats_mutex_);
            opportunities_found_ += opportunities.size();
            
            std::cout << "Found " << opportunities.size() << " triangle arbitrage opportunities" << std::endl;
            
            // Execute the most profitable opportunity
            if (enable_execution_) {
                auto best_opportunity = *std::max_element(opportunities.begin(), opportunities.end(),
                    [](const TriangleOpportunity& a, const TriangleOpportunity& b) {
                        return a.profit_percentage < b.profit_percentage;
                    });
                
                if (executeArbitrage(best_opportunity)) {
                    opportunities_executed_++;
                }
            }
        }
        
        last_opportunity_check_ = now;
    }
}

void TriangleArbitrageStrategy::onOrderFill(const OrderFill& fill) {
    std::lock_guard<std::mutex> lock(arbitrage_mutex_);
    
    // Find the pending arbitrage for this fill
    for (auto& [id, pending] : pending_arbitrages_) {
        auto it = std::find(pending.order_ids.begin(), pending.order_ids.end(), fill.order_id);
        if (it != pending.order_ids.end()) {
            std::cout << "Order fill for arbitrage " << id << " stage " << pending.stage << std::endl;
            
            // Move to next stage
            pending.stage++;
            
            if (pending.stage < 3) {
                // Execute next stage
                executeArbitrageStage(id, pending.stage);
            } else {
                // Arbitrage complete
                std::cout << "Arbitrage " << id << " completed" << std::endl;
                
                std::lock_guard<std::mutex> stats_lock(stats_mutex_);
                successful_arbitrages_++;
                total_arbitrage_profit_ += pending.expected_profit;
                
                pending_arbitrages_.erase(id);
            }
            break;
        }
    }
}

void TriangleArbitrageStrategy::onOrderReject(const OrderReject& reject) {
    std::lock_guard<std::mutex> lock(arbitrage_mutex_);
    
    // Find and cancel the pending arbitrage
    for (auto& [id, pending] : pending_arbitrages_) {
        auto it = std::find(pending.order_ids.begin(), pending.order_ids.end(), reject.order_id);
        if (it != pending.order_ids.end()) {
            std::cout << "Order rejected for arbitrage " << id << ": " << reject.reason << std::endl;
            cancelPendingArbitrage(id);
            pending_arbitrages_.erase(id);
            break;
        }
    }
}

void TriangleArbitrageStrategy::onBalanceUpdate(const Balance& balance) {
    // Update available balance for arbitrage calculations
    std::cout << "Balance update: " << balance.asset << " = " << balance.total << std::endl;
}

void TriangleArbitrageStrategy::onPositionUpdate(const Position& position) {
    // Monitor position changes
    std::cout << "Position update: " << position.symbol << " = " << position.quantity << std::endl;
}

void TriangleArbitrageStrategy::addTrianglePair(const std::string& symbol_a, 
                                                const std::string& symbol_b, 
                                                const std::string& symbol_c) {
    triangle_pairs_.emplace_back(symbol_a, symbol_b, symbol_c);
    std::cout << "Added triangle pair: " << symbol_a << " / " << symbol_b << " / " << symbol_c << std::endl;
}

std::vector<TriangleOpportunity> TriangleArbitrageStrategy::findOpportunities() {
    std::vector<TriangleOpportunity> opportunities;
    
    std::lock_guard<std::mutex> lock(prices_mutex_);
    
    for (const auto& pair : triangle_pairs_) {
        if (hasRecentPrices(pair)) {
            auto opportunity = calculateTriangleArbitrage(pair);
            if (isOpportunityValid(opportunity)) {
                opportunities.push_back(opportunity);
            }
        }
    }
    
    return opportunities;
}

TriangleOpportunity TriangleArbitrageStrategy::calculateTriangleArbitrage(const TrianglePair& pair) {
    TriangleOpportunity opportunity;
    opportunity.symbol_a = pair.symbol_a;
    opportunity.symbol_b = pair.symbol_b;
    opportunity.symbol_c = pair.symbol_c;
    opportunity.timestamp = std::chrono::system_clock::now();
    
    // Get current prices
    auto it_a = latest_prices_.find(pair.symbol_a);
    auto it_b = latest_prices_.find(pair.symbol_b);
    auto it_c = latest_prices_.find(pair.symbol_c);
    
    if (it_a != latest_prices_.end() && it_b != latest_prices_.end() && it_c != latest_prices_.end()) {
        opportunity.price_a = it_a->second;
        opportunity.price_b = it_b->second;
        opportunity.price_c = it_c->second;
        
        // Calculate arbitrage profit
        // For triangle A/USD, B/USD, A/B
        // Buy A with USD, sell A for B, sell B for USD
        // profit = (price_a / price_c) * price_b - price_a
        
        double theoretical_price = (opportunity.price_a / opportunity.price_c) * opportunity.price_b;
        double profit = theoretical_price - opportunity.price_a;
        opportunity.profit_percentage = (profit / opportunity.price_a) * 100.0;
        
        // Calculate maximum quantity (simplified)
        opportunity.max_quantity = std::min(max_position_size_, 
                                          std::min(opportunity.price_a, opportunity.price_b));
        
        opportunity.is_profitable = opportunity.profit_percentage > min_profit_percentage_;
    }
    
    return opportunity;
}

bool TriangleArbitrageStrategy::isOpportunityValid(const TriangleOpportunity& opportunity) {
    return opportunity.is_profitable && 
           opportunity.max_quantity > 0 && 
           opportunity.profit_percentage < 50.0; // Sanity check
}

bool TriangleArbitrageStrategy::executeArbitrage(const TriangleOpportunity& opportunity) {
    if (!enable_execution_) {
        std::cout << "Execution disabled - would execute arbitrage with " 
                  << opportunity.profit_percentage << "% profit" << std::endl;
        return false;
    }
    
    // Generate unique ID for this arbitrage
    std::string arbitrage_id = "ARB_" + std::to_string(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count());
    
    std::lock_guard<std::mutex> lock(arbitrage_mutex_);
    
    // Create pending arbitrage
    pending_arbitrages_.emplace(arbitrage_id, PendingArbitrage(arbitrage_id, opportunity.profit_percentage));
    
    // Start first stage
    executeArbitrageStage(arbitrage_id, 0);
    
    std::cout << "Started arbitrage " << arbitrage_id << " with expected profit: " 
              << opportunity.profit_percentage << "%" << std::endl;
    
    return true;
}

void TriangleArbitrageStrategy::executeArbitrageStage(const std::string& opportunity_id, int stage) {
    auto it = pending_arbitrages_.find(opportunity_id);
    if (it == pending_arbitrages_.end()) {
        return;
    }
    
    auto& pending = it->second;
    
    // Create and place order for this stage
    Order order;
    order.client_order_id = opportunity_id + "_" + std::to_string(stage);
    order.timestamp = std::chrono::system_clock::now();
    
    switch (stage) {
        case 0: {
            // Stage 0: Buy A with USD
            order.symbol = "BTCUSD"; // Simplified
            order.side = OrderSide::BUY;
            order.type = OrderType::MARKET;
            order.quantity = 0.01; // Simplified
            break;
        }
        case 1: {
            // Stage 1: Sell A for B
            order.symbol = "BTCETH"; // Simplified
            order.side = OrderSide::SELL;
            order.type = OrderType::MARKET;
            order.quantity = 0.01; // Simplified
            break;
        }
        case 2: {
            // Stage 2: Sell B for USD
            order.symbol = "ETHUSD"; // Simplified
            order.side = OrderSide::SELL;
            order.type = OrderType::MARKET;
            order.quantity = 0.01; // Simplified
            break;
        }
    }
    
    if (placeOrder(order)) {
        pending.order_ids.push_back(order.client_order_id);
        std::cout << "Placed order for arbitrage " << opportunity_id << " stage " << stage << std::endl;
    } else {
        std::cout << "Failed to place order for arbitrage " << opportunity_id << " stage " << stage << std::endl;
        cancelPendingArbitrage(opportunity_id);
    }
}

void TriangleArbitrageStrategy::cancelPendingArbitrage(const std::string& opportunity_id) {
    auto it = pending_arbitrages_.find(opportunity_id);
    if (it != pending_arbitrages_.end()) {
        // Cancel all pending orders
        for (const auto& order_id : it->second.order_ids) {
            cancelOrder(order_id);
        }
        std::cout << "Cancelled pending arbitrage " << opportunity_id << std::endl;
    }
}

void TriangleArbitrageStrategy::updatePrice(const std::string& symbol, double price) {
    std::lock_guard<std::mutex> lock(prices_mutex_);
    
    latest_prices_[symbol] = price;
    price_timestamps_[symbol] = std::chrono::system_clock::now();
    
    // Update triangle pair prices
    for (auto& pair : triangle_pairs_) {
        if (pair.symbol_a == symbol) {
            pair.last_price_a = price;
            pair.last_update_a = std::chrono::system_clock::now();
        } else if (pair.symbol_b == symbol) {
            pair.last_price_b = price;
            pair.last_update_b = std::chrono::system_clock::now();
        } else if (pair.symbol_c == symbol) {
            pair.last_price_c = price;
            pair.last_update_c = std::chrono::system_clock::now();
        }
    }
}

bool TriangleArbitrageStrategy::hasRecentPrices(const TrianglePair& pair) {
    auto now = std::chrono::system_clock::now();
    auto threshold = std::chrono::milliseconds(static_cast<int>(price_staleness_threshold_ms_));
    
    return (now - pair.last_update_a < threshold) &&
           (now - pair.last_update_b < threshold) &&
           (now - pair.last_update_c < threshold) &&
           (pair.last_price_a > 0) &&
           (pair.last_price_b > 0) &&
           (pair.last_price_c > 0);
}

} // namespace moneybot
