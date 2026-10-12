#include "crypto/order_book.hpp"

#include <cassert>
#include <chrono>

namespace {

crypto::BookSnapshot snapshot() {
    crypto::BookSnapshot value;
    value.symbol = "BTCUSDT";
    value.last_update_id = 100;
    value.received_at = std::chrono::steady_clock::now();
    value.bids = {{100.0, 2.0}, {99.0, 1.0}};
    value.asks = {{101.0, 3.0}, {102.0, 1.0}};
    return value;
}

crypto::DepthUpdate update(std::uint64_t first, std::uint64_t last) {
    crypto::DepthUpdate value;
    value.symbol = "BTCUSDT";
    value.first_update_id = first;
    value.final_update_id = last;
    value.received_at = std::chrono::steady_clock::now();
    return value;
}

}

int main() {
    crypto::OrderBook book{"BTCUSDT"};
    auto before_snapshot = update(101, 101);
    before_snapshot.bids = {{100.0, 1.0}};
    assert(book.apply_update(before_snapshot) == crypto::BookUpdateResult::waiting_for_snapshot);

    assert(book.apply_snapshot(snapshot()));
    assert(book.initialized());
    assert(book.valid());
    assert(book.best_bid() == 100.0);
    assert(book.best_ask() == 101.0);
    assert(book.mid_price() == 100.5);
    assert(book.spread() == 1.0);

    auto applied = update(101, 102);
    applied.bids = {{100.0, 4.0}};
    applied.asks = {{101.0, 0.0}, {100.5, 1.0}};
    assert(book.apply_update(applied) == crypto::BookUpdateResult::applied);
    assert(book.best_bid() == 100.0);
    assert(book.best_ask() == 100.5);
    assert(book.last_update_id() == 102);

    auto stale = update(102, 102);
    stale.bids = {{99.0, 3.0}};
    assert(book.apply_update(stale) == crypto::BookUpdateResult::stale_update);

    auto gap = update(104, 104);
    gap.bids = {{99.0, 3.0}};
    assert(book.apply_update(gap) == crypto::BookUpdateResult::sequence_gap);
    assert(!book.valid());

    crypto::OrderBook wrong_symbol{"ETHUSDT"};
    assert(!wrong_symbol.apply_snapshot(snapshot()));
    return 0;
}