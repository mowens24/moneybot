#include "binance_exchange.h"
#include "modern_logger.h"
#include "event_manager.h"
#include "application_state.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <chrono>
#include <thread>
#include <openssl/hmac.h>
#include <openssl/sha.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// Static callback for CURL
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp) {
    size_t totalSize = size * nmemb;
    userp->append((char*)contents, totalSize);
    return totalSize;
}

BinanceExchange::BinanceExchange(const std::string& api_key, const std::string& secret_key, bool testnet)
    : api_key(api_key), secret_key(secret_key), use_testnet(testnet), 
      last_rate_limit_reset(std::chrono::steady_clock::now()) {
    
    if (testnet) {
        base_url = "https://testnet.binance.vision/api/v3";
    } else {
        // Use Binance US for US users
        base_url = "https://api.binance.us/api/v3";
    }
    
    // Initialize CURL
    curl_handle = curl_easy_init();
    if (!curl_handle) {
        throw std::runtime_error("Failed to initialize CURL");
    }
    
    // Setup polling thread for market data (simplified approach)
    polling_active = false;
}

BinanceExchange::~BinanceExchange() {
    disconnect();
    if (curl_handle) {
        curl_easy_cleanup(curl_handle);
    }
}

bool BinanceExchange::connect() {
    try {
        LOG_INFO("Connecting to Binance REST API...");
        
        // Test REST API connection
        auto response = makeRequest("/ping");
        if (response.response_code != 200) {
            LOG_ERROR("Binance REST API not reachable - Response code: " + std::to_string(response.response_code));
            PUBLISH_EVENT(moneybot::events::createConnectionEvent("binance", 
                moneybot::ConnectionEvent::ConnectionEventType::ERROR, "API not reachable"));
            return false;
        }
        
        // Start polling for market data
        polling_active = true;
        polling_thread = std::thread(&BinanceExchange::pollMarketData, this);
        
        LOG_INFO("Connected to Binance exchange successfully");
        PUBLISH_EVENT(moneybot::events::createConnectionEvent("binance", 
            moneybot::ConnectionEvent::ConnectionEventType::CONNECTED, "Connection established"));
        
        auto& state_manager = moneybot::ApplicationStateManager::getInstance();
        state_manager.setExchangeConnected("binance", true);
        
        return true;
        
    } catch (const std::exception& e) {
        LOG_ERROR("Binance connection error: " + std::string(e.what()));
        PUBLISH_EVENT(moneybot::events::createErrorEvent(moneybot::ErrorEvent::ErrorLevel::ERROR, 
            "binance_exchange", "Connection failed", e.what()));
        return false;
    }
}

bool BinanceExchange::disconnect() {
    try {
        polling_active = false;
        
        if (polling_thread.joinable()) {
            polling_thread.join();
        }
        
        LOG_INFO("Disconnected from Binance");
        PUBLISH_EVENT(moneybot::events::createConnectionEvent("binance", 
            moneybot::ConnectionEvent::ConnectionEventType::DISCONNECTED, "Disconnected successfully"));
        
        auto& state_manager = moneybot::ApplicationStateManager::getInstance();
        state_manager.setExchangeConnected("binance", false);
        
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("Error disconnecting from Binance: " + std::string(e.what()));
        PUBLISH_EVENT(moneybot::events::createErrorEvent(moneybot::ErrorEvent::ErrorLevel::ERROR, 
            "binance_exchange", "Disconnection failed", e.what()));
        return false;
    }
}

