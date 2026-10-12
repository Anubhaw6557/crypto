#pragma once

#include <chrono>
#include <cstdint>
#include <string>
#include <vector>

namespace crypto {

using EventTime = std::chrono::system_clock::time_point;
using MonotonicTime = std::chrono::steady_clock::time_point;

struct PriceLevelUpdate {
    double price{0.0};
    double quantity{0.0};
};

struct DepthUpdate {
    std::string symbol;
    std::uint64_t first_update_id{0};
    std::uint64_t final_update_id{0};
    EventTime event_time{};
    MonotonicTime received_at{};
    std::vector<PriceLevelUpdate> bids;
    std::vector<PriceLevelUpdate> asks;
};

struct BookSnapshot {
    std::string symbol;
    std::uint64_t last_update_id{0};
    EventTime event_time{};
    MonotonicTime received_at{};
    std::vector<PriceLevelUpdate> bids;
    std::vector<PriceLevelUpdate> asks;
};

enum class FeedHealth { disconnected, connecting, syncing, healthy, stale, recovering, failed };

struct FeedStatus {
    std::string symbol;
    FeedHealth health{FeedHealth::disconnected};
    MonotonicTime last_update_at{};
    std::uint64_t sequence_gaps{0};
    std::uint64_t reconnects{0};
    std::chrono::nanoseconds recovery_duration{0};
};

enum class SignalAction { enter_long_spread, enter_short_spread, exit_spread, health };

struct Signal {
    std::uint64_t sequence_id{0};
    std::uint32_t schema_version{1};
    std::string strategy{"btc_eth_mean_reversion"};
    std::string symbol_pair{"BTCUSDT/ETHUSDT"};
    SignalAction action{SignalAction::health};
    double z_score{0.0};
    double btc_reference_price{0.0};
    double eth_reference_price{0.0};
    EventTime event_time{};
    MonotonicTime created_at{};
    bool market_data_valid{false};
    std::string configuration_version;
};

}