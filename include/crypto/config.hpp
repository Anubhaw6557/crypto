#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace crypto {

struct Config {
    std::vector<std::string> symbols{"BTCUSDT", "ETHUSDT", "SOLUSDT", "BNBUSDT", "XRPUSDT"};
    std::string strategy_pair{"BTCUSDT/ETHUSDT"};
    std::uint32_t sampling_interval_ms{1000};
    double beta{1.0};
    std::size_t rolling_window{1000};
    double entry_threshold{2.0};
    double exit_threshold{0.5};
    std::uint16_t signal_port{9000};
    std::string log_level{"info"};

    static Config defaults();
    static Config load(const std::filesystem::path& path);
    void validate() const;
};

}