void BinanceExchange::pollMarketData() {
    std::vector<std::string> symbols = {"BTCUSDT", "ETHUSDT", "ADAUSDT", "DOTUSDT", "LINKUSDT"};
    
    while (polling_active) {
        try {
            // Get 24hr ticker statistics for all symbols
            auto response = makeRequest("/ticker/24hr");
            if (response.response_code == 200 && !response.data.empty()) {
                try {
                    auto data = json::parse(response.data);
                    
                    for (const auto& ticker : data) {
                        std::string symbol = ticker["symbol"].get<std::string>();
                        
                        // Only process symbols we're interested in
                        if (std::find(symbols.begin(), symbols.end(), symbol) != symbols.end()) {
                            LiveTickData tick;
                            tick.symbol = symbol;
                            tick.exchange = "binance";
                            tick.bid_price = std::stod(ticker["bidPrice"].get<std::string>());
                            tick.ask_price = std::stod(ticker["askPrice"].get<std::string>());
                            tick.last_price = std::stod(ticker["lastPrice"].get<std::string>());
                            tick.volume_24h = std::stod(ticker["volume"].get<std::string>());
                            tick.high_24h = std::stod(ticker["highPrice"].get<std::string>());
                            tick.low_24h = std::stod(ticker["lowPrice"].get<std::string>());
                            tick.price_change_24h = std::stod(ticker["priceChange"].get<std::string>());
                            tick.price_change_percent_24h = std::stod(ticker["priceChangePercent"].get<std::string>());
                            tick.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                                std::chrono::system_clock::now().time_since_epoch()).count();
                            
                            {
                                std::lock_guard<std::mutex> lock(data_mutex);
                                tickers[symbol] = tick;
                            }
                            
                            // Calculate spread for internal use
                            double spread_bps = ((tick.ask_price - tick.bid_price) / tick.bid_price) * 10000;
                            
                            // Optional debug output (disabled for clean operation)
                            if (false) { // Change to true if you want to see live data in console
                                std::cout << "🚀 LIVE " << tick.exchange << " " << tick.symbol 
                                          << " | Bid: $" << std::fixed << std::setprecision(2) << tick.bid_price
                                          << " | Ask: $" << tick.ask_price 
                                          << " | Last: $" << tick.last_price
                                          << " | Spread: " << std::setprecision(2) << spread_bps << " bps"
                                          << " | Vol: " << std::setprecision(0) << tick.volume_24h << std::endl;
                            }
                            
                            // Trigger callback if set
                            if (tick_callback) {
                                tick_callback(tick);
                            }
                        }
                    }
                } catch (const json::parse_error& e) {
                    if (error_callback) {
                        error_callback("JSON_PARSE_ERROR", e.what());
                    }
                    std::cout << "❌ JSON parse error in market data: " << e.what() << std::endl;
                }
            } else {
                if (error_callback) {
                    error_callback("MARKET_DATA_REQUEST_FAILED", 
                                 "Response code: " + std::to_string(response.response_code) + 
                                 ", Data: " + response.data);
                }
                std::cout << "❌ Market data request failed. Code: " << response.response_code 
                          << ", Error: " << response.error << std::endl;
            }
        } catch (const std::exception& e) {
            std::cout << "❌ Error polling market data: " << e.what() << std::endl;
        }
        
        // Poll every 5 seconds to avoid rate limits
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
}

bool BinanceExchange::isConnected() const {
    return polling_active.load();
}

LiveTickData BinanceExchange::getLatestTick(const std::string& symbol) {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(data_mutex));
    auto it = tickers.find(symbol);
    if (it != tickers.end()) {
        return it->second;
    }
    return LiveTickData{}; // Return empty tick if not found
}

std::vector<std::string> BinanceExchange::getAvailableSymbols() const {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(data_mutex));
    std::vector<std::string> symbols;
    for (const auto& [symbol, tick] : tickers) {
        symbols.push_back(symbol);
    }
    return symbols;
}

std::string BinanceExchange::getStatus() const {
    if (polling_active) {
        return "Connected - Live Data Active";
    } else {
        return "Disconnected";
    }
}

