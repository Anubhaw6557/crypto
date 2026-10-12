#pragma once

#include "crypto/contracts.hpp"

#include <chrono>
#include <string>

namespace crypto {

bool parse_snapshot_json(const std::string& symbol, const std::string& payload, BookSnapshot& snapshot);
bool fetch_snapshot(const std::string& symbol, BookSnapshot& snapshot, std::chrono::milliseconds timeout_ms = std::chrono::milliseconds{5000});

}
