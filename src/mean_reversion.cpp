#include "crypto/mean_reversion.hpp"

#include <cmath>
#include <numeric>
#include <utility>
#include <vector>

namespace crypto {

MeanReversionStrategy::MeanReversionStrategy(std::string symbol_pair, const double beta,
                                             const std::size_t window_size,
                                             const double entry_threshold,
                                             const double exit_threshold,
                                             std::string configuration_version)
    : symbol_pair_(std::move(symbol_pair)),
      beta_(beta),
      window_size_(window_size),
      entry_threshold_(entry_threshold),
      exit_threshold_(exit_threshold),
      configuration_version_(std::move(configuration_version)) {}

std::optional<Signal> MeanReversionStrategy::observe(const std::optional<double> btc_price,
                                                     const std::optional<double> eth_price,
                                                     const EventTime event_time,
                                                     const MonotonicTime created_at) {
    if (!btc_price || !eth_price || *btc_price <= 0.0 || *eth_price <= 0.0 ||
        !std::isfinite(*btc_price) || !std::isfinite(*eth_price)) {
        return make_signal(SignalAction::health, 0.0, btc_price.value_or(0.0), eth_price.value_or(0.0),
                           event_time, created_at, false);
    }

    const double spread = std::log(*btc_price) - beta_ * std::log(*eth_price);
    spreads_.push_back(spread);
    if (spreads_.size() > window_size_) {
        spreads_.pop_front();
    }
    if (spreads_.size() < window_size_) {
        return std::nullopt;
    }

    const double mean = std::accumulate(spreads_.begin(), spreads_.end(), 0.0) /
                        static_cast<double>(spreads_.size());
    double squared_deviation = 0.0;
    for (const double value : spreads_) {
        const double difference = value - mean;
        squared_deviation += difference * difference;
    }
    const double standard_deviation = std::sqrt(squared_deviation / static_cast<double>(spreads_.size()));
    if (standard_deviation == 0.0) {
        return std::nullopt;
    }

    const double z_score = (spread - mean) / standard_deviation;
    if (state_ == State::neutral && z_score < -entry_threshold_) {
        state_ = State::long_spread;
        return make_signal(SignalAction::enter_long_spread, z_score, *btc_price, *eth_price,
                           event_time, created_at, true);
    }
    if (state_ == State::neutral && z_score > entry_threshold_) {
        state_ = State::short_spread;
        return make_signal(SignalAction::enter_short_spread, z_score, *btc_price, *eth_price,
                           event_time, created_at, true);
    }
    if (state_ != State::neutral && std::abs(z_score) < exit_threshold_) {
        state_ = State::neutral;
        return make_signal(SignalAction::exit_spread, z_score, *btc_price, *eth_price,
                           event_time, created_at, true);
    }
    return std::nullopt;
}

Signal MeanReversionStrategy::make_signal(const SignalAction action, const double z_score,
                                          const double btc_price, const double eth_price,
                                          const EventTime event_time, const MonotonicTime created_at,
                                          const bool market_data_valid) {
    Signal signal;
    signal.sequence_id = next_sequence_id_++;
    signal.symbol_pair = symbol_pair_;
    signal.action = action;
    signal.z_score = z_score;
    signal.btc_reference_price = btc_price;
    signal.eth_reference_price = eth_price;
    signal.event_time = event_time;
    signal.created_at = created_at;
    signal.market_data_valid = market_data_valid;
    signal.configuration_version = configuration_version_;
    return signal;
}

}