// HTTP Request implementation
BinanceExchange::HTTPResponse BinanceExchange::makeRequest(const std::string& endpoint, 
                                                          const std::string& params, 
                                                          const std::string& method, 
                                                          bool sign) {
    HTTPResponse response;
    
    if (!curl_handle) {
        response.error = "CURL not initialized";
        return response;
    }
    
    std::string url = base_url + endpoint;
    std::string query_params = params;
    
    if (sign && !api_key.empty()) {
        // Add timestamp
        auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        
        if (!query_params.empty()) {
            query_params += "&";
        }
        query_params += "timestamp=" + std::to_string(timestamp);
        
        // Add signature
        if (!secret_key.empty()) {
            std::string signature = generateSignature(query_params);
            query_params += "&signature=" + signature;
        }
    }
    
    if (!query_params.empty()) {
        url += "?" + query_params;
    }
    
    // Track rate limiting
    requests_per_minute++;
    
    // Reset counter every minute
    auto now = std::chrono::steady_clock::now();
    if (std::chrono::duration_cast<std::chrono::minutes>(now - last_rate_limit_reset).count() >= 1) {
        requests_per_minute = 1;
        last_rate_limit_reset = now;
    }
    
    curl_easy_setopt(curl_handle, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, &response.data);
    curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT, 10L);
    curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYPEER, 1L);
    
    // Set headers
    struct curl_slist* headers = nullptr;
    if (sign && !api_key.empty()) {
        std::string api_header = "X-MBX-APIKEY: " + api_key;
        headers = curl_slist_append(headers, api_header.c_str());
        curl_easy_setopt(curl_handle, CURLOPT_HTTPHEADER, headers);
    }
    
    CURLcode res = curl_easy_perform(curl_handle);
    
    if (headers) {
        curl_slist_free_all(headers);
    }
    
    if (res != CURLE_OK) {
        response.error = curl_easy_strerror(res);
        response.response_code = 0;
    } else {
        curl_easy_getinfo(curl_handle, CURLINFO_RESPONSE_CODE, &response.response_code);
    }
    
    return response;
}

std::string BinanceExchange::generateSignature(const std::string& query_string) {
    unsigned char* digest = HMAC(EVP_sha256(), 
                                secret_key.c_str(), secret_key.length(),
                                (unsigned char*)query_string.c_str(), query_string.length(),
                                nullptr, nullptr);
    
    std::stringstream ss;
    for (int i = 0; i < 32; ++i) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)digest[i];
    }
    
    return ss.str();
}

// Order management implementations
bool BinanceExchange::placeOrder(const LiveOrder& order) {
    try {
        std::string params = "symbol=" + order.symbol + 
                           "&side=" + order.side + 
                           "&type=" + order.type + 
                           "&quantity=" + std::to_string(order.quantity);
        
        if (order.type == "LIMIT") {
            params += "&price=" + std::to_string(order.price);
            params += "&timeInForce=GTC";  // Good Till Canceled
        }
        
        auto response = makeRequest("/order", params, "POST", true);
        if (response.response_code == 200) {
            auto data = json::parse(response.data);
            std::cout << "✅ Order placed: " << data["orderId"] << std::endl;
            
            // Trigger callback if set
            if (order_update_callback) {
                LiveOrder updated_order = order;
                updated_order.order_id = std::to_string(data["orderId"].get<int64_t>());
                updated_order.status = data["status"].get<std::string>();
                order_update_callback(updated_order);
            }
            
            return true;
        } else {
            if (error_callback) {
                error_callback("ORDER_PLACEMENT_FAILED", response.data);
            }
            std::cout << "❌ Order placement failed: " << response.data << std::endl;
            return false;
        }
    } catch (const std::exception& e) {
        if (error_callback) {
            error_callback("ORDER_PLACEMENT_ERROR", e.what());
        }
        std::cout << "❌ Error placing order: " << e.what() << std::endl;
        return false;
    }
}

bool BinanceExchange::cancelOrder(const std::string& symbol, const std::string& order_id) {
    try {
        std::string params = "symbol=" + symbol + "&orderId=" + order_id;
        
        auto response = makeRequest("/order", params, "DELETE", true);
        if (response.response_code == 200) {
            auto data = json::parse(response.data);
            std::cout << "✅ Order canceled: " << order_id << std::endl;
            
            // Trigger callback if set
            if (order_update_callback) {
                LiveOrder canceled_order;
                canceled_order.order_id = order_id;
                canceled_order.symbol = symbol;
                canceled_order.status = "CANCELED";
                order_update_callback(canceled_order);
            }
            
            return true;
        } else {
            if (error_callback) {
                error_callback("ORDER_CANCELLATION_FAILED", response.data);
            }
            std::cout << "❌ Order cancellation failed: " << response.data << std::endl;
            return false;
        }
    } catch (const std::exception& e) {
        if (error_callback) {
            error_callback("ORDER_CANCELLATION_ERROR", e.what());
        }
        std::cout << "❌ Error canceling order: " << e.what() << std::endl;
        return false;
    }
}

