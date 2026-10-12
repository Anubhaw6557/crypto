#pragma once

#include "crypto/contracts.hpp"

#include <cstddef>
#include <deque>
#include <optional>
#include <string>

namespace crypto {

class MeanReversionStrategy {
public:
    MeanReversionStrategy(std::string symbol_pair, double beta, std::size_t window_size,
                          double entry_threshold, double exit_threshold,
                          std::string configuration_version = "v1");

    std::optional<Signal> observe(std::optional<double> btc_price,
                                  std::optional<double> eth_price,
                                  EventTime event_time,
                                  MonotonicTime created_at);

private:
    enum class State { neutral, long_spread, short_spread };

    Signal make_signal(SignalAction action, double z_score, double btc_price,
                       double eth_price, EventTime event_time, MonotonicTime created_at,
                       bool market_data_valid);

    std::string symbol_pair_;
    double beta_;
    std::size_t window_size_;
    double entry_threshold_;
    double exit_threshold_;
    std::string configuration_version_;
    std::deque<double> spreads_;
    State state_{State::neutral};
    std::uint64_t next_sequence_id_{1};
};

}
