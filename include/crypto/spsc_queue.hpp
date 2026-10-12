#pragma once

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <utility>

namespace crypto {

struct QueueStats {
    std::size_t size{0};
    std::size_t high_water_mark{0};
    std::uint64_t pushed{0};
    std::uint64_t popped{0};
    std::uint64_t overflows{0};
    bool closed{false};
};

template <typename T, std::size_t Capacity>
class SpscQueue {
    static_assert(Capacity > 0, "SPSC queue capacity must be positive");

public:
    SpscQueue() = default;
    SpscQueue(const SpscQueue&) = delete;
    SpscQueue& operator=(const SpscQueue&) = delete;

    ~SpscQueue() {
        close();
    }

    bool try_push(T value) {
        if (closed_.load(std::memory_order_acquire)) {
            return false;
        }

        const auto head = head_.load(std::memory_order_relaxed);
        const auto tail = tail_.load(std::memory_order_acquire);
        if (head - tail >= Capacity) {
            overflows_.fetch_add(1, std::memory_order_relaxed);
            return false;
        }

        slots_[head % Capacity].emplace(std::move(value));
        head_.store(head + 1, std::memory_order_release);
        pushed_.fetch_add(1, std::memory_order_relaxed);
        const auto size = head + 1 - tail;
        const auto previous_high_water_mark = high_water_mark_.load(std::memory_order_relaxed);
        if (size > previous_high_water_mark) {
            high_water_mark_.store(size, std::memory_order_relaxed);
        }
        return true;
    }

    bool try_pop(T& value) {
        const auto tail = tail_.load(std::memory_order_relaxed);
        const auto head = head_.load(std::memory_order_acquire);
        if (tail == head) {
            return false;
        }

        auto& slot = slots_[tail % Capacity];
        value = std::move(*slot);
        slot.reset();
        tail_.store(tail + 1, std::memory_order_release);
        popped_.fetch_add(1, std::memory_order_relaxed);
        return true;
    }

    void close() {
        closed_.store(true, std::memory_order_release);
    }

    [[nodiscard]] bool closed() const {
        return closed_.load(std::memory_order_acquire);
    }

    [[nodiscard]] QueueStats stats() const {
        const auto tail = tail_.load(std::memory_order_acquire);
        const auto head = head_.load(std::memory_order_acquire);
        return QueueStats{
            head - tail,
            high_water_mark_.load(std::memory_order_relaxed),
            pushed_.load(std::memory_order_relaxed),
            popped_.load(std::memory_order_relaxed),
            overflows_.load(std::memory_order_relaxed),
            closed(),
        };
    }

private:
    std::array<std::optional<T>, Capacity> slots_{};
    alignas(64) std::atomic<std::size_t> head_{0};
    alignas(64) std::atomic<std::size_t> tail_{0};
    std::atomic_bool closed_{false};
    std::atomic<std::size_t> high_water_mark_{0};
    std::atomic<std::uint64_t> pushed_{0};
    std::atomic<std::uint64_t> popped_{0};
    std::atomic<std::uint64_t> overflows_{0};
};

}