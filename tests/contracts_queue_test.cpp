#include "crypto/contracts.hpp"
#include "crypto/spsc_queue.hpp"

#include <cassert>
#include <chrono>
#include <string>
#include <thread>

int main() {
    crypto::SpscQueue<std::string, 2> queue;
    assert(queue.stats().size == 0);
    assert(queue.try_push("first"));
    assert(queue.try_push("second"));
    assert(!queue.try_push("overflow"));

    std::string value;
    assert(queue.try_pop(value));
    assert(value == "first");
    assert(queue.try_pop(value));
    assert(value == "second");
    assert(!queue.try_pop(value));

    queue.close();
    assert(queue.closed());
    assert(!queue.try_push("after close"));
    const auto stats = queue.stats();
    assert(stats.pushed == 2);
    assert(stats.popped == 2);
    assert(stats.overflows == 1);
    assert(stats.high_water_mark == 2);

    crypto::DepthUpdate update;
    update.symbol = "BTCUSDT";
    update.first_update_id = 10;
    update.final_update_id = 11;
    update.event_time = std::chrono::system_clock::now();
    update.received_at = std::chrono::steady_clock::now();
    update.bids.push_back({100.0, 1.5});
    assert(update.final_update_id >= update.first_update_id);

    crypto::Signal signal;
    signal.action = crypto::SignalAction::enter_long_spread;
    signal.market_data_valid = true;
    assert(signal.schema_version == 1);

    crypto::SpscQueue<int, 64> concurrent_queue;
    constexpr int item_count = 10000;
    std::thread producer([&concurrent_queue]() {
        for (int item = 0; item < item_count;) {
            if (concurrent_queue.try_push(item)) {
                ++item;
            } else {
                std::this_thread::yield();
            }
        }
        concurrent_queue.close();
    });

    int expected = 0;
    while (!concurrent_queue.closed() || concurrent_queue.stats().size != 0) {
        int received = 0;
        if (concurrent_queue.try_pop(received)) {
            assert(received == expected);
            ++expected;
        } else {
            std::this_thread::yield();
        }
    }
    producer.join();
    assert(expected == item_count);
    return 0;
}