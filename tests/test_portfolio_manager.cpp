#include "testing/test_framework.h"
#include "core/portfolio_manager.h"
#include "logger.h"
#include <memory>
#include <nlohmann/json.hpp>

using namespace moneybot;
using namespace moneybot::testing;

std::shared_ptr<PortfolioManager> createTestPortfolioManager() {
    auto logger = std::make_shared<Logger>();
    nlohmann::json config = {
        {"portfolio", {
            {"base_currency", "USD"},
            {"auto_save_snapshots", false},
            {"max_history_snapshots", 100}
        }}
    };
    
    return std::make_shared<PortfolioManager>(logger, config);
}

void testPortfolioManagerConstruction() {
    auto pm = createTestPortfolioManager();
    ASSERT_TRUE(pm != nullptr);
    ASSERT_EQ("USD", pm->getBaseCurrency());
    ASSERT_EQ(0.0, pm->getTotalValue());
}

void testBalanceManagement() {
    auto pm = createTestPortfolioManager();
    
    // Test adding balance
    Balance balance;
    balance.asset = "BTC";
    balance.free = 1.0;
    balance.locked = 0.5;
    balance.total = 1.5;
    
    pm->updateBalance(balance);
    
    auto retrieved = pm->getBalance("BTC");
    ASSERT_EQ("BTC", retrieved.asset);
    ASSERT_EQ(1.0, retrieved.free);
    ASSERT_EQ(0.5, retrieved.locked);
    ASSERT_EQ(1.5, retrieved.total);
    
    // Test available balance
    ASSERT_EQ(1.0, pm->getAvailableBalance("BTC"));
    
    // Test all balances
    auto all_balances = pm->getAllBalances();
    ASSERT_EQ(1, all_balances.size());
    ASSERT_EQ("BTC", all_balances[0].asset);
}

void testPositionManagement() {
    auto pm = createTestPortfolioManager();
    
    // Test adding position
    Position position;
    position.symbol = "BTCUSD";
    position.quantity = 0.1;
    position.avg_price = 50000.0;
    position.unrealized_pnl = 500.0;
    position.realized_pnl = 0.0;
    
    pm->updatePosition(position);
    
    auto retrieved = pm->getPosition("BTCUSD");
    ASSERT_EQ("BTCUSD", retrieved.symbol);
    ASSERT_EQ(0.1, retrieved.quantity);
    ASSERT_EQ(50000.0, retrieved.avg_price);
    ASSERT_EQ(500.0, retrieved.unrealized_pnl);
    
    // Test all positions
    auto all_positions = pm->getAllPositions();
    ASSERT_EQ(1, all_positions.size());
    
    // Test active positions
    auto active_positions = pm->getActivePositions();
    ASSERT_EQ(1, active_positions.size());
    
    // Test closing position
    position.quantity = 0.0;
    pm->updatePosition(position);
    
    active_positions = pm->getActivePositions();
    ASSERT_EQ(0, active_positions.size());
}

void testTradeRecording() {
    auto pm = createTestPortfolioManager();
    
    // Set up a position first
    Position position;
    position.symbol = "BTCUSD";
    position.quantity = 0.1;
    position.avg_price = 50000.0;
    pm->updatePosition(position);
    
    // Record a trade
    Trade trade;
    trade.trade_id = "12345";
    trade.symbol = "BTCUSD";
    trade.price = 51000.0;
    trade.quantity = 0.05;
    trade.side = OrderSide::SELL;
    trade.timestamp = std::chrono::system_clock::now();
    
    pm->recordTrade(trade);
    
    auto trade_history = pm->getTradeHistory();
    ASSERT_EQ(1, trade_history.size());
    ASSERT_EQ("BTCUSD", trade_history[0].symbol);
    ASSERT_EQ(51000.0, trade_history[0].price);
    
    // Test filtered trade history
    auto btc_trades = pm->getTradeHistory("BTCUSD");
    ASSERT_EQ(1, btc_trades.size());
    
    auto eth_trades = pm->getTradeHistory("ETHUSD");
    ASSERT_EQ(0, eth_trades.size());
}

