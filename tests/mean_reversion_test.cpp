#include "crypto/mean_reversion.hpp"

#include <cassert>
#include <cmath>
#include <chrono>

int main() {
    crypto::MeanReversionStrategy strategy{"BTCUSDT/ETHUSDT", 1.0, 6, 2.0, 0.5};
    const auto event_time = std::chrono::system_clock::now();
    const auto created_at = std::chrono::steady_clock::now();

    for (int sample = 0; sample < 5; ++sample) {
        assert(!strategy.observe(1.0, 1.0, event_time, created_at));
    }
    const auto entry = strategy.observe(std::exp(-100.0), 1.0, event_time, created_at);
    assert(entry.has_value());
    assert(entry->action == crypto::SignalAction::enter_long_spread);
    assert(entry->market_data_valid);

    const auto exit = strategy.observe(1.0, 1.0, event_time, created_at);
    assert(exit.has_value());
    assert(exit->action == crypto::SignalAction::exit_spread);

    const auto repeated = strategy.observe(1.0, 1.0, event_time, created_at);
    assert(!repeated.has_value() || repeated->action != crypto::SignalAction::enter_long_spread);

    const auto health = strategy.observe(std::nullopt, 1.0, event_time, created_at);
    assert(health.has_value());
    assert(health->action == crypto::SignalAction::health);
    assert(!health->market_data_valid);
    return 0;
}