std::vector<LiveOrder> BinanceExchange::getActiveOrders(const std::string& symbol) {
    return getOpenOrders(symbol);  // Same implementation for now
}

std::map<std::string, double> BinanceExchange::getBalances() {
    try {
        auto response = makeRequest("/account", "", "GET", true);
        if (response.response_code == 200) {
            auto data = json::parse(response.data);
            std::map<std::string, double> balances;
            
            for (const auto& balance : data["balances"]) {
                std::string asset = balance["asset"].get<std::string>();
                double free = std::stod(balance["free"].get<std::string>());
                double locked = std::stod(balance["locked"].get<std::string>());
                
                if (free > 0 || locked > 0) {
                    balances[asset] = free + locked;
                }
            }
            
            return balances;
        } else {
            if (error_callback) {
                error_callback("BALANCE_QUERY_FAILED", response.data);
            }
            std::cout << "❌ Balance query failed: " << response.data << std::endl;
            return {};
        }
    } catch (const std::exception& e) {
        if (error_callback) {
            error_callback("BALANCE_QUERY_ERROR", e.what());
        }
        std::cout << "❌ Error querying balances: " << e.what() << std::endl;
        return {};
    }
}

// Additional required interface implementations
std::string BinanceExchange::getExchangeName() const {
    return "binance";
}

bool BinanceExchange::subscribeToTickers(const std::vector<std::string>& symbols) {
    std::cout << "📊 Subscribing to tickers for " << symbols.size() << " symbols" << std::endl;
    return true; // Already handled by polling
}

bool BinanceExchange::subscribeToOrderBook(const std::string& symbol, int depth) {
    try {
        std::cout << "📖 Subscribing to order book for " << symbol << " (depth: " << depth << ")" << std::endl;
        
        // For now, store the subscription in memory for WebSocket implementation later
        // This would typically set up a WebSocket connection to stream order book updates
        
        // Make a one-time REST API call to get initial order book data
        std::string params = "symbol=" + symbol + "&limit=" + std::to_string(depth);
        auto response = makeRequest("/depth", params);
        
        if (response.response_code == 200) {
            auto data = json::parse(response.data);
            
            std::cout << "✅ Order book snapshot retrieved for " << symbol 
                      << " (bids: " << data["bids"].size() 
                      << ", asks: " << data["asks"].size() << ")" << std::endl;
            
            // Store in cache for later retrieval
            std::lock_guard<std::mutex> lock(data_mutex);
            // order_book_cache[symbol] would store the order book data
            
            return true;
        } else {
            if (error_callback) {
                error_callback("ORDER_BOOK_SUBSCRIPTION_FAILED", response.data);
            }
            std::cout << "❌ Order book subscription failed: " << response.data << std::endl;
            return false;
        }
    } catch (const std::exception& e) {
        if (error_callback) {
            error_callback("ORDER_BOOK_SUBSCRIPTION_ERROR", e.what());
        }
        std::cout << "❌ Error subscribing to order book: " << e.what() << std::endl;
        return false;
    }
}

bool BinanceExchange::subscribeToTrades(const std::string& symbol) {
    try {
        std::cout << "🔄 Subscribing to trades for " << symbol << std::endl;
        
        // For now, make a REST API call to get recent trades
        std::string params = "symbol=" + symbol + "&limit=100";
        auto response = makeRequest("/trades", params);
        
        if (response.response_code == 200) {
            auto data = json::parse(response.data);
            
            std::cout << "✅ Recent trades retrieved for " << symbol 
                      << " (" << data.size() << " trades)" << std::endl;
            
            // In a full implementation, this would set up WebSocket streaming
            return true;
        } else {
            if (error_callback) {
                error_callback("TRADES_SUBSCRIPTION_FAILED", response.data);
            }
            std::cout << "❌ Trades subscription failed: " << response.data << std::endl;
            return false;
        }
    } catch (const std::exception& e) {
        if (error_callback) {
            error_callback("TRADES_SUBSCRIPTION_ERROR", e.what());
        }
        std::cout << "❌ Error subscribing to trades: " << e.what() << std::endl;
        return false;
    }
}

