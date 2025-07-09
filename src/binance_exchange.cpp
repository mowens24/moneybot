#include "binance_exchange.h"
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
    : api_key(api_key), secret_key(secret_key), use_testnet(testnet) {
    
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
        std::cout << "🔌 Connecting to Binance REST API..." << std::endl;
        
        // Test REST API connection
        auto response = makeRequest("/ping");
        if (response.response_code != 200) {
            std::cout << "❌ Binance REST API not reachable" << std::endl;
            return false;
        }
        
        // Start polling for market data
        polling_active = true;
        polling_thread = std::thread(&BinanceExchange::pollMarketData, this);
        
        std::cout << "✅ Connected to Binance exchange" << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        std::cout << "❌ Binance connection error: " << e.what() << std::endl;
        return false;
    }
}

bool BinanceExchange::disconnect() {
    try {
        polling_active = false;
        
        if (polling_thread.joinable()) {
            polling_thread.join();
        }
        
        std::cout << "🔌 Disconnected from Binance" << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cout << "❌ Error disconnecting from Binance: " << e.what() << std::endl;
        return false;
    }
}

void BinanceExchange::pollMarketData() {
    std::vector<std::string> symbols = {"BTCUSDT", "ETHUSDT", "ADAUSDT", "DOTUSDT", "LINKUSDT"};
    
    while (polling_active) {
        try {
            // Get 24hr ticker statistics for all symbols
            auto response = makeRequest("/ticker/24hr");
            if (response.response_code == 200) {
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
                        
                        // Print live data update
                        double spread_bps = ((tick.ask_price - tick.bid_price) / tick.bid_price) * 10000;
                        std::cout << "🚀 LIVE " << tick.exchange << " " << tick.symbol 
                                  << " | Bid: $" << std::fixed << std::setprecision(2) << tick.bid_price
                                  << " | Ask: $" << tick.ask_price 
                                  << " | Last: $" << tick.last_price
                                  << " | Spread: " << std::setprecision(2) << spread_bps << " bps"
                                  << " | Vol: " << std::setprecision(0) << tick.volume_24h << std::endl;
                        
                        // Trigger callback if set
                        if (tick_callback) {
                            tick_callback(tick);
                        }
                    }
                }
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

// Placeholder implementations for order management (to be implemented later)
bool BinanceExchange::placeOrder(const LiveOrder& order) {
    std::cout << "📝 Order placement not yet implemented for live trading" << std::endl;
    return false;
}

bool BinanceExchange::cancelOrder(const std::string& symbol, const std::string& order_id) {
    std::cout << "❌ Order cancellation not yet implemented for live trading" << std::endl;
    return false;
}

std::vector<LiveOrder> BinanceExchange::getActiveOrders(const std::string& symbol) {
    std::cout << "📋 Active orders query not yet implemented" << std::endl;
    return {};
}

std::map<std::string, double> BinanceExchange::getBalances() {
    std::cout << "💰 Balance query not yet implemented" << std::endl;
    return {};
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
    std::cout << "📖 Subscribing to order book for " << symbol << " (depth: " << depth << ")" << std::endl;
    return true; // TODO: Implement WebSocket order book
}

bool BinanceExchange::subscribeToTrades(const std::string& symbol) {
    std::cout << "🔄 Subscribing to trades for " << symbol << std::endl;
    return true; // TODO: Implement WebSocket trades
}

std::vector<std::string> BinanceExchange::getSupportedSymbols() {
    return {"BTCUSDT", "ETHUSDT", "ADAUSDT", "DOTUSDT", "LINKUSDT"};
}

std::string BinanceExchange::placeLimitOrder(const std::string& symbol, const std::string& side, 
                                            double quantity, double price) {
    std::cout << "📝 Limit order placement not yet implemented: " 
              << side << " " << quantity << " " << symbol << " @ $" << price << std::endl;
    return "";
}

std::string BinanceExchange::placeMarketOrder(const std::string& symbol, const std::string& side, 
                                             double quantity) {
    std::cout << "📝 Market order placement not yet implemented: " 
              << side << " " << quantity << " " << symbol << std::endl;
    return "";
}

bool BinanceExchange::cancelAllOrders(const std::string& symbol) {
    std::cout << "❌ Cancel all orders not yet implemented for symbol: " << symbol << std::endl;
    return false;
}

std::vector<LiveOrder> BinanceExchange::getOpenOrders(const std::string& symbol) {
    std::cout << "📋 Get open orders not yet implemented for symbol: " << symbol << std::endl;
    return {};
}

LiveOrder BinanceExchange::getOrderStatus(const std::string& symbol, const std::string& order_id) {
    std::cout << "🔍 Get order status not yet implemented: " << order_id << std::endl;
    return LiveOrder{};
}

std::vector<AccountBalance> BinanceExchange::getAccountBalances() {
    std::cout << "💰 Account balances query not yet implemented" << std::endl;
    return {};
}

AccountBalance BinanceExchange::getAssetBalance(const std::string& asset) {
    std::cout << "💰 Asset balance query not yet implemented for: " << asset << std::endl;
    return AccountBalance{};
}

double BinanceExchange::getAvailableBalance(const std::string& asset) {
    std::cout << "💰 Available balance query not yet implemented for: " << asset << std::endl;
    return 0.0;
}

int BinanceExchange::getRateLimitRemaining() const {
    return 1200 - requests_per_minute.load(); // Binance limit is 1200/min
}

double BinanceExchange::getLatencyMs() const {
    return 15.0; // TODO: Measure actual latency
}

std::map<std::string, double> BinanceExchange::getTradingFees() {
    return {{"maker", 0.001}, {"taker", 0.001}}; // 0.1% default fees
}

std::map<std::string, double> BinanceExchange::getMinOrderSizes() {
    return {{"BTCUSDT", 0.00001}, {"ETHUSDT", 0.0001}, {"ADAUSDT", 1.0}};
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
