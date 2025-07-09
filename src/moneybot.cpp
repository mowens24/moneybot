#include "moneybot.h"
#include <fstream>
#include <iostream>

namespace moneybot {

TradingEngine::TradingEngine(const nlohmann::json& config) 
    : config_(config), running_(false), emergency_stop_(false),
      total_pnl_(0.0), total_trades_(0), start_time_(std::chrono::system_clock::now()),
      last_event_("init"), ws_connected_(false) {
    loadConfig(config);
    initializeComponents();
    logger_->getLogger()->info("TradingEngine initialized successfully");
}

TradingEngine::~TradingEngine() {
    stop();
}

void TradingEngine::initializeComponents() {
    // Initialize core components
    logger_ = std::make_shared<Logger>();
    order_book_ = std::make_shared<OrderBook>(logger_);
    network_ = std::make_shared<Network>(logger_, order_book_, config_);
    order_manager_ = std::make_shared<OrderManager>(logger_, config_);
    risk_manager_ = std::make_shared<RiskManager>(logger_, config_);
    network_->setOrderManager(order_manager_);
    
    // Initialize strategy based on config
    std::string strategy_type = config_["strategy"]["type"].get<std::string>();
    if (strategy_type == "market_maker") {
        strategy_ = std::make_shared<MarketMakerStrategy>(logger_, order_manager_, risk_manager_, config_);
    } else if (strategy_type == "multi_asset") {
        // For multi-asset mode, use a dummy strategy for now
        // In the future, this will create a Multi-Asset Strategy Manager
        strategy_ = std::make_shared<MarketMakerStrategy>(logger_, order_manager_, risk_manager_, config_);
        logger_->getLogger()->info("Multi-asset strategy mode initialized (using market maker for now)");
    } else {
        throw std::runtime_error("Unknown strategy type: " + strategy_type);
    }
    
    logger_->getLogger()->info("All components initialized");
}

void TradingEngine::loadConfig(const nlohmann::json& config) {
    std::lock_guard<std::mutex> lock(config_mutex_);
    config_ = config;
}

void TradingEngine::start() {
    if (running_.load()) {
        logger_->getLogger()->warn("TradingEngine already running");
        return;
    }
    
    logger_->getLogger()->info("Starting TradingEngine...");
    
    // Initialize strategy
    strategy_->initialize();
    
    // Start order manager
    order_manager_->start();

    // Start user data stream for private events
    std::string listenKey = order_manager_->createUserDataStream();
    if (!listenKey.empty()) {
        std::thread([this, listenKey]() {
            network_->runUserDataStream(listenKey);
        }).detach();
    } else {
        logger_->getLogger()->error("Failed to start user data stream: listenKey is empty");
    }

    // Start network thread
    running_.store(true);
    network_thread_ = std::thread(&TradingEngine::networkThread, this);
    
    // Start strategy thread
    strategy_thread_ = std::thread(&TradingEngine::strategyThread, this);
    
    logger_->getLogger()->info("TradingEngine started successfully");
}

void TradingEngine::stop() {
    if (!running_.load()) {
        return;
    }
    
    logger_->getLogger()->info("Stopping TradingEngine...");
    
    running_.store(false);
    
    // Stop network
    network_->stop();
    
    // Stop order manager
    order_manager_->stop();
    
    // Shutdown strategy
    strategy_->shutdown();
    
    // Wait for threads to finish
    if (network_thread_.joinable()) {
        network_thread_.join();
    }
    if (strategy_thread_.joinable()) {
        strategy_thread_.join();
    }
    
    logger_->getLogger()->info("TradingEngine stopped");
}

void TradingEngine::emergencyStop() {
    logger_->getLogger()->error("EMERGENCY STOP ACTIVATED");
    emergency_stop_.store(true);
    risk_manager_->emergencyStop();
    stop();
}

void TradingEngine::networkThread() {
    try {
        network_->run(
            config_["exchange"]["websocket_host"].get<std::string>(),
            config_["exchange"]["websocket_port"].get<std::string>(),
            config_["exchange"]["websocket_endpoint"].get<std::string>()
        );
    } catch (const std::exception& e) {
        logger_->getLogger()->error("Network thread error: {}", e.what());
        emergencyStop();
    }
}

void TradingEngine::strategyThread() {
    try {
        while (running_.load() && !emergency_stop_.load()) {
            // Strategy runs continuously, processing market data
            // The actual strategy logic is handled in the event callbacks
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    } catch (const std::exception& e) {
        logger_->getLogger()->error("Strategy thread error: {}", e.what());
        emergencyStop();
    }
}

void TradingEngine::onOrderBookUpdate(const OrderBook& order_book) {
    if (emergency_stop_.load()) return;
    setLastEvent("OrderBookUpdate");
    try {
        strategy_->onOrderBookUpdate(order_book);
    } catch (const std::exception& e) {
        logger_->getLogger()->error("Error in order book update: {}", e.what());
    }
}

void TradingEngine::onTrade(const Trade& trade) {
    if (emergency_stop_.load()) return;
    setLastEvent("Trade");
    try {
        strategy_->onTrade(trade);
        total_trades_++;
    } catch (const std::exception& e) {
        logger_->getLogger()->error("Error in trade processing: {}", e.what());
    }
}

void TradingEngine::onOrderAck(const OrderAck& ack) {
    if (emergency_stop_.load()) return;
    setLastEvent("OrderAck");
    try {
        strategy_->onOrderAck(ack);
    } catch (const std::exception& e) {
        logger_->getLogger()->error("Error in order ack processing: {}", e.what());
    }
}

void TradingEngine::onOrderReject(const OrderReject& reject) {
    if (emergency_stop_.load()) return;
    setLastEvent("OrderReject");
    try {
        strategy_->onOrderReject(reject);
    } catch (const std::exception& e) {
        logger_->getLogger()->error("Error in order reject processing: {}", e.what());
    }
}

void TradingEngine::onOrderFill(const OrderFill& fill) {
    if (emergency_stop_.load()) return;
    setLastEvent("OrderFill");
    try {
        strategy_->onOrderFill(fill);
        total_pnl_ += fill.commission; // Simplified PnL calculation
    } catch (const std::exception& e) {
        logger_->getLogger()->error("Error in order fill processing: {}", e.what());
    }
}

void TradingEngine::updateConfig(const nlohmann::json& config) {
    loadConfig(config);
    
    if (strategy_) {
        strategy_->updateConfig(config);
    }
    
    logger_->getLogger()->info("Configuration updated");
}

nlohmann::json TradingEngine::getStatus() const {
    std::lock_guard<std::mutex> lock(status_mutex_);
    
    nlohmann::json status;
    status["running"] = running_.load();
    status["emergency_stop"] = emergency_stop_.load();
    status["uptime_seconds"] = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now() - start_time_).count();
    status["last_event"] = last_event_;
    status["ws_connected"] = ws_connected_;
    
    // Risk status
    if (risk_manager_) {
        status["risk"] = risk_manager_->getRiskReport();
    }
    
    // Strategy status
    if (strategy_) {
        status["strategy"] = {
            {"name", strategy_->getName()},
            {"active", running_.load()}
        };
    }
    
    return status;
}

nlohmann::json TradingEngine::getPerformanceMetrics() const {
    std::lock_guard<std::mutex> lock(status_mutex_);
    
    auto now = std::chrono::system_clock::now();
    auto uptime = std::chrono::duration_cast<std::chrono::hours>(now - start_time_).count();
    
    nlohmann::json metrics;
    metrics["total_pnl"] = total_pnl_;
    metrics["total_trades"] = total_trades_;
    metrics["uptime_hours"] = uptime;
    metrics["trades_per_hour"] = uptime > 0 ? total_trades_ / uptime : 0.0;
    metrics["avg_pnl_per_trade"] = total_trades_ > 0 ? total_pnl_ / total_trades_ : 0.0;
    
    return metrics;
}

} // namespace moneybot