std::vector<std::string> BinanceExchange::getSupportedSymbols() {
    try {
        auto response = makeRequest("/exchangeInfo");
        if (response.response_code == 200) {
            auto data = json::parse(response.data);
            std::vector<std::string> symbols;
            
            for (const auto& symbol_info : data["symbols"]) {
                if (symbol_info["status"].get<std::string>() == "TRADING") {
                    symbols.push_back(symbol_info["symbol"].get<std::string>());
                }
            }
            
            return symbols;
        } else {
            // Fallback to hardcoded list
            return {"BTCUSDT", "ETHUSDT", "ADAUSDT", "DOTUSDT", "LINKUSDT"};
        }
    } catch (const std::exception& e) {
        // Fallback to hardcoded list
        return {"BTCUSDT", "ETHUSDT", "ADAUSDT", "DOTUSDT", "LINKUSDT"};
    }
}

std::string BinanceExchange::placeLimitOrder(const std::string& symbol, const std::string& side, 
                                            double quantity, double price) {
    try {
        std::string params = "symbol=" + symbol + 
                           "&side=" + side + 
                           "&type=LIMIT" + 
                           "&timeInForce=GTC" +
                           "&quantity=" + std::to_string(quantity) +
                           "&price=" + std::to_string(price);
        
        auto response = makeRequest("/order", params, "POST", true);
        if (response.response_code == 200) {
            auto data = json::parse(response.data);
            std::string order_id = std::to_string(data["orderId"].get<int64_t>());
            
            std::cout << "✅ Limit order placed: " << side << " " << quantity 
                      << " " << symbol << " @ $" << price << " (ID: " << order_id << ")" << std::endl;
            
            // Trigger callback if set
            if (order_update_callback) {
                LiveOrder order;
                order.order_id = order_id;
                order.symbol = symbol;
                order.side = side;
                order.type = "LIMIT";
                order.quantity = quantity;
                order.price = price;
                order.status = data["status"].get<std::string>();
                order_update_callback(order);
            }
            
            return order_id;
        } else {
            if (error_callback) {
                error_callback("LIMIT_ORDER_FAILED", response.data);
            }
            std::cout << "❌ Limit order failed: " << response.data << std::endl;
            return "";
        }
    } catch (const std::exception& e) {
        if (error_callback) {
            error_callback("LIMIT_ORDER_ERROR", e.what());
        }
        std::cout << "❌ Error placing limit order: " << e.what() << std::endl;
        return "";
    }
}

std::string BinanceExchange::placeMarketOrder(const std::string& symbol, const std::string& side, 
                                             double quantity) {
    try {
        std::string params = "symbol=" + symbol + 
                           "&side=" + side + 
                           "&type=MARKET" + 
                           "&quantity=" + std::to_string(quantity);
        
        auto response = makeRequest("/order", params, "POST", true);
        if (response.response_code == 200) {
            auto data = json::parse(response.data);
            std::string order_id = std::to_string(data["orderId"].get<int64_t>());
            
            std::cout << "✅ Market order placed: " << side << " " << quantity 
                      << " " << symbol << " (ID: " << order_id << ")" << std::endl;
            
            // Trigger callback if set
            if (order_update_callback) {
                LiveOrder order;
                order.order_id = order_id;
                order.symbol = symbol;
                order.side = side;
                order.type = "MARKET";
                order.quantity = quantity;
                order.status = data["status"].get<std::string>();
                order_update_callback(order);
            }
            
            return order_id;
        } else {
            if (error_callback) {
                error_callback("MARKET_ORDER_FAILED", response.data);
            }
            std::cout << "❌ Market order failed: " << response.data << std::endl;
            return "";
        }
    } catch (const std::exception& e) {
        if (error_callback) {
            error_callback("MARKET_ORDER_ERROR", e.what());
        }
        std::cout << "❌ Error placing market order: " << e.what() << std::endl;
        return "";
    }
}

