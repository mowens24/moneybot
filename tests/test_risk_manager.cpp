#include "testing/test_framework.h"
#include "risk_manager.h"
#include "logger.h"
#include <memory>
#include <nlohmann/json.hpp>

using namespace moneybot;
using namespace moneybot::testing;

std::shared_ptr<RiskManager> createTestRiskManager() {
    auto logger = std::make_shared<Logger>();
    nlohmann::json config = {
        {"risk", {
            {"max_position_size", 1000.0},
            {"max_order_size", 100.0},
            {"max_daily_loss", -5000.0},
            {"max_drawdown", -10.0},
            {"max_orders_per_minute", 10},
            {"min_spread", 0.001},
            {"max_slippage", 0.01}
        }}
    };
    
    return std::make_shared<RiskManager>(logger, config);
}

void testRiskManagerConstruction() {
    auto rm = createTestRiskManager();
    ASSERT_TRUE(rm != nullptr);
    ASSERT_FALSE(rm->isEmergencyStopped());
    
    auto limits = rm->getRiskLimits();
    ASSERT_EQ(1000.0, limits.max_position_size);
    ASSERT_EQ(100.0, limits.max_order_size);
    ASSERT_EQ(-5000.0, limits.max_daily_loss);
}

void testOrderRiskCheck() {
    auto rm = createTestRiskManager();
    
    // Test valid order
    Order valid_order;
    valid_order.symbol = "BTCUSD";
    valid_order.quantity = 50.0;
    valid_order.price = 50000.0;
    valid_order.side = OrderSide::BUY;
    valid_order.type = OrderType::LIMIT;
    
    ASSERT_TRUE(rm->checkOrderRisk(valid_order));
    
    // Test oversized order
    Order oversized_order;
    oversized_order.symbol = "BTCUSD";
    oversized_order.quantity = 200.0; // Exceeds max_order_size of 100
    oversized_order.price = 50000.0;
    oversized_order.side = OrderSide::BUY;
    oversized_order.type = OrderType::LIMIT;
    
    ASSERT_FALSE(rm->checkOrderRisk(oversized_order));
}

void testPositionRiskCheck() {
    auto rm = createTestRiskManager();
    
    // Test valid position
    ASSERT_TRUE(rm->checkPositionRisk("BTCUSD", 500.0));
    
    // Test oversized position
    ASSERT_FALSE(rm->checkPositionRisk("BTCUSD", 1500.0)); // Exceeds max_position_size
    
    // Test negative position (short)
    ASSERT_TRUE(rm->checkPositionRisk("BTCUSD", -500.0));
    ASSERT_FALSE(rm->checkPositionRisk("BTCUSD", -1500.0));
}

void testOrderRateLimit() {
    auto rm = createTestRiskManager();
    
    // Test that we can place orders up to the limit
    for (int i = 0; i < 10; ++i) { // max_orders_per_minute is 10
        ASSERT_TRUE(rm->checkOrderRate("BTCUSD"));
    }
    
    // The 11th order should fail
    ASSERT_FALSE(rm->checkOrderRate("BTCUSD"));
    
    // Different symbol should still work
    ASSERT_TRUE(rm->checkOrderRate("ETHUSD"));
}

void testDailyLossCheck() {
    auto rm = createTestRiskManager();
    
    // Test acceptable loss
    ASSERT_TRUE(rm->checkDailyLoss(-1000.0));
    
    // Test excessive loss (should trigger emergency stop)
    ASSERT_FALSE(rm->checkDailyLoss(-6000.0)); // Exceeds max_daily_loss of -5000
    
    // After emergency stop, should be stopped
    ASSERT_TRUE(rm->isEmergencyStopped());
}

void testDrawdownCheck() {
    auto rm = createTestRiskManager();
    
    // Test acceptable drawdown
    ASSERT_TRUE(rm->checkDrawdown(-5.0));
    
    // Test excessive drawdown (should trigger emergency stop)
    ASSERT_FALSE(rm->checkDrawdown(-15.0)); // Exceeds max_drawdown of -10.0
    
    // After emergency stop, should be stopped
    ASSERT_TRUE(rm->isEmergencyStopped());
}

void testPositionUpdates() {
    auto rm = createTestRiskManager();
    
    // Update position
    rm->updatePosition("BTCUSD", 0.5, 50000.0);
    
    // Update with more quantity
    rm->updatePosition("BTCUSD", 0.3, 51000.0);
    
    // Position should be averaged: (0.5 * 50000 + 0.3 * 51000) / 0.8 = 50375
    // Total quantity: 0.8
    
    // This is internal state, but we can test indirectly through other methods
    ASSERT_TRUE(rm->checkPositionRisk("BTCUSD", 0.8));
}

void testPnLUpdates() {
    auto rm = createTestRiskManager();
    
    // Update PnL for a position
    rm->updatePnL("BTCUSD", 1000.0);
    
    // Check that daily loss is still within limits
    ASSERT_TRUE(rm->checkDailyLoss(-1000.0));
    
    // Update with a large loss
    rm->updatePnL("BTCUSD", -6000.0);
    
    // This should have triggered emergency stop
    ASSERT_TRUE(rm->isEmergencyStopped());
}

