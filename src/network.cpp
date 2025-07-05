#include "network.h"
#include "order_book.h"
#include "types.h"
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/beast/ssl.hpp>
#include <nlohmann/json.hpp>

namespace moneybot {
    namespace ssl = boost::asio::ssl;
    using json = nlohmann::json;
    using boost::asio::awaitable;
    using boost::asio::co_spawn;
    using boost::asio::detached;
    using boost::asio::use_awaitable;
    namespace this_coro = boost::asio::this_coro;

    Network::Network(std::shared_ptr<Logger> logger, std::shared_ptr<OrderBook> order_book, const json& config)
        : logger_(logger), order_book_(order_book), config_(config), ioc_(), ssl_ctx_(ssl::context::tlsv12_client),
          strand_(ioc_.get_executor()) {
        ssl_ctx_.set_default_verify_paths();
        // ssl_ctx_.set_verify_mode(ssl::verify_peer); // Production
        ssl_ctx_.set_verify_mode(ssl::verify_none); // For testing
        logger_->getLogger()->info("Network initialized.");
    }

    Network::~Network() {
        stop();
    }

    void Network::pingExchange(const std::string& url) {
        logger_->getLogger()->info("Pinged exchange: {}", url);
    }

    awaitable<void> Network::connectWebSocket(std::shared_ptr<Network> self, const std::string& host,
                                              const std::string& port, const std::string& endpoint) {
        auto logger = self->logger_->getLogger();
        auto& ws = self->ws_;
        auto& order_book = self->order_book_;
        auto& buffer = self->buffer_;

        co_await net::post(self->strand_, use_awaitable);

        try {
            logger->debug("Attempting WebSocket connection to host: {}, port: {}, endpoint: {}", host, port, endpoint);

            ws = std::make_unique<beast::websocket::stream<beast::ssl_stream<beast::tcp_stream>>>(
                co_await this_coro::executor, self->ssl_ctx_);

            ws->set_option(beast::websocket::stream_base::decorator([&self](beast::websocket::request_type& req) {
                req.set(beast::http::field::user_agent, self->config_["exchange"]["user_agent"].get<std::string>());
            }));

            if (!SSL_set_tlsext_host_name(ws->next_layer().native_handle(), host.c_str())) {
                throw beast::system_error{
                    beast::error_code{static_cast<int>(::ERR_get_error()), net::error::get_ssl_category()},
                    "Failed to set SNI"};
            }

            net::ip::tcp::resolver resolver(co_await this_coro::executor);
            auto results = co_await resolver.async_resolve(host, port, use_awaitable);
            co_await beast::get_lowest_layer(ws->next_layer()).async_connect(results, use_awaitable);
            logger->info("TCP connected to {}:{}", host, port);

            co_await ws->next_layer().async_handshake(ssl::stream_base::client, use_awaitable);
            logger->info("SSL handshake successful.");

            ws->control_callback([self, &ws](beast::websocket::frame_type kind, boost::string_view payload) {
                if (kind == beast::websocket::frame_type::ping) {
                    self->logger_->getLogger()->debug("Received ping frame: {}",
                                                  std::string(payload.data(), payload.size()));
                    beast::websocket::ping_data ping_payload;
                    if (!payload.empty()) {
                        ping_payload.assign(payload.data(), std::min(payload.size(), ping_payload.max_size()));
                    }
                    ws->async_pong(ping_payload, [self](boost::system::error_code ec) {
                        if (ec) {
                            self->logger_->getLogger()->error("Failed to send pong: {}", ec.message());
                        }
                    });
                } else if (kind == beast::websocket::frame_type::pong) {
                    self->logger_->getLogger()->debug("Received pong frame: {}",
                                                  std::string(payload.data(), payload.size()));
                }
            });

            co_await ws->async_handshake(host, endpoint, use_awaitable);
            logger->info("WebSocket handshake successful.");

            // Only send subscription message if using combined stream endpoint
            if (endpoint.rfind("/stream", 0) == 0) {
                json subscribe = self->config_["exchange"]["websocket_subscription"];
                co_await ws->async_write(net::buffer(subscribe.dump()), use_awaitable);
                logger->info("Subscribed to streams");
            }

            for (;;) {
                buffer.clear();
                co_await ws->async_read(buffer, use_awaitable);
                auto data = beast::buffers_to_string(buffer.data());
                logger->debug("Received data: {}", data);
                try {
                    json j = json::parse(data);
                    // Handle combined stream payloads
                    if (j.contains("stream") && j.contains("data")) {
                        self->processMessage(j["data"]);
                    } else {
                        self->processMessage(j);
                    }
                } catch (const std::exception& e) {
                    logger->error("Failed to process data: {}", e.what());
                }
            }
        } catch (const boost::system::system_error& e) {
            if (e.code() == net::error::operation_aborted) {
                logger->info("WebSocket operation canceled.");
            } else {
                logger->error("WebSocket connection failed (system error): {}", e.what());
            }
        } catch (const std::exception& e) {
            logger->error("WebSocket connection failed: {}", e.what());
        }
    }

