#pragma once

#include "logger.h"
#include "order_book.h"
#include <boost/asio.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/beast.hpp>
#include <boost/beast/ssl.hpp>
#include <memory>
#include <string>
#include <nlohmann/json.hpp>

namespace moneybot {
    namespace net = boost::asio;
    namespace beast = boost::beast;
    using json = nlohmann::json;

    class Network : public std::enable_shared_from_this<Network> {
    public:
        Network(std::shared_ptr<Logger> logger, std::shared_ptr<OrderBook> order_book, const json& config);
        ~Network();
        void pingExchange(const std::string& url);
        void run(const std::string& host, const std::string& port,
                 const std::string& endpoint);
        void stop();

    private:
        static net::awaitable<void>
        connectWebSocket(std::shared_ptr<Network> self, const std::string& host, const std::string& port,
                         const std::string& endpoint);
        void processMessage(const json& message);
        std::shared_ptr<Logger> logger_;
        std::shared_ptr<OrderBook> order_book_;
        const json& config_;
        net::io_context ioc_;
        net::ssl::context ssl_ctx_;
        std::unique_ptr<beast::websocket::stream<beast::ssl_stream<beast::tcp_stream>>> ws_;
        beast::multi_buffer buffer_;
        net::strand<net::any_io_executor> strand_;
    };
} // namespace moneybot 