bool BinanceExchange::cancelAllOrders(const std::string& symbol) {
    try {
        std::string params = "symbol=" + symbol;
        
        auto response = makeRequest("/openOrders", params, "DELETE", true);
        if (response.response_code == 200) {
            auto data = json::parse(response.data);
            std::cout << "✅ All orders canceled for symbol: " << symbol 
                      << " (" << data.size() << " orders)" << std::endl;
            
            // Trigger callbacks for canceled orders
            if (order_update_callback) {
                for (const auto& order_data : data) {
                    LiveOrder order;
                    order.order_id = std::to_string(order_data["orderId"].get<int64_t>());
                    order.symbol = symbol;
                    order.status = "CANCELED";
                    order_update_callback(order);
                }
            }
            
            return true;
        } else {
            if (error_callback) {
                error_callback("CANCEL_ALL_ORDERS_FAILED", response.data);
            }
            std::cout << "❌ Cancel all orders failed for symbol: " << symbol 
                      << " - " << response.data << std::endl;
            return false;
        }
    } catch (const std::exception& e) {
        if (error_callback) {
            error_callback("CANCEL_ALL_ORDERS_ERROR", e.what());
        }
        std::cout << "❌ Error canceling all orders for symbol: " << symbol 
                  << " - " << e.what() << std::endl;
        return false;
    }
}

std::vector<LiveOrder> BinanceExchange::getOpenOrders(const std::string& symbol) {
    try {
        std::string params = symbol.empty() ? "" : "symbol=" + symbol;
        
        auto response = makeRequest("/openOrders", params, "GET", true);
        if (response.response_code == 200) {
            auto data = json::parse(response.data);
            std::vector<LiveOrder> orders;
            
            for (const auto& order_data : data) {
                LiveOrder order;
                order.order_id = std::to_string(order_data["orderId"].get<int64_t>());
                order.client_order_id = order_data["clientOrderId"].get<std::string>();
                order.symbol = order_data["symbol"].get<std::string>();
                order.side = order_data["side"].get<std::string>();
                order.type = order_data["type"].get<std::string>();
                order.status = order_data["status"].get<std::string>();
                order.quantity = std::stod(order_data["origQty"].get<std::string>());
                order.price = std::stod(order_data["price"].get<std::string>());
                order.filled_quantity = std::stod(order_data["executedQty"].get<std::string>());
                order.remaining_quantity = order.quantity - order.filled_quantity;
                order.timestamp = order_data["time"].get<int64_t>();
                order.update_time = order_data["updateTime"].get<int64_t>();
                
                orders.push_back(order);
            }
            
            return orders;
        } else {
            if (error_callback) {
                error_callback("GET_OPEN_ORDERS_FAILED", response.data);
            }
            std::cout << "❌ Get open orders failed: " << response.data << std::endl;
            return {};
        }
    } catch (const std::exception& e) {
        if (error_callback) {
            error_callback("GET_OPEN_ORDERS_ERROR", e.what());
        }
        std::cout << "❌ Error getting open orders: " << e.what() << std::endl;
        return {};
    }
}

