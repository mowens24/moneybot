#pragma once
#include "multi_exchange_gateway.h"
#include "network.h"
#include <thread>
#include <atomic>

namespace moneybot {

// Base implementation for exchange connectors
class BaseExchangeConnector : public ExchangeConnector {
public:
    BaseExchangeConnector(const ExchangeConfig& config, std::shared_ptr<Logger> logger);
    virtual ~BaseExchangeConnector();
    
    // ExchangeConnector interface
    virtual void connect() override;
    virtual void disconnect() override;
    virtual bool isConnected() const override;
    virtual std::vector<std::string> getAvailableSymbols() const override;
    virtual std::string placeOrder(const Order& order) override;
    virtual bool cancelOrder(const std::string& order_id) override;
    virtual ExchangeBalance getBalance(const std::string& asset) const override;
    virtual OrderBook getOrderBook(const std::string& symbol) const override;
    virtual std::string getExchangeName() const override;
    virtual void setOrderBookCallback(std::function<void(const std::string&, const OrderBook&)> callback) override;
    virtual void setTradeCallback(std::function<void(const Trade&)> callback) override;
    virtual double getLatency() const override;
    
protected:
    ExchangeConfig config_;
    std::shared_ptr<Logger> logger_;
    std::shared_ptr<Network> network_;
    std::atomic<bool> connected_;
    std::atomic<bool> should_stop_;
    std::thread ws_thread_;
    
    std::function<void(const std::string&, const OrderBook&)> orderbook_callback_;
    std::function<void(const Trade&)> trade_callback_;
    
    mutable std::mutex latency_mutex_;
    double last_latency_ms_ = 0.0;
    
    // Virtual methods for specific exchange implementations
    virtual std::string getRestEndpoint() const = 0;
    virtual std::string getWebSocketEndpoint() const = 0;
    virtual nlohmann::json formatOrderRequest(const Order& order) const = 0;
    virtual Order parseOrderResponse(const nlohmann::json& response) const = 0;
    virtual void handleWebSocketMessage(const std::string& message) = 0;
    
    // Helper methods
    void updateLatency(double latency_ms);
    void logInfo(const std::string& message);
    void logError(const std::string& message);
    void logWarning(const std::string& message);
    
private:
    void webSocketLoop();
};

// Binance connector implementation
class BinanceConnector : public BaseExchangeConnector {
public:
    BinanceConnector(const ExchangeConfig& config, std::shared_ptr<Logger> logger);
    
    // Override specific methods
    virtual std::vector<std::string> getAvailableSymbols() const override;
    virtual std::string placeOrder(const Order& order) override;
    virtual bool cancelOrder(const std::string& order_id) override;
    virtual ExchangeBalance getBalance(const std::string& asset) const override;
    virtual OrderBook getOrderBook(const std::string& symbol) const override;
    virtual std::string getExchangeName() const override;
    
protected:
    virtual std::string getRestEndpoint() const override;
    virtual std::string getWebSocketEndpoint() const override;
    virtual nlohmann::json formatOrderRequest(const Order& order) const override;
    virtual Order parseOrderResponse(const nlohmann::json& response) const override;
    virtual void handleWebSocketMessage(const std::string& message) override;
};

// Coinbase connector implementation  
class CoinbaseConnector : public BaseExchangeConnector {
public:
    CoinbaseConnector(const ExchangeConfig& config, std::shared_ptr<Logger> logger);
    
    // Override specific methods
    virtual std::vector<std::string> getAvailableSymbols() const override;
    virtual std::string placeOrder(const Order& order) override;
    virtual bool cancelOrder(const std::string& order_id) override;
    virtual ExchangeBalance getBalance(const std::string& asset) const override;
    virtual OrderBook getOrderBook(const std::string& symbol) const override;
    virtual std::string getExchangeName() const override;
    
protected:
    virtual std::string getRestEndpoint() const override;
    virtual std::string getWebSocketEndpoint() const override;
    virtual nlohmann::json formatOrderRequest(const Order& order) const override;
    virtual Order parseOrderResponse(const nlohmann::json& response) const override;
    virtual void handleWebSocketMessage(const std::string& message) override;
};

// Kraken connector implementation
class KrakenConnector : public BaseExchangeConnector {
public:
    KrakenConnector(const ExchangeConfig& config, std::shared_ptr<Logger> logger);
    
    // Override specific methods
    virtual std::vector<std::string> getAvailableSymbols() const override;
    virtual std::string placeOrder(const Order& order) override;
    virtual bool cancelOrder(const std::string& order_id) override;
    virtual ExchangeBalance getBalance(const std::string& asset) const override;
    virtual OrderBook getOrderBook(const std::string& symbol) const override;
    virtual std::string getExchangeName() const override;
    
protected:
    virtual std::string getRestEndpoint() const override;
    virtual std::string getWebSocketEndpoint() const override;
    virtual nlohmann::json formatOrderRequest(const Order& order) const override;
    virtual Order parseOrderResponse(const nlohmann::json& response) const override;
    virtual void handleWebSocketMessage(const std::string& message) override;
};

} // namespace moneybot
