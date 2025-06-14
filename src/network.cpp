#include "network.h"
#include "order_book.h"
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/beast/ssl.hpp>
#include <nlohmann/json.hpp>

namespace moneybot {
    namespace net = boost::asio;
    namespace beast = boost::beast;
    namespace ssl = boost::asio::ssl;
    using json = nlohmann::json;

    Network::Network(std::shared_ptr<Logger> logger, std::shared_ptr<OrderBook> order_book)
        : logger_(logger), order_book_(order_book), ioc_(), ssl_ctx_(ssl::context::tlsv12_client) {
        ssl_ctx_.set_default_verify_paths();
        ssl_ctx_.set_verify_mode(ssl::verify_none); // For testing
        logger_->getLogger()->info("Network initialized.");
    }

    Network::~Network() {
        stop();
    }

    void Network::pingExchange(const std::string& url) {
        logger_->getLogger()->info("Pinged exchange: {}", url);
        // Implement HTTP ping if needed
    }

    void Network::connectWebSocket(const std::string& host, const std::string& port,
                                   const std::string& endpoint) {
        try {
            ws_ = std::make_unique<beast::websocket::stream<beast::ssl_stream<beast::tcp_stream>>>(
                ioc_, ssl_ctx_);
            if (!SSL_set_tlsext_host_name(ws_->next_layer().native_handle(), host.c_str())) {
                throw beast::system_error{
                    beast::error_code{static_cast<int>(::ERR_get_error()), net::error::get_ssl_category()},
                    "Failed to set SNI"};
            }
            net::ip::tcp::resolver resolver(ioc_);
            auto results = resolver.resolve(host, port);
            beast::get_lowest_layer(ws_->next_layer()).connect(results);
            logger_->getLogger()->info("TCP connected to {}:{}", host, port);
            ws_->next_layer().async_handshake(
                ssl::stream_base::client,
                [this, host, endpoint](const boost::system::error_code& ec) { // Capture host, endpoint
                    if (ec) {
                        logger_->getLogger()->error("SSL handshake failed: {}", ec.message());
                        return;
                    }
                    logger_->getLogger()->info("SSL handshake successful.");
                    ws_->async_handshake(host, endpoint,
                        [this](const boost::system::error_code& ec) {
                            if (ec) {
                                logger_->getLogger()->error("WebSocket handshake failed: {}", ec.message());
                                return;
                            }
                            logger_->getLogger()->info("WebSocket handshake successful.");
                            ws_->async_read(buffer_,
                                [this](const boost::system::error_code& ec, std::size_t bytes_transferred) {
                                    processData(ec, bytes_transferred);
                                });
                        });
                });
        } catch (const std::exception& e) {
            logger_->getLogger()->error("WebSocket connection failed: {}", e.what());
        }
    }

    void Network::processData(const boost::system::error_code& ec, std::size_t bytes_transferred) {
        if (ec) {
            logger_->getLogger()->error("WebSocket read failed: {}", ec.message());
            stop();
            return;
        }
        try {
            auto data = beast::buffers_to_string(buffer_.data());
            json j = json::parse(data);
            order_book_->update(j); // Pass depth data to OrderBook
            buffer_.clear();
            ws_->async_read(buffer_,
                [this](const boost::system::error_code& ec, std::size_t bytes_transferred) {
                    processData(ec, bytes_transferred);
                });
        } catch (const std::exception& e) {
            logger_->getLogger()->error("Failed to process data: {}", e.what());
            buffer_.clear();
            ws_->async_read(buffer_,
                [this](const boost::system::error_code& ec, std::size_t bytes_transferred) {
                    processData(ec, bytes_transferred);
                });
        }
    }

    void Network::run() {
        ioc_.run();
    }

    void Network::stop() {
        if (ws_) {
            boost::system::error_code ec;
            ws_->close(beast::websocket::close_code::normal, ec);
            logger_->getLogger()->info("WebSocket closed: {}", ec ? ec.message() : "success");
            ws_.reset();
        }
        ioc_.stop();
    }
} // namespace moneybot