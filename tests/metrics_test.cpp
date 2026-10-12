#include "crypto/metrics.hpp"

#include <cassert>
#include <chrono>
#include <string>

int main() {
    crypto::Metrics metrics{{"BTCUSDT"}};
    metrics.feed_message("BTCUSDT");
    metrics.book_update("BTCUSDT");
    metrics.sequence_gap("BTCUSDT");
    metrics.recovery("BTCUSDT");
    metrics.strategy_sample();
    metrics.signal("ENTER_LONG_SPREAD");
    metrics.set_queue_stats(3, 8, 1);
    metrics.observe_queue_wait(std::chrono::microseconds(250));
    metrics.observe_book_update(std::chrono::microseconds(900));
    metrics.observe_strategy(std::chrono::milliseconds(2));

    const auto rendered = metrics.render_prometheus();
    assert(rendered.find("crypto_queue_depth 3") != std::string::npos);
    assert(rendered.find("crypto_queue_high_water_mark 8") != std::string::npos);
    assert(rendered.find("crypto_feed_messages_total{symbol=\"BTCUSDT\"} 1") != std::string::npos);
    assert(rendered.find("crypto_sequence_gaps_total{symbol=\"BTCUSDT\"} 1") != std::string::npos);
    assert(rendered.find("crypto_signals_total{action=\"ENTER_LONG_SPREAD\"} 1") != std::string::npos);
    assert(rendered.find("crypto_book_update_seconds_bucket") != std::string::npos);
    return 0;
}
