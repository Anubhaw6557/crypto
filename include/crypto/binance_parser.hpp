#pragma once

#include "crypto/contracts.hpp"

#include <string_view>

namespace crypto {

// Parses a Binance depthUpdate payload into the application's common format.
bool parse_binance_depth(std::string_view payload, DepthUpdate& update, MonotonicTime received_at);

}