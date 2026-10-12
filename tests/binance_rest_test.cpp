#include "crypto/binance_rest.hpp"

#include <cassert>
#include <string>

int main() {
    const std::string response = R"({
        "lastUpdateId": 100,
        "bids": [["100.00", "2.50"], ["99.80", "1.00"]],
        "asks": [["101.00", "3.20"], ["101.20", "0.50"]]
    })";

    crypto::BookSnapshot snapshot;
    assert(crypto::parse_snapshot_json("BTCUSDT", response, snapshot));
    assert(snapshot.symbol == "BTCUSDT");
    assert(snapshot.last_update_id == 100);
    assert(snapshot.bids.size() == 2);
    assert(snapshot.asks.size() == 2);
    assert(snapshot.bids[0].price == 100.0);
    assert(snapshot.bids[0].quantity == 2.5);
    assert(snapshot.asks[0].price == 101.0);
    assert(snapshot.asks[0].quantity == 3.2);

    crypto::BookSnapshot invalid;
    assert(!crypto::parse_snapshot_json("BTCUSDT", R"({"lastUpdateId": 1, "bids": [["bad","2"]], "asks": []})", invalid));
    return 0;
}