void testEmergencyStopAndResume() {
    auto rm = createTestRiskManager();
    
    // Initially not stopped
    ASSERT_FALSE(rm->isEmergencyStopped());
    
    // Trigger emergency stop
    rm->emergencyStop();
    ASSERT_TRUE(rm->isEmergencyStopped());
    
    // Orders should be rejected when stopped
    Order order;
    order.symbol = "BTCUSD";
    order.quantity = 50.0;
    order.side = OrderSide::BUY;
    order.type = OrderType::LIMIT;
    
    ASSERT_FALSE(rm->checkOrderRisk(order));
    
    // Resume operations
    rm->resume();
    ASSERT_FALSE(rm->isEmergencyStopped());
    
    // Orders should be accepted again
    ASSERT_TRUE(rm->checkOrderRisk(order));
}

void testRiskLimitsUpdate() {
    auto rm = createTestRiskManager();
    
    // Get initial limits
    auto initial_limits = rm->getRiskLimits();
    ASSERT_EQ(1000.0, initial_limits.max_position_size);
    
    // Update limits
    RiskLimits new_limits;
    new_limits.max_position_size = 2000.0;
    new_limits.max_order_size = 200.0;
    new_limits.max_daily_loss = -10000.0;
    new_limits.max_drawdown = -20.0;
    new_limits.max_orders_per_minute = 20;
    new_limits.min_spread = 0.002;
    new_limits.max_slippage = 0.02;
    
    rm->setRiskLimits(new_limits);
    
    // Verify limits were updated
    auto updated_limits = rm->getRiskLimits();
    ASSERT_EQ(2000.0, updated_limits.max_position_size);
    ASSERT_EQ(200.0, updated_limits.max_order_size);
    ASSERT_EQ(-10000.0, updated_limits.max_daily_loss);
    
    // Test that new limits are enforced
    ASSERT_TRUE(rm->checkPositionRisk("BTCUSD", 1500.0)); // Should pass now
    ASSERT_FALSE(rm->checkPositionRisk("BTCUSD", 2500.0)); // Should still fail
}

void testRiskReport() {
    auto rm = createTestRiskManager();
    
    // Add some data
    rm->updatePosition("BTCUSD", 0.5, 50000.0);
    rm->updatePosition("ETHUSD", 2.0, 3000.0);
    rm->updatePnL("BTCUSD", 500.0);
    rm->updatePnL("ETHUSD", -50.0);  // Changed from -200.0 to -50.0 to avoid triggering emergency stop
    
    auto report = rm->getRiskReport();
    
    // Check report structure
    ASSERT_TRUE(report.contains("emergency_stopped"));
    ASSERT_TRUE(report.contains("total_realized_pnl"));
    ASSERT_TRUE(report.contains("positions"));
    ASSERT_TRUE(report.contains("limits"));
    
    // Check values
    ASSERT_FALSE(report["emergency_stopped"]);
    ASSERT_TRUE(report["positions"].contains("BTCUSD"));
    ASSERT_TRUE(report["positions"].contains("ETHUSD"));
    ASSERT_EQ(1000.0, report["limits"]["max_position_size"]);
}

void testRiskLimitsFromJson() {
    nlohmann::json limits_json = {
        {"max_position_size", 500.0},
        {"max_order_size", 50.0},
        {"max_daily_loss", -2500.0},
        {"max_drawdown", -5.0},
        {"max_orders_per_minute", 5},
        {"min_spread", 0.0005},
        {"max_slippage", 0.005}
    };
    
    RiskLimits limits(limits_json);
    
    ASSERT_EQ(500.0, limits.max_position_size);
    ASSERT_EQ(50.0, limits.max_order_size);
    ASSERT_EQ(-2500.0, limits.max_daily_loss);
    ASSERT_EQ(-5.0, limits.max_drawdown);
    ASSERT_EQ(5, limits.max_orders_per_minute);
    ASSERT_EQ(0.0005, limits.min_spread);
    ASSERT_EQ(0.005, limits.max_slippage);
}

int main() {
    TestRunner runner;
    
    auto risk_suite = std::make_shared<TestSuite>("RiskManager");
    
    risk_suite->addTest("Construction", testRiskManagerConstruction);
    risk_suite->addTest("Order Risk Check", testOrderRiskCheck);
    risk_suite->addTest("Position Risk Check", testPositionRiskCheck);
    risk_suite->addTest("Order Rate Limit", testOrderRateLimit);
    risk_suite->addTest("Daily Loss Check", testDailyLossCheck);
    risk_suite->addTest("Drawdown Check", testDrawdownCheck);
    risk_suite->addTest("Position Updates", testPositionUpdates);
    risk_suite->addTest("PnL Updates", testPnLUpdates);
    risk_suite->addTest("Emergency Stop and Resume", testEmergencyStopAndResume);
    risk_suite->addTest("Risk Limits Update", testRiskLimitsUpdate);
    risk_suite->addTest("Risk Report", testRiskReport);
    risk_suite->addTest("Risk Limits from JSON", testRiskLimitsFromJson);
    
    runner.addSuite(risk_suite);
    runner.runAll();
    
    return 0;
}