void testPnLCalculations() {
    auto pm = createTestPortfolioManager();
    
    // Add some positions with PnL
    Position position1;
    position1.symbol = "BTCUSD";
    position1.quantity = 0.1;
    position1.avg_price = 50000.0;
    position1.unrealized_pnl = 500.0;
    position1.realized_pnl = 100.0;
    pm->updatePosition(position1);
    
    Position position2;
    position2.symbol = "ETHUSD";
    position2.quantity = 1.0;
    position2.avg_price = 3000.0;
    position2.unrealized_pnl = -200.0;
    position2.realized_pnl = 50.0;
    pm->updatePosition(position2);
    
    // Test PnL calculations
    ASSERT_EQ(300.0, pm->getUnrealizedPnL()); // 500 - 200
    ASSERT_EQ(300.0, pm->getTotalPnL()); // 500 - 200 (unrealized only for now)
}

void testPortfolioMetrics() {
    auto pm = createTestPortfolioManager();
    
    // Add some trades to calculate metrics
    Trade trade1;
    trade1.trade_id = "1";
    trade1.symbol = "BTCUSD";
    trade1.price = 50000.0;
    trade1.quantity = 0.1;
    trade1.side = OrderSide::BUY;
    pm->recordTrade(trade1);
    
    Trade trade2;
    trade2.trade_id = "2";
    trade2.symbol = "BTCUSD";
    trade2.price = 51000.0;
    trade2.quantity = 0.05;
    trade2.side = OrderSide::SELL;
    pm->recordTrade(trade2);
    
    auto metrics = pm->calculateMetrics();
    ASSERT_EQ(2, metrics.total_trades);
    ASSERT_TRUE(metrics.win_rate >= 0.0 && metrics.win_rate <= 1.0);
}

void testPortfolioSnapshot() {
    auto pm = createTestPortfolioManager();
    
    // Add some data
    Balance balance;
    balance.asset = "USD";
    balance.free = 10000.0;
    balance.locked = 0.0;
    balance.total = 10000.0;
    pm->updateBalance(balance);
    
    Position position;
    position.symbol = "BTCUSD";
    position.quantity = 0.1;
    position.avg_price = 50000.0;
    position.unrealized_pnl = 500.0;
    pm->updatePosition(position);
    
    auto snapshot = pm->getSnapshot();
    ASSERT_TRUE(snapshot.balances.size() == 1);
    ASSERT_TRUE(snapshot.positions.size() == 1);
    ASSERT_EQ(500.0, snapshot.unrealized_pnl);
}

void testPortfolioReporting() {
    auto pm = createTestPortfolioManager();
    
    // Add some test data
    Balance balance;
    balance.asset = "USD";
    balance.free = 10000.0;
    balance.total = 10000.0;
    pm->updateBalance(balance);
    
    auto report = pm->getPortfolioReport();
    ASSERT_TRUE(report.contains("total_value"));
    ASSERT_TRUE(report.contains("balances"));
    ASSERT_TRUE(report.contains("positions"));
    ASSERT_TRUE(report.contains("metrics"));
    
    auto perf_report = pm->getPerformanceReport();
    ASSERT_TRUE(perf_report.contains("performance"));
    ASSERT_TRUE(perf_report.contains("risk_metrics"));
}

void testPortfolioClear() {
    auto pm = createTestPortfolioManager();
    
    // Add some data
    Balance balance;
    balance.asset = "USD";
    balance.total = 10000.0;
    pm->updateBalance(balance);
    
    Position position;
    position.symbol = "BTCUSD";
    position.quantity = 0.1;
    pm->updatePosition(position);
    
    // Clear portfolio
    pm->clear();
    
    // Verify it's cleared
    auto balances = pm->getAllBalances();
    auto positions = pm->getAllPositions();
    
    ASSERT_EQ(0, balances.size());
    ASSERT_EQ(0, positions.size());
    ASSERT_EQ(0.0, pm->getTotalValue());
}

int main() {
    TestRunner runner;
    
    auto portfolio_suite = std::make_shared<TestSuite>("PortfolioManager");
    
    portfolio_suite->addTest("Construction", testPortfolioManagerConstruction);
    portfolio_suite->addTest("Balance Management", testBalanceManagement);
    portfolio_suite->addTest("Position Management", testPositionManagement);
    portfolio_suite->addTest("Trade Recording", testTradeRecording);
    portfolio_suite->addTest("PnL Calculations", testPnLCalculations);
    portfolio_suite->addTest("Portfolio Metrics", testPortfolioMetrics);
    portfolio_suite->addTest("Portfolio Snapshot", testPortfolioSnapshot);
    portfolio_suite->addTest("Portfolio Reporting", testPortfolioReporting);
    portfolio_suite->addTest("Portfolio Clear", testPortfolioClear);
    
    runner.addSuite(portfolio_suite);
    runner.runAll();
    
    return 0;
}
