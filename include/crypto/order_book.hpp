#pragma once

#include "crypto/contracts.hpp"

#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <string>

namespace crypto {

enum class BookUpdateResult {
    applied,
    waiting_for_snapshot,
    stale_update,
    sequence_gap,
    invalid_update,
};

class OrderBook {
public:
    explicit OrderBook(std::string symbol);

    bool apply_snapshot(const BookSnapshot& snapshot);
    BookUpdateResult apply_update(const DepthUpdate& update);

    [[nodiscard]] bool initialized() const;
    [[nodiscard]] bool valid() const;
    [[nodiscard]] std::uint64_t last_update_id() const;
    [[nodiscard]] std::optional<double> best_bid() const;
    [[nodiscard]] std::optional<double> best_ask() const;
    [[nodiscard]] std::optional<double> mid_price() const;
    [[nodiscard]] std::optional<double> spread() const;
    [[nodiscard]] bool is_fresh(MonotonicTime now, std::chrono::milliseconds max_age) const;

private:
    using BidLevels = std::map<double, double, std::greater<double>>;
    using AskLevels = std::map<double, double>;

    static bool valid_level(const PriceLevelUpdate& level);
    void clear();

    std::string symbol_;
    BidLevels bids_;
    AskLevels asks_;
    std::uint64_t last_update_id_{0};
    MonotonicTime last_update_at_{};
    bool initialized_{false};
    bool valid_{false};
};

}