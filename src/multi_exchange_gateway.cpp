#include "multi_exchange_gateway.h"
#include "exchange_connectors.h"
#include "config_manager.h"
#include <algorithm>
#include <thread>
#include <chrono>
#include <cmath>
#include <set>

namespace moneybot {

MultiExchangeGateway::MultiExchangeGateway(const std::vector<ExchangeConfig>& configs, 
                                           std::shared_ptr<Logger> logger)
    : configs_(configs), logger_(logger) {
    
    // Create connectors for each enabled exchange
    for (const auto& config : configs_) {
        if (config.enabled) {
            try {
                auto connector = createExchangeConnector(config, logger_);
                if (connector) {
                    connectors_[config.name] = std::move(connector);
                    logInfo("Created connector for exchange: " + config.name);
                }
            } catch (const std::exception& e) {
                logError("Failed to create connector for " + config.name + ": " + e.what());
            }
        }
    }
    
    logInfo("MultiExchangeGateway initialized with " + std::to_string(connectors_.size()) + " exchanges");
}

MultiExchangeGateway::~MultiExchangeGateway() {
    stop();
}

void MultiExchangeGateway::start() {
    if (running_) {
        logWarning("MultiExchangeGateway already running");
        return;
    }
    
    running_ = true;
    logInfo("Starting MultiExchangeGateway...");
    
    // Connect to all exchanges
    for (auto& [exchange_name, connector] : connectors_) {
        try {
            // Set up callbacks
            connector->setOrderBookCallback([this, exchange_name](const std::string& symbol, const OrderBook& book) {
                onOrderBookUpdate(exchange_name, symbol, book);
            });
            
            connector->setTradeCallback([this, exchange_name](const Trade& trade) {
                onTradeUpdate(exchange_name, trade);
            });
            
            // Connect to exchange
            connector->connect();
            logInfo("Connected to exchange: " + exchange_name);
            
        } catch (const std::exception& e) {
            logError("Failed to connect to " + exchange_name + ": " + e.what());
        }
    }
    
    // Start worker threads
    worker_threads_.emplace_back([this]() {
        while (running_) {
            try {
                scanArbitrageOpportunities();
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            } catch (const std::exception& e) {
                logError("Error in arbitrage scanner: " + std::string(e.what()));
            }
        }
    });
    
    worker_threads_.emplace_back([this]() {
        while (running_) {
            try {
                // Update balances every 30 seconds
                for (const auto& [exchange_name, connector] : connectors_) {
                    if (connector->isConnected()) {
                        updateBalances(exchange_name);
                    }
                }
                std::this_thread::sleep_for(std::chrono::seconds(30));
            } catch (const std::exception& e) {
                logError("Error updating balances: " + std::string(e.what()));
            }
        }
    });
    
    logInfo("MultiExchangeGateway started successfully");
}

void MultiExchangeGateway::stop() {
    if (!running_) return;
    
    logInfo("Stopping MultiExchangeGateway...");
    running_ = false;
    
    // Wait for worker threads
    for (auto& thread : worker_threads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    worker_threads_.clear();
    
    // Disconnect from all exchanges
    for (auto& [exchange_name, connector] : connectors_) {
        try {
            connector->disconnect();
            logInfo("Disconnected from exchange: " + exchange_name);
        } catch (const std::exception& e) {
            logError("Error disconnecting from " + exchange_name + ": " + e.what());
        }
    }
    
    logInfo("MultiExchangeGateway stopped");
}

bool MultiExchangeGateway::isExchangeConnected(const std::string& exchange) const {
    auto it = connectors_.find(exchange);
    return it != connectors_.end() && it->second->isConnected();
}

std::vector<std::string> MultiExchangeGateway::getConnectedExchanges() const {
    std::vector<std::string> connected;
    for (const auto& [exchange_name, connector] : connectors_) {
        if (connector->isConnected()) {
            connected.push_back(exchange_name);
        }
    }
    return connected;
}

std::vector<std::string> MultiExchangeGateway::getAllExchanges() const {
    std::vector<std::string> exchanges;
    for (const auto& [exchange_name, connector] : connectors_) {
        exchanges.push_back(exchange_name);
    }
    return exchanges;
}

CrossExchangeOrderBook MultiExchangeGateway::getAggregatedOrderBook(const std::string& symbol) const {
    std::lock_guard<std::mutex> lock(books_mutex_);
    auto it = aggregated_books_.find(symbol);
    if (it != aggregated_books_.end()) {
        return it->second;
    }
    return CrossExchangeOrderBook{symbol};
}

std::vector<std::string> MultiExchangeGateway::getAvailableSymbols() const {
    std::set<std::string> unique_symbols;
    
    for (const auto& [exchange_name, connector] : connectors_) {
        if (connector->isConnected()) {
            auto symbols = connector->getAvailableSymbols();
            unique_symbols.insert(symbols.begin(), symbols.end());
        }
    }
    
    return std::vector<std::string>(unique_symbols.begin(), unique_symbols.end());
}

std::string MultiExchangeGateway::placeOrder(const std::string& exchange, const Order& order) {
    auto it = connectors_.find(exchange);
    if (it == connectors_.end()) {
        throw std::runtime_error("Exchange not found: " + exchange);
    }
    
    if (!it->second->isConnected()) {
        throw std::runtime_error("Exchange not connected: " + exchange);
    }
    
    return it->second->placeOrder(order);
}

bool MultiExchangeGateway::cancelOrder(const std::string& exchange, const std::string& order_id) {
    auto it = connectors_.find(exchange);
    if (it == connectors_.end()) {
        logError("Exchange not found: " + exchange);
        return false;
    }
    
    if (!it->second->isConnected()) {
        logError("Exchange not connected: " + exchange);
        return false;
    }
    
    return it->second->cancelOrder(order_id);
}

ExchangeBalance MultiExchangeGateway::getBalance(const std::string& exchange, const std::string& asset) const {
    std::lock_guard<std::mutex> lock(balances_mutex_);
    auto exchange_it = balances_.find(exchange);
    if (exchange_it != balances_.end()) {
        auto asset_it = exchange_it->second.find(asset);
        if (asset_it != exchange_it->second.end()) {
            return asset_it->second;
        }
    }
    return ExchangeBalance{asset};
}

double MultiExchangeGateway::getTotalBalance(const std::string& asset) const {
    std::lock_guard<std::mutex> lock(balances_mutex_);
    double total = 0.0;
    
    for (const auto& [exchange_name, exchange_balances] : balances_) {
        auto it = exchange_balances.find(asset);
        if (it != exchange_balances.end()) {
            total += it->second.total;
        }
    }
    
    return total;
}

std::vector<ArbitrageOpportunity> MultiExchangeGateway::findArbitrageOpportunities(double min_profit_bps) const {
    std::vector<ArbitrageOpportunity> opportunities;
    std::lock_guard<std::mutex> lock(books_mutex_);
    
    for (const auto& [symbol, aggregated_book] : aggregated_books_) {
        if (aggregated_book.exchange_books.size() < 2) continue;
        
        // Find best bid and ask across exchanges
        double best_bid = 0.0;
        double best_ask = std::numeric_limits<double>::max();
        std::string best_bid_exchange, best_ask_exchange;
        
        for (const auto& [exchange_name, book_ptr] : aggregated_book.exchange_books) {
            if (!isExchangeConnected(exchange_name) || !book_ptr) continue;
            
            double bid = book_ptr->getBestBid();
            double ask = book_ptr->getBestAsk();
            
            if (bid > best_bid) {
                best_bid = bid;
                best_bid_exchange = exchange_name;
            }
            
            if (ask < best_ask && ask > 0) {
                best_ask = ask;
                best_ask_exchange = exchange_name;
            }
        }
        
        // Calculate arbitrage opportunity
        if (best_bid > 0 && best_ask > 0 && best_bid_exchange != best_ask_exchange) {
            double profit_bps = ((best_bid - best_ask) / best_ask) * 10000.0;
            
            if (profit_bps >= min_profit_bps) {
                ArbitrageOpportunity opp;
                opp.symbol = symbol;
                opp.buy_exchange = best_ask_exchange;
                opp.sell_exchange = best_bid_exchange;
                opp.buy_price = best_ask;
                opp.sell_price = best_bid;
                opp.profit_bps = profit_bps;
                opp.max_size = calculateMaxArbitrageSize(opp);
                opp.profit_usd = opp.max_size * (best_bid - best_ask);
                opp.confidence_score = calculateConfidenceScore(opp);
                opp.timestamp = std::chrono::steady_clock::now();
                
                opportunities.push_back(opp);
            }
        }
    }
    
    // Sort by profit potential
    std::sort(opportunities.begin(), opportunities.end(), 
              [](const ArbitrageOpportunity& a, const ArbitrageOpportunity& b) {
                  return a.profit_usd > b.profit_usd;
              });
    
    return opportunities;
}

double MultiExchangeGateway::getBestPrice(const std::string& symbol, bool is_bid) const {
    auto aggregated_book = getAggregatedOrderBook(symbol);
    return is_bid ? aggregated_book.best_bid_price : aggregated_book.best_ask_price;
}

nlohmann::json MultiExchangeGateway::getPerformanceMetrics() const {
    nlohmann::json metrics;
    
    std::lock_guard<std::mutex> metrics_lock(metrics_mutex_);
    
    // Exchange connectivity
    metrics["exchanges"]["total"] = connectors_.size();
    metrics["exchanges"]["connected"] = getConnectedExchanges().size();
    
    // Latency metrics
    nlohmann::json latencies;
    double total_latency = 0.0;
    int connected_count = 0;
    
    for (const auto& [exchange_name, latency] : exchange_latencies_) {
        latencies[exchange_name] = latency;
        if (isExchangeConnected(exchange_name)) {
            total_latency += latency;
            connected_count++;
        }
    }
    
    metrics["latency"]["per_exchange"] = latencies;
    metrics["latency"]["average"] = connected_count > 0 ? total_latency / connected_count : 0.0;
    
    // Market data
    std::lock_guard<std::mutex> books_lock(books_mutex_);
    metrics["market_data"]["symbols_tracked"] = aggregated_books_.size();
    
    nlohmann::json symbol_stats;
    for (const auto& [symbol, book] : aggregated_books_) {
        symbol_stats[symbol]["exchanges"] = book.exchange_books.size();
        symbol_stats[symbol]["spread"] = book.best_ask_price > 0 && book.best_bid_price > 0 ? 
                                        book.best_ask_price - book.best_bid_price : 0.0;
        symbol_stats[symbol]["last_update"] = std::chrono::duration_cast<std::chrono::milliseconds>(
            book.last_update.time_since_epoch()).count();
    }
    metrics["market_data"]["symbols"] = symbol_stats;
    
    // Arbitrage opportunities
    auto opportunities = findArbitrageOpportunities(10.0);
    metrics["arbitrage"]["opportunities_count"] = opportunities.size();
    metrics["arbitrage"]["total_profit_potential"] = 0.0;
    
    for (const auto& opp : opportunities) {
        metrics["arbitrage"]["total_profit_potential"] = 
            metrics["arbitrage"]["total_profit_potential"].get<double>() + opp.profit_usd;
    }
    
    return metrics;
}

nlohmann::json MultiExchangeGateway::getStatus() const {
    nlohmann::json status;
    
    status["running"] = running_.load();
    status["exchanges"] = nlohmann::json::object();
    
    for (const auto& [exchange_name, connector] : connectors_) {
        auto& exchange_status = status["exchanges"][exchange_name];
        exchange_status["connected"] = connector->isConnected();
        exchange_status["latency_ms"] = getExchangeLatency(exchange_name);
        
        // Add symbol count for this exchange
        if (connector->isConnected()) {
            exchange_status["symbols"] = connector->getAvailableSymbols().size();
        }
    }
    
    status["market_data"]["aggregated_symbols"] = aggregated_books_.size();
    status["worker_threads"] = worker_threads_.size();
    
    return status;
}

// Private methods implementation

void MultiExchangeGateway::onOrderBookUpdate(const std::string& exchange, const std::string& symbol, const OrderBook& book) {
    {
        std::lock_guard<std::mutex> lock(books_mutex_);
        auto& aggregated_book = aggregated_books_[symbol];
        aggregated_book.symbol = symbol;
        
        // Create a copy of the order book - we need to handle the logger requirement
        if (aggregated_book.exchange_books.find(exchange) == aggregated_book.exchange_books.end()) {
            aggregated_book.exchange_books[exchange] = std::make_shared<OrderBook>(logger_);
        }
        
        // Copy the order book data (we'll need to add a copy method or assignment operator)
        // For now, we'll store a reference to the original book
        // TODO: Implement proper deep copy of OrderBook
        
        aggregated_book.last_update = std::chrono::steady_clock::now();
        
        updateAggregatedBook(symbol);
    }
    
    // Call user callback if set
    if (orderbook_callback_) {
        orderbook_callback_(exchange, symbol, book);
    }
}

void MultiExchangeGateway::onTradeUpdate(const std::string& exchange, const Trade& trade) {
    // Log trade update
    logInfo("Trade update from " + exchange + ": " + trade.symbol + " " + 
            std::to_string(trade.price) + " @ " + std::to_string(trade.quantity));
    
    // Call user trade callback if set
    if (trade_callback_) {
        trade_callback_(exchange, trade);
    }
}

void MultiExchangeGateway::updateAggregatedBook(const std::string& symbol) {
    auto& aggregated_book = aggregated_books_[symbol];
    
    double best_bid = 0.0;
    double best_ask = std::numeric_limits<double>::max();
    std::string best_bid_exchange, best_ask_exchange;
    double best_bid_size = 0.0, best_ask_size = 0.0;
    
    for (const auto& [exchange_name, book_ptr] : aggregated_book.exchange_books) {
        if (!isExchangeConnected(exchange_name) || !book_ptr) continue;
        
        double bid = book_ptr->getBestBid();
        double ask = book_ptr->getBestAsk();
        
        if (bid > best_bid) {
            best_bid = bid;
            best_bid_exchange = exchange_name;
            best_bid_size = book_ptr->getBestBidSize();
        }
        
        if (ask < best_ask && ask > 0) {
            best_ask = ask;
            best_ask_exchange = exchange_name;
            best_ask_size = book_ptr->getBestAskSize();
        }
    }
    
    aggregated_book.best_bid_price = best_bid;
    aggregated_book.best_ask_price = best_ask == std::numeric_limits<double>::max() ? 0.0 : best_ask;
    aggregated_book.best_bid_exchange = best_bid_exchange;
    aggregated_book.best_ask_exchange = best_ask_exchange;
    aggregated_book.best_bid_size = best_bid_size;
    aggregated_book.best_ask_size = best_ask_size;
}

void MultiExchangeGateway::scanArbitrageOpportunities() {
    auto opportunities = findArbitrageOpportunities(15.0); // Scan for 15+ bps opportunities
    
    for (const auto& opp : opportunities) {
        if (arbitrage_callback_) {
            arbitrage_callback_(opp);
        }
    }
}

double MultiExchangeGateway::calculateMaxArbitrageSize(const ArbitrageOpportunity& opp) const {
    // Get order book depths
    auto aggregated_book = getAggregatedOrderBook(opp.symbol);
    
    auto buy_book_it = aggregated_book.exchange_books.find(opp.buy_exchange);
    auto sell_book_it = aggregated_book.exchange_books.find(opp.sell_exchange);
    
    if (buy_book_it == aggregated_book.exchange_books.end() || 
        sell_book_it == aggregated_book.exchange_books.end() ||
        !buy_book_it->second || !sell_book_it->second) {
        return 0.0;
    }
    
    double buy_size = buy_book_it->second->getBestAskSize();
    double sell_size = sell_book_it->second->getBestBidSize();
    
    return std::min(buy_size, sell_size);
}

double MultiExchangeGateway::calculateConfidenceScore(const ArbitrageOpportunity& opp) const {
    double score = 1.0;
    
    // Reduce confidence based on latency
    double buy_latency = getExchangeLatency(opp.buy_exchange);
    double sell_latency = getExchangeLatency(opp.sell_exchange);
    double total_latency = buy_latency + sell_latency;
    
    if (total_latency > 100.0) {
        score *= 0.5; // High latency reduces confidence
    }
    
    // Reduce confidence if opportunity is stale
    auto now = std::chrono::steady_clock::now();
    auto age_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - opp.timestamp).count();
    if (age_ms > 1000) {
        score *= 0.3; // Stale data
    }
    
    // Increase confidence for larger size
    if (opp.max_size > 0.01) {
        score *= 1.2;
    }
    
    return std::min(score, 1.0);
}

void MultiExchangeGateway::updateBalances(const std::string& exchange) {
    auto connector_it = connectors_.find(exchange);
    if (connector_it == connectors_.end() || !connector_it->second->isConnected()) {
        return;
    }
    
    // This would typically fetch all balances from the exchange
    // For now, we'll implement a placeholder
    std::lock_guard<std::mutex> lock(balances_mutex_);
    // Implementation would go here
}

double MultiExchangeGateway::getExchangeLatency(const std::string& exchange) const {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    auto it = exchange_latencies_.find(exchange);
    return it != exchange_latencies_.end() ? it->second : 0.0;
}

void MultiExchangeGateway::logInfo(const std::string& message) const {
    if (logger_) {
        logger_->getLogger()->info("[MultiExchangeGateway] {}", message);
    }
}

void MultiExchangeGateway::logWarning(const std::string& message) const {
    if (logger_) {
        logger_->getLogger()->warn("[MultiExchangeGateway] {}", message);
    }
}

void MultiExchangeGateway::logError(const std::string& message) const {
    if (logger_) {
        logger_->getLogger()->error("[MultiExchangeGateway] {}", message);
    }
}

// Exchange connector factory with proper API key injection
std::unique_ptr<ExchangeConnector> createExchangeConnector(const ExchangeConfig& config, 
                                                           std::shared_ptr<Logger> logger) {
    std::string exchange_name = config.name;
    std::transform(exchange_name.begin(), exchange_name.end(), exchange_name.begin(), ::tolower);
    
    // Get updated config with environment variable API keys
    auto& config_manager = ConfigManager::getInstance();
    nlohmann::json exchange_config = config_manager.getExchangeConfig(exchange_name);
    
    if (exchange_config.empty()) {
        logger->getLogger()->error("No configuration found for exchange: {}", config.name);
        return nullptr;
    }
    
    // Create ExchangeConfig with resolved API keys
    ExchangeConfig updated_config = config;
    
    if (exchange_config.contains("api_key")) {
        updated_config.api_key = exchange_config["api_key"];
    }
    if (exchange_config.contains("secret_key")) {
        updated_config.secret_key = exchange_config["secret_key"];
    }
    if (exchange_config.contains("passphrase")) {
        updated_config.passphrase = exchange_config["passphrase"];
    }
    
    // Validate API keys for production mode
    if (!config_manager.isDryRunMode()) {
        if (updated_config.api_key.empty() || updated_config.secret_key.empty()) {
            logger->getLogger()->error("Missing API keys for exchange: {}. Set environment variables.", config.name);
            return nullptr;
        }
        
        // Check for placeholder values
        if (updated_config.api_key.find("your_") != std::string::npos || 
            updated_config.secret_key.find("your_") != std::string::npos) {
            logger->getLogger()->error("Placeholder API keys detected for exchange: {}. Update environment variables.", config.name);
            return nullptr;
        }
    }
    
    if (exchange_name == "binance") {
        return std::make_unique<BinanceConnector>(updated_config, logger);
    } else if (exchange_name == "coinbase") {
        return std::make_unique<CoinbaseConnector>(updated_config, logger);
    } else if (exchange_name == "kraken") {
        return std::make_unique<KrakenConnector>(updated_config, logger);
    } else {
        logger->getLogger()->warn("Unknown exchange: {}. Using base connector.", config.name);
        // For unknown exchanges, we could return a generic base connector
        // For now, return nullptr to indicate unsupported exchange
        return nullptr;
    }
}

} // namespace moneybot
