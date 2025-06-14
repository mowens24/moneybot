#ifndef NETWORK_H
#define NETWORK_H

#include "logger.h"
#include "order_book.h"
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/beast/ssl.hpp>
#include <memory>
#include <string>

namespace moneybot {
    class Network {
    public:
        Network(std::shared_ptr<Logger> logger, std::shared_ptr<OrderBook> order_book);
        ~Network();
        void pingExchange(const std::string& url);
        void connectWebSocket(const std::string& host, const std::string& port,
                              const std::string& endpoint);
        void run();
        void stop();
    private:
        void processData(const boost::system::error_code& ec, std::size_t bytes_transferred);
        std::shared_ptr<Logger> logger_;
        std::shared_ptr<OrderBook> order_book_;
        boost::asio::io_context ioc_;
        boost::asio::ssl::context ssl_ctx_;
        std::unique_ptr<boost::beast::websocket::stream<boost::beast::ssl_stream<boost::beast::tcp_stream>>> ws_;
        boost::beast::multi_buffer buffer_;
    };
} // namespace moneybot

#endif // NETWORK_H