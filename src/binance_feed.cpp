#include "crypto/binance_feed.hpp"

#include "crypto/binance_parser.hpp"
#include "crypto/logger.hpp"

#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>
#include <openssl/ssl.h>

#include <algorithm>
#include <chrono>
#include <cctype>
#include <sstream>
#include <string>
#include <thread>

namespace crypto {
namespace {

namespace asio = boost::asio;
namespace beast = boost::beast;
namespace websocket = beast::websocket;
using tcp = asio::ip::tcp;
using TlsWebSocket = websocket::stream<beast::ssl_stream<beast::tcp_stream>>;

std::string lower_case(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](const unsigned char character) {
        return static_cast<char>(std::tolower(character));
    });
    return value;
}

std::string stream_target(const Config& config) {
    std::ostringstream target;
    target << "/stream?streams=";
    for (std::size_t index = 0; index < config.symbols.size(); ++index) {
        if (index != 0) {
            target << '/';
        }
        target << lower_case(config.symbols[index]) << "@depth@100ms";
    }
    return target.str();
}

void connect_and_read(const Config& config, SpscQueue<DepthUpdate, 4096>& updates,
                      const std::atomic_bool& stop_requested, Metrics* metrics) {
    asio::io_context io_context;
    asio::ssl::context tls_context{asio::ssl::context::tls_client};
    tls_context.set_default_verify_paths();
    tls_context.set_verify_mode(asio::ssl::verify_peer);

    tcp::resolver resolver{io_context};
    const auto endpoints = resolver.resolve("stream.binance.com", "9443");
    TlsWebSocket websocket{io_context, tls_context};
    websocket.next_layer().set_verify_callback(asio::ssl::host_name_verification("stream.binance.com"));
    websocket.next_layer().set_verify_mode(asio::ssl::verify_peer);
    if (SSL_set_tlsext_host_name(websocket.next_layer().native_handle(), "stream.binance.com") != 1) {
        throw beast::system_error(
            beast::error_code(static_cast<int>(::ERR_get_error()), asio::error::get_ssl_category()),
            "unable to set Binance TLS SNI");
    }

    beast::get_lowest_layer(websocket).connect(endpoints);
    websocket.next_layer().handshake(asio::ssl::stream_base::client);
    websocket.handshake("stream.binance.com", stream_target(config));
    websocket.binary(false);
    websocket.control_callback([](websocket::frame_type, beast::string_view) {});
    log(LogLevel::info, "feed", "connected to Binance depth streams");

    while (!stop_requested.load()) {
        beast::flat_buffer buffer;
        websocket.read(buffer);
        const auto payload = beast::buffers_to_string(buffer.data());
        DepthUpdate update;
        if (!parse_binance_depth(payload, update, std::chrono::steady_clock::now())) {
            if (metrics != nullptr) {
                metrics->parse_failure("unknown");
            }
            log(LogLevel::warning, "feed", "discarded invalid Binance depth message");
            continue;
        }
        if (metrics != nullptr) {
            metrics->feed_message(update.symbol);
        }
        const auto symbol = update.symbol;
        if (!updates.try_push(std::move(update))) {
            if (metrics != nullptr) {
                metrics->queue_overflow(symbol);
            }
            log(LogLevel::warning, "feed", "depth update queue is full");
        }
    }

    beast::error_code close_error;
    websocket.close(websocket::close_code::normal, close_error);
}

}

void BinanceFeed::run(const Config& config, SpscQueue<DepthUpdate, 4096>& updates,
                      const std::atomic_bool& stop_requested, Metrics* metrics) const {
    while (!stop_requested.load()) {
        try {
            connect_and_read(config, updates, stop_requested, metrics);
        } catch (const std::exception& error) {
            log(LogLevel::warning, "feed", std::string{"Binance connection stopped: "} + error.what());
            if (!stop_requested.load()) {
                log(LogLevel::info, "feed", "reconnecting in two seconds");
                std::this_thread::sleep_for(std::chrono::seconds(2));
            }
        }
    }
}

}