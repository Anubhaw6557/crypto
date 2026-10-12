#pragma once

#include "crypto/config.hpp"
#include "crypto/contracts.hpp"
#include "crypto/spsc_queue.hpp"

#include <atomic>

namespace crypto {

class BinanceFeed {
public:
    void run(const Config& config, SpscQueue<DepthUpdate, 4096>& updates, const std::atomic_bool& stop_requested) const;
};

}