#include "crypto/binance_parser.hpp"

#include <cassert>
#include <chrono>
#include <string>

int main() {
    const std::string payload = R"({"e":"depthUpdate","E":1672515782136,"s":"BTCUSDT","U":157,"u":160,"b":[["100.00","1.50"],["99.00","0.00"]],"a":[["101.00","2.00"]]})";
    crypto::DepthUpdate update;
    const auto received_at = std::chrono::steady_clock::now();
    assert(crypto::parse_binance_depth(payload, update, received_at));
    assert(update.symbol == "BTCUSDT");
    assert(update.first_update_id == 157);
    assert(update.final_update_id == 160);
    assert(update.bids.size() == 2);
    assert(update.bids[0].price == 100.0);
    assert(update.asks[0].quantity == 2.0);
    assert(update.received_at == received_at);

    crypto::DepthUpdate invalid;
    assert(!crypto::parse_binance_depth(R"({"e":"trade","s":"BTCUSDT"})", invalid, received_at));
    assert(!crypto::parse_binance_depth(R"({"e":"depthUpdate","E":1,"s":"BTCUSDT","U":9,"u":8,"b":[],"a":[]})", invalid, received_at));
    assert(!crypto::parse_binance_depth(R"({"e":"depthUpdate","E":1,"s":"BTCUSDT","U":9,"u":10,"b":[["bad","1"]],"a":[]})", invalid, received_at));
    return 0;
}