#include "crypto/metrics.hpp"

#include <boost/asio.hpp>

#include <algorithm>
#include <sstream>
#include <string>

namespace crypto {

void Metrics::Histogram::observe(const std::chrono::nanoseconds duration) {
    const auto microseconds = static_cast<std::uint64_t>(std::max<std::int64_t>(0, duration.count() / 1000));
    sum_microseconds.fetch_add(microseconds, std::memory_order_relaxed);
    count.fetch_add(1, std::memory_order_relaxed);
    for (std::size_t index = 0; index + 1 < bucket_microseconds.size(); ++index) {
        if (microseconds <= bucket_microseconds[index]) {
            buckets[index].fetch_add(1, std::memory_order_relaxed);
            return;
        }
    }
    buckets.back().fetch_add(1, std::memory_order_relaxed);
}

void Metrics::Histogram::render(std::string& output, const std::string& name) const {
    std::uint64_t cumulative = 0;
    for (std::size_t index = 0; index + 1 < bucket_microseconds.size(); ++index) {
        cumulative += buckets[index].load(std::memory_order_relaxed);
        output += name + "_bucket{le=\"" + std::to_string(static_cast<double>(bucket_microseconds[index]) / 1000000.0) + "\"} " + std::to_string(cumulative) + "\n";
    }
    cumulative += buckets.back().load(std::memory_order_relaxed);
    output += name + "_bucket{le=\"+Inf\"} " + std::to_string(cumulative) + "\n";
    output += name + "_count " + std::to_string(count.load(std::memory_order_relaxed)) + "\n";
    output += name + "_sum " + std::to_string(static_cast<double>(sum_microseconds.load(std::memory_order_relaxed)) / 1000000.0) + "\n";
}

Metrics::Metrics(const std::vector<std::string>& symbols)
    : symbols_(symbols), symbol_counters_(symbols.size()) {}

Metrics::SymbolCounters* Metrics::find_symbol(const std::string& symbol) {
    const auto iterator = std::find(symbols_.begin(), symbols_.end(), symbol);
    return iterator == symbols_.end() ? nullptr : &symbol_counters_[static_cast<std::size_t>(iterator - symbols_.begin())];
}

const Metrics::SymbolCounters* Metrics::find_symbol(const std::string& symbol) const {
    const auto iterator = std::find(symbols_.begin(), symbols_.end(), symbol);
    return iterator == symbols_.end() ? nullptr : &symbol_counters_[static_cast<std::size_t>(iterator - symbols_.begin())];
}

void Metrics::feed_message(const std::string& symbol) {
    if (auto* counters = find_symbol(symbol)) counters->feed_messages.fetch_add(1, std::memory_order_relaxed);
}

void Metrics::parse_failure(const std::string& symbol) {
    if (auto* counters = find_symbol(symbol)) counters->parse_failures.fetch_add(1, std::memory_order_relaxed);
}

void Metrics::queue_overflow(const std::string& symbol) {
    if (auto* counters = find_symbol(symbol)) counters->queue_overflows.fetch_add(1, std::memory_order_relaxed);
}

void Metrics::book_update(const std::string& symbol) {
    if (auto* counters = find_symbol(symbol)) counters->book_updates.fetch_add(1, std::memory_order_relaxed);
}

void Metrics::sequence_gap(const std::string& symbol) {
    if (auto* counters = find_symbol(symbol)) counters->sequence_gaps.fetch_add(1, std::memory_order_relaxed);
}

void Metrics::recovery(const std::string& symbol) {
    if (auto* counters = find_symbol(symbol)) counters->recoveries.fetch_add(1, std::memory_order_relaxed);
}

void Metrics::strategy_sample() {
    strategy_samples_.fetch_add(1, std::memory_order_relaxed);
}

void Metrics::signal(const std::string& action) {
    signals_.fetch_add(1, std::memory_order_relaxed);
    if (action == "ENTER_LONG_SPREAD") enter_long_signals_.fetch_add(1, std::memory_order_relaxed);
    if (action == "ENTER_SHORT_SPREAD") enter_short_signals_.fetch_add(1, std::memory_order_relaxed);
    if (action == "EXIT_SPREAD") exit_signals_.fetch_add(1, std::memory_order_relaxed);
}

void Metrics::set_queue_stats(const std::size_t depth, const std::size_t high_water_mark, const std::uint64_t overflows) {
    queue_depth_.store(depth, std::memory_order_relaxed);
    queue_high_water_mark_.store(high_water_mark, std::memory_order_relaxed);
    queue_overflows_.store(overflows, std::memory_order_relaxed);
}

void Metrics::observe_queue_wait(const std::chrono::nanoseconds duration) { queue_wait_microseconds_.observe(duration); }
void Metrics::observe_book_update(const std::chrono::nanoseconds duration) { book_update_microseconds_.observe(duration); }
void Metrics::observe_strategy(const std::chrono::nanoseconds duration) { strategy_microseconds_.observe(duration); }

std::string Metrics::render_prometheus() const {
    std::string output;
    output += "# TYPE crypto_queue_depth gauge\ncrypto_queue_depth " + std::to_string(queue_depth_.load()) + "\n";
    output += "# TYPE crypto_queue_high_water_mark gauge\ncrypto_queue_high_water_mark " + std::to_string(queue_high_water_mark_.load()) + "\n";
    output += "# TYPE crypto_queue_overflows counter\ncrypto_queue_overflows " + std::to_string(queue_overflows_.load()) + "\n";
    output += "# TYPE crypto_strategy_samples counter\ncrypto_strategy_samples " + std::to_string(strategy_samples_.load()) + "\n";
    output += "# TYPE crypto_signals counter\ncrypto_signals " + std::to_string(signals_.load()) + "\n";
    output += "crypto_signals_total{action=\"ENTER_LONG_SPREAD\"} " + std::to_string(enter_long_signals_.load()) + "\n";
    output += "crypto_signals_total{action=\"ENTER_SHORT_SPREAD\"} " + std::to_string(enter_short_signals_.load()) + "\n";
    output += "crypto_signals_total{action=\"EXIT_SPREAD\"} " + std::to_string(exit_signals_.load()) + "\n";
    for (std::size_t index = 0; index < symbols_.size(); ++index) {
        const auto& symbol = symbols_[index];
        const auto& counters = symbol_counters_[index];
        const auto label = "{symbol=\"" + symbol + "\"}";
        output += "crypto_feed_messages_total" + label + " " + std::to_string(counters.feed_messages.load()) + "\n";
        output += "crypto_parse_failures_total" + label + " " + std::to_string(counters.parse_failures.load()) + "\n";
        output += "crypto_queue_overflows_total" + label + " " + std::to_string(counters.queue_overflows.load()) + "\n";
        output += "crypto_order_book_updates_total" + label + " " + std::to_string(counters.book_updates.load()) + "\n";
        output += "crypto_sequence_gaps_total" + label + " " + std::to_string(counters.sequence_gaps.load()) + "\n";
        output += "crypto_recoveries_total" + label + " " + std::to_string(counters.recoveries.load()) + "\n";
    }
    queue_wait_microseconds_.render(output, "crypto_queue_wait_seconds");
    book_update_microseconds_.render(output, "crypto_book_update_seconds");
    strategy_microseconds_.render(output, "crypto_strategy_seconds");
    return output;
}

MetricsHttpServer::MetricsHttpServer(const Metrics& metrics, const std::uint16_t port)
    : metrics_(metrics), port_(port) {}

void MetricsHttpServer::run(const std::atomic_bool& stop_requested) const {
    namespace asio = boost::asio;
    using tcp = asio::ip::tcp;
    asio::io_context context;
    tcp::acceptor acceptor(context, tcp::endpoint(tcp::v4(), port_));
    acceptor.non_blocking(true);
    while (!stop_requested.load(std::memory_order_acquire)) {
        tcp::socket socket(context);
        boost::system::error_code error;
        acceptor.accept(socket, error);
        if (error == asio::error::would_block || error == asio::error::try_again) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            continue;
        }
        if (error) {
            continue;
        }
        const auto body = metrics_.render_prometheus();
        const auto response = "HTTP/1.1 200 OK\r\nContent-Type: text/plain; version=0.0.4\r\nContent-Length: " +
            std::to_string(body.size()) + "\r\nConnection: close\r\n\r\n" + body;
        asio::write(socket, asio::buffer(response), error);
    }
}

}
