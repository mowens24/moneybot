#include "moneybot.h"

namespace moneybot {
    TradingEngine::TradingEngine(const nlohmann::json& config) : config_(config) {
        logger_ = std::make_shared<Logger>();
        order_book_ = std::make_shared<OrderBook>(logger_);
        network_ = std::make_shared<Network>(logger_, order_book_);
        logger_->getLogger()->info("TradingEngine initialized for data retrieval.");
    }

    TradingEngine::~TradingEngine() {
        stop();
    }

    void TradingEngine::start() {
        logger_->getLogger()->info("Starting TradingEngine...");
        network_->pingExchange("https://testnet.binance.vision/api/v3/ping");
        network_->connectWebSocket(
            config_["exchange"]["websocket_host"].get<std::string>(),
            config_["exchange"]["websocket_port"].get<std::string>(),
            config_["exchange"]["websocket_endpoint"].get<std::string>()
        );
        network_thread_ = std::thread([this]() { network_->run(); });
    }

    void TradingEngine::stop() {
        logger_->getLogger()->info("Stopping TradingEngine...");
        network_->stop();
        if (network_thread_.joinable()) network_thread_.join();
    }
} // namespace moneybot