LiveOrder BinanceExchange::getOrderStatus(const std::string& symbol, const std::string& order_id) {
    try {
        std::string params = "symbol=" + symbol + "&orderId=" + order_id;
        
        auto response = makeRequest("/order", params, "GET", true);
        if (response.response_code == 200) {
            auto data = json::parse(response.data);
            
            LiveOrder order;
            order.order_id = std::to_string(data["orderId"].get<int64_t>());
            order.client_order_id = data["clientOrderId"].get<std::string>();
            order.symbol = data["symbol"].get<std::string>();
            order.side = data["side"].get<std::string>();
            order.type = data["type"].get<std::string>();
            order.status = data["status"].get<std::string>();
            order.quantity = std::stod(data["origQty"].get<std::string>());
            order.price = std::stod(data["price"].get<std::string>());
            order.filled_quantity = std::stod(data["executedQty"].get<std::string>());
            order.remaining_quantity = order.quantity - order.filled_quantity;
            order.timestamp = data["time"].get<int64_t>();
            order.update_time = data["updateTime"].get<int64_t>();
            
            if (data.contains("fills") && data["fills"].size() > 0) {
                double total_commission = 0;
                for (const auto& fill : data["fills"]) {
                    total_commission += std::stod(fill["commission"].get<std::string>());
                    if (order.commission_asset.empty()) {
                        order.commission_asset = fill["commissionAsset"].get<std::string>();
                    }
                }
                order.commission = total_commission;
            }
            
            return order;
        } else {
            if (error_callback) {
                error_callback("GET_ORDER_STATUS_FAILED", response.data);
            }
            std::cout << "❌ Get order status failed: " << response.data << std::endl;
            return LiveOrder{};
        }
    } catch (const std::exception& e) {
        if (error_callback) {
            error_callback("GET_ORDER_STATUS_ERROR", e.what());
        }
        std::cout << "❌ Error getting order status for: " << order_id 
                  << " - " << e.what() << std::endl;
        return LiveOrder{};
    }
}

std::vector<AccountBalance> BinanceExchange::getAccountBalances() {
    try {
        auto response = makeRequest("/account", "", "GET", true);
        if (response.response_code == 200) {
            auto data = json::parse(response.data);
            std::vector<AccountBalance> balances;
            
            for (const auto& balance : data["balances"]) {
                std::string asset = balance["asset"].get<std::string>();
                double free = std::stod(balance["free"].get<std::string>());
                double locked = std::stod(balance["locked"].get<std::string>());
                
                if (free > 0 || locked > 0) {
                    AccountBalance account_balance;
                    account_balance.asset = asset;
                    account_balance.free = free;
                    account_balance.locked = locked;
                    account_balance.total = free + locked;
                    balances.push_back(account_balance);
                }
            }
            
            return balances;
        } else {
            if (error_callback) {
                error_callback("GET_ACCOUNT_BALANCES_FAILED", response.data);
            }
            std::cout << "❌ Account balances query failed: " << response.data << std::endl;
            return {};
        }
    } catch (const std::exception& e) {
        if (error_callback) {
            error_callback("GET_ACCOUNT_BALANCES_ERROR", e.what());
        }
        std::cout << "❌ Error querying account balances: " << e.what() << std::endl;
        return {};
    }
}

AccountBalance BinanceExchange::getAssetBalance(const std::string& asset) {
    try {
        auto response = makeRequest("/account", "", "GET", true);
        if (response.response_code == 200) {
            auto data = json::parse(response.data);
            
            for (const auto& balance : data["balances"]) {
                if (balance["asset"].get<std::string>() == asset) {
                    AccountBalance account_balance;
                    account_balance.asset = asset;
                    account_balance.free = std::stod(balance["free"].get<std::string>());
                    account_balance.locked = std::stod(balance["locked"].get<std::string>());
                    account_balance.total = account_balance.free + account_balance.locked;
                    return account_balance;
                }
            }
            
            // Asset not found, return empty balance
            AccountBalance empty_balance;
            empty_balance.asset = asset;
            return empty_balance;
        } else {
            if (error_callback) {
                error_callback("GET_ASSET_BALANCE_FAILED", response.data);
            }
            std::cout << "❌ Asset balance query failed for: " << asset 
                      << " - " << response.data << std::endl;
            return AccountBalance{};
        }
    } catch (const std::exception& e) {
        if (error_callback) {
            error_callback("GET_ASSET_BALANCE_ERROR", e.what());
        }
        std::cout << "❌ Error querying asset balance for: " << asset 
                  << " - " << e.what() << std::endl;
        return AccountBalance{};
    }
}

