#pragma once

#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <string>
#include <thread>
#include <vector>

namespace crypto {

class Metrics {
public:
    explicit Metrics(const std::vector<std::string>& symbols);

    void feed_message(const std::string& symbol);
    void parse_failure(const std::string& symbol);
    void queue_overflow(const std::string& symbol);
    void book_update(const std::string& symbol);
    void sequence_gap(const std::string& symbol);
    void recovery(const std::string& symbol);
    void strategy_sample();
    void signal(const std::string& action);
    void set_queue_stats(std::size_t depth, std::size_t high_water_mark, std::uint64_t overflows);
    void observe_queue_wait(std::chrono::nanoseconds duration);
    void observe_book_update(std::chrono::nanoseconds duration);
    void observe_strategy(std::chrono::nanoseconds duration);

    [[nodiscard]] std::string render_prometheus() const;

private:
    struct SymbolCounters {
        std::atomic<std::uint64_t> feed_messages{0};
        std::atomic<std::uint64_t> parse_failures{0};
        std::atomic<std::uint64_t> queue_overflows{0};
        std::atomic<std::uint64_t> book_updates{0};
        std::atomic<std::uint64_t> sequence_gaps{0};
        std::atomic<std::uint64_t> recoveries{0};
    };

    struct Histogram {
        static constexpr std::array<std::uint64_t, 7> bucket_microseconds{100, 500, 1000, 5000, 10000, 50000, 0};
        std::array<std::atomic<std::uint64_t>, 7> buckets{};
        std::atomic<std::uint64_t> count{0};
        std::atomic<std::uint64_t> sum_microseconds{0};

        void observe(std::chrono::nanoseconds duration);
        void render(std::string& output, const std::string& name) const;
    };

    SymbolCounters* find_symbol(const std::string& symbol);
    const SymbolCounters* find_symbol(const std::string& symbol) const;

    std::vector<std::string> symbols_;
    std::vector<SymbolCounters> symbol_counters_;
    std::atomic<std::uint64_t> strategy_samples_{0};
    std::atomic<std::uint64_t> signals_{0};
    std::atomic<std::uint64_t> enter_long_signals_{0};
    std::atomic<std::uint64_t> enter_short_signals_{0};
    std::atomic<std::uint64_t> exit_signals_{0};
    std::atomic<std::size_t> queue_depth_{0};
    std::atomic<std::size_t> queue_high_water_mark_{0};
    std::atomic<std::uint64_t> queue_overflows_{0};
    Histogram queue_wait_microseconds_;
    Histogram book_update_microseconds_;
    Histogram strategy_microseconds_;
};

class MetricsHttpServer {
public:
    MetricsHttpServer(const Metrics& metrics, std::uint16_t port);
    void run(const std::atomic_bool& stop_requested) const;

private:
    const Metrics& metrics_;
    std::uint16_t port_;
};

}