    void Network::processMessage(const json& message) {
        try {
            // Handle different message types
            if (message.contains("e")) {
                std::string event_type = message["e"].get<std::string>();
                
                if (event_type == "depthUpdate") {
                    // Order book update
                    order_book_->update(message);
                } else if (event_type == "trade") {
                    // Trade update
                    Trade trade(message);
                    // Notify strategy about trade
                    logger_->getLogger()->debug("Trade: {} {} @ {}", 
                                              trade.symbol, trade.quantity, trade.price);
                } else if (event_type == "executionReport") {
                    // Order execution report
                    logger_->getLogger()->debug("Order execution: {}", message.dump());
                } else if (event_type == "outboundAccountPosition") {
                    // Account position update
                    logger_->getLogger()->debug("Account position update: {}", message.dump());
                } else {
                    logger_->getLogger()->debug("Unknown event type: {}", event_type);
                }
            } else if (message.contains("result") || message.contains("id")) {
                // Response to subscription or other requests
                logger_->getLogger()->debug("Response message: {}", message.dump());
            } else {
                // Unknown message format
                logger_->getLogger()->debug("Unknown message format: {}", message.dump());
            }
        } catch (const std::exception& e) {
            logger_->getLogger()->error("Error processing message: {}", e.what());
        }
    }

    void Network::run(const std::string& host, const std::string& port, const std::string& endpoint) {
        co_spawn(
            ioc_, [self = shared_from_this(), host, port, endpoint]() {
                return connectWebSocket(self, host, port, endpoint);
            },
            detached);
        ioc_.run();
    }

    void Network::runUserDataStream(const std::string& listenKey) {
        using namespace std::chrono_literals;
        std::string host = "stream.binance.us";
        std::string port = "9443";
        std::string endpoint = "/ws/" + listenKey;
        auto logger = logger_->getLogger();
        auto& ws = ws_;
        auto& buffer = buffer_;

        co_spawn(ioc_, [self = shared_from_this(), host, port, endpoint, logger, &ws, &buffer]() -> net::awaitable<void> {
            try {
                ws = std::make_unique<beast::websocket::stream<beast::ssl_stream<beast::tcp_stream>>>(
                    co_await this_coro::executor, self->ssl_ctx_);
                ws->set_option(beast::websocket::stream_base::decorator([&self](beast::websocket::request_type& req) {
                    req.set(beast::http::field::user_agent, "MoneyBot/1.0");
                }));
                if (!SSL_set_tlsext_host_name(ws->next_layer().native_handle(), host.c_str())) {
                    throw beast::system_error{
                        beast::error_code{static_cast<int>(::ERR_get_error()), net::error::get_ssl_category()},
                        "Failed to set SNI"};
                }
                net::ip::tcp::resolver resolver(co_await this_coro::executor);
                auto results = co_await resolver.async_resolve(host, port, use_awaitable);
                co_await beast::get_lowest_layer(ws->next_layer()).async_connect(results, use_awaitable);
                logger->info("TCP connected to {}:{} (user data stream)", host, port);
                co_await ws->next_layer().async_handshake(ssl::stream_base::client, use_awaitable);
                logger->info("SSL handshake successful (user data stream)");
                co_await ws->async_handshake(host, endpoint, use_awaitable);
                logger->info("WebSocket handshake successful (user data stream)");
                for (;;) {
                    buffer.clear();
                    co_await ws->async_read(buffer, use_awaitable);
                    auto data = beast::buffers_to_string(buffer.data());
                    logger->debug("[UserData] Received data: {}", data);
                    try {
                        json j = json::parse(data);
                        // Route to order/account update handlers if present
                        if (j.contains("e")) {
                            std::string event_type = j["e"].get<std::string>();
                            if (event_type == "executionReport") {
                                if (self->order_manager_)
                                    self->order_manager_->handleOrderUpdate(j);
                            } else if (event_type == "outboundAccountPosition") {
                                if (self->order_manager_)
                                    self->order_manager_->handleAccountUpdate(j);
                            }
                        }
                    } catch (const std::exception& e) {
                        logger->error("[UserData] Failed to process data: {}", e.what());
                    }
                }
            } catch (const std::exception& e) {
                logger->error("User data WebSocket connection failed: {}", e.what());
            }
        }, detached);
        ioc_.run();
    }

    void Network::stop() {
        net::post(ioc_, [this]() {
            if (ws_) {
                boost::system::error_code ec;
                ws_->close(beast::websocket::close_code::normal, ec);
                logger_->getLogger()->info("WebSocket closed: {}", ec ? ec.message() : "success");
                ws_.reset();
            }
        });
        if (ioc_.stopped())
            return;
        ioc_.stop();
    }
} // namespace moneybot