double BinanceExchange::getAvailableBalance(const std::string& asset) {
    try {
        auto response = makeRequest("/account", "", "GET", true);
        if (response.response_code == 200) {
            auto data = json::parse(response.data);
            
            for (const auto& balance : data["balances"]) {
                if (balance["asset"].get<std::string>() == asset) {
                    return std::stod(balance["free"].get<std::string>());
                }
            }
            
            // Asset not found
            return 0.0;
        } else {
            if (error_callback) {
                error_callback("GET_AVAILABLE_BALANCE_FAILED", response.data);
            }
            std::cout << "❌ Available balance query failed for: " << asset 
                      << " - " << response.data << std::endl;
            return 0.0;
        }
    } catch (const std::exception& e) {
        if (error_callback) {
            error_callback("GET_AVAILABLE_BALANCE_ERROR", e.what());
        }
        std::cout << "❌ Error querying available balance for: " << asset 
                  << " - " << e.what() << std::endl;
        return 0.0;
    }
}

int BinanceExchange::getRateLimitRemaining() const {
    return 1200 - requests_per_minute.load(); // Binance limit is 1200/min
}

double BinanceExchange::getLatencyMs() const {
    try {
        auto start = std::chrono::high_resolution_clock::now();
        
        // Make a simple ping request to measure latency
        auto response = const_cast<BinanceExchange*>(this)->makeRequest("/ping");
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        if (response.response_code == 200) {
            return static_cast<double>(duration.count());
        } else {
            return 999.0; // High latency if connection failed
        }
    } catch (const std::exception& e) {
        return 999.0; // High latency on error
    }
}

std::map<std::string, double> BinanceExchange::getTradingFees() {
    try {
        auto response = makeRequest("/account", "", "GET", true);
        if (response.response_code == 200) {
            auto data = json::parse(response.data);
            
            // Extract maker and taker fees from account info
            double maker_commission = data["makerCommission"].get<int>() / 10000.0; // Convert from basis points
            double taker_commission = data["takerCommission"].get<int>() / 10000.0;
            
            return {
                {"maker", maker_commission},
                {"taker", taker_commission},
                {"buyer_commission", data["buyerCommission"].get<int>() / 10000.0},
                {"seller_commission", data["sellerCommission"].get<int>() / 10000.0}
            };
        } else {
            // Fallback to default fees if API call fails
            return {{"maker", 0.001}, {"taker", 0.001}}; // 0.1% default fees
        }
    } catch (const std::exception& e) {
        // Fallback to default fees
        return {{"maker", 0.001}, {"taker", 0.001}}; // 0.1% default fees
    }
}

std::map<std::string, double> BinanceExchange::getMinOrderSizes() {
    try {
        auto response = makeRequest("/exchangeInfo");
        if (response.response_code == 200) {
            auto data = json::parse(response.data);
            std::map<std::string, double> min_sizes;
            
            for (const auto& symbol_info : data["symbols"]) {
                std::string symbol = symbol_info["symbol"].get<std::string>();
                
                // Find LOT_SIZE filter for minimum quantity
                for (const auto& filter : symbol_info["filters"]) {
                    if (filter["filterType"].get<std::string>() == "LOT_SIZE") {
                        min_sizes[symbol] = std::stod(filter["minQty"].get<std::string>());
                        break;
                    }
                }
            }
            
            return min_sizes;
        } else {
            // Fallback to hardcoded values
            return {
                {"BTCUSDT", 0.00001}, 
                {"ETHUSDT", 0.0001}, 
                {"ADAUSDT", 1.0},
                {"DOTUSDT", 0.1},
                {"LINKUSDT", 0.1}
            };
        }
    } catch (const std::exception& e) {
        // Fallback to hardcoded values
        return {
            {"BTCUSDT", 0.00001}, 
            {"ETHUSDT", 0.0001}, 
            {"ADAUSDT", 1.0},
            {"DOTUSDT", 0.1},
            {"LINKUSDT", 0.1}
        };
    }
}

void BinanceExchange::setOrderUpdateCallback(std::function<void(const LiveOrder&)> callback) {
    order_update_callback = callback;
}

void BinanceExchange::setTickerUpdateCallback(std::function<void(const LiveTickData&)> callback) {
    tick_callback = callback;
}

void BinanceExchange::setErrorCallback(std::function<void(const std::string&, const std::string&)> callback) {
    error_callback = callback;
}
