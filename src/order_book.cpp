#include "crypto/order_book.hpp"

#include <cmath>

namespace crypto {

OrderBook::OrderBook(std::string symbol) : symbol_(std::move(symbol)) {}

bool OrderBook::apply_snapshot(const BookSnapshot& snapshot) {
    if (snapshot.symbol != symbol_ || snapshot.last_update_id == 0 || snapshot.bids.empty() || snapshot.asks.empty()) {
        clear();
        return false;
    }
    for (const auto& level : snapshot.bids) {
        if (!valid_level(level)) {
            clear();
            return false;
        }
    }
    for (const auto& level : snapshot.asks) {
        if (!valid_level(level)) {
            clear();
            return false;
        }
    }

    BidLevels bids;
    AskLevels asks;
    for (const auto& level : snapshot.bids) {
        if (level.quantity > 0.0) {
            bids[level.price] = level.quantity;
        }
    }
    for (const auto& level : snapshot.asks) {
        if (level.quantity > 0.0) {
            asks[level.price] = level.quantity;
        }
    }
    if (bids.empty() || asks.empty() || bids.begin()->first >= asks.begin()->first) {
        clear();
        return false;
    }

    bids_ = std::move(bids);
    asks_ = std::move(asks);
    last_update_id_ = snapshot.last_update_id;
    last_update_at_ = snapshot.received_at;
    initialized_ = true;
    valid_ = true;
    return true;
}

BookUpdateResult OrderBook::apply_update(const DepthUpdate& update) {
    if (update.symbol != symbol_ || update.first_update_id > update.final_update_id ||
        (update.bids.empty() && update.asks.empty())) {
        valid_ = false;
        return BookUpdateResult::invalid_update;
    }
    for (const auto& level : update.bids) {
        if (!valid_level(level)) {
            valid_ = false;
            return BookUpdateResult::invalid_update;
        }
    }
    for (const auto& level : update.asks) {
        if (!valid_level(level)) {
            valid_ = false;
            return BookUpdateResult::invalid_update;
        }
    }
    if (!initialized_) {
        return BookUpdateResult::waiting_for_snapshot;
    }
    if (update.final_update_id <= last_update_id_) {
        return BookUpdateResult::stale_update;
    }
    if (update.first_update_id > last_update_id_ + 1) {
        valid_ = false;
        return BookUpdateResult::sequence_gap;
    }

    for (const auto& level : update.bids) {
        if (level.quantity == 0.0) {
            bids_.erase(level.price);
        } else {
            bids_[level.price] = level.quantity;
        }
    }
    for (const auto& level : update.asks) {
        if (level.quantity == 0.0) {
            asks_.erase(level.price);
        } else {
            asks_[level.price] = level.quantity;
        }
    }

    if (bids_.empty() || asks_.empty() || bids_.begin()->first >= asks_.begin()->first) {
        valid_ = false;
        return BookUpdateResult::invalid_update;
    }
    last_update_id_ = update.final_update_id;
    last_update_at_ = update.received_at;
    valid_ = true;
    return BookUpdateResult::applied;
}

bool OrderBook::initialized() const {
    return initialized_;
}

bool OrderBook::valid() const {
    return valid_;
}

std::uint64_t OrderBook::last_update_id() const {
    return last_update_id_;
}

std::optional<double> OrderBook::best_bid() const {
    if (!valid_ || bids_.empty()) {
        return std::nullopt;
    }
    return bids_.begin()->first;
}

std::optional<double> OrderBook::best_ask() const {
    if (!valid_ || asks_.empty()) {
        return std::nullopt;
    }
    return asks_.begin()->first;
}

std::optional<double> OrderBook::mid_price() const {
    const auto bid = best_bid();
    const auto ask = best_ask();
    if (!bid || !ask) {
        return std::nullopt;
    }
    return (*bid + *ask) / 2.0;
}

std::optional<double> OrderBook::spread() const {
    const auto bid = best_bid();
    const auto ask = best_ask();
    if (!bid || !ask) {
        return std::nullopt;
    }
    return *ask - *bid;
}

bool OrderBook::is_fresh(const MonotonicTime now, const std::chrono::milliseconds max_age) const {
    return valid_ && now >= last_update_at_ && now - last_update_at_ <= max_age;
}

bool OrderBook::valid_level(const PriceLevelUpdate& level) {
    return std::isfinite(level.price) && std::isfinite(level.quantity) && level.price > 0.0 && level.quantity >= 0.0;
}

void OrderBook::clear() {
    bids_.clear();
    asks_.clear();
    last_update_id_ = 0;
    last_update_at_ = {};
    initialized_ = false;
    valid_ = false;
}

}