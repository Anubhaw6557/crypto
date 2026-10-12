#include "crypto/config.hpp"

#include <cmath>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <string_view>

namespace crypto {
namespace {

std::string trim(std::string value) {
    const auto first = value.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) {
        return {};
    }
    const auto last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1);
}

std::string unquote(std::string value) {
    value = trim(std::move(value));
    if (value.size() >= 2 && value.front() == '"' && value.back() == '"') {
        return value.substr(1, value.size() - 2);
    }
    return value;
}

std::vector<std::string> parse_symbols(std::string value) {
    value = trim(std::move(value));
    if (value.size() < 2 || value.front() != '[' || value.back() != ']') {
        throw std::runtime_error("symbols must be a TOML array");
    }

    value = value.substr(1, value.size() - 2);
    std::vector<std::string> symbols;
    std::size_t start = 0;
    while (start < value.size()) {
        const auto comma = value.find(',', start);
        const auto item = value.substr(start, comma == std::string::npos ? std::string::npos : comma - start);
        if (!trim(item).empty()) {
            symbols.push_back(unquote(item));
        }
        if (comma == std::string::npos) {
            break;
        }
        start = comma + 1;
    }
    return symbols;
}

}

Config Config::defaults() {
    return {};
}

Config Config::load(const std::filesystem::path& path) {
    Config config = defaults();
    std::ifstream input(path);
    if (!input) {
        throw std::runtime_error("unable to open configuration: " + path.string());
    }

    std::string line;
    std::size_t line_number = 0;
    while (std::getline(input, line)) {
        ++line_number;
        const auto comment = line.find('#');
        if (comment != std::string::npos) {
            line.erase(comment);
        }
        line = trim(std::move(line));
        if (line.empty() || line.front() == '[') {
            continue;
        }

        const auto equals = line.find('=');
        if (equals == std::string::npos) {
            throw std::runtime_error("invalid configuration at line " + std::to_string(line_number));
        }
        const auto key = trim(line.substr(0, equals));
        const auto value = trim(line.substr(equals + 1));
        try {
            if (key == "symbols") {
                config.symbols = parse_symbols(value);
            } else if (key == "strategy_pair") {
                config.strategy_pair = unquote(value);
            } else if (key == "sampling_interval_ms") {
                config.sampling_interval_ms = static_cast<std::uint32_t>(std::stoul(value));
            } else if (key == "beta") {
                config.beta = std::stod(value);
            } else if (key == "rolling_window") {
                config.rolling_window = static_cast<std::size_t>(std::stoull(value));
            } else if (key == "entry_threshold") {
                config.entry_threshold = std::stod(value);
            } else if (key == "exit_threshold") {
                config.exit_threshold = std::stod(value);
            } else if (key == "signal_port") {
                config.signal_port = static_cast<std::uint16_t>(std::stoul(value));
            } else if (key == "metrics_port") {
                config.metrics_port = static_cast<std::uint16_t>(std::stoul(value));
            } else if (key == "log_level") {
                config.log_level = unquote(value);
            }
        } catch (const std::exception& error) {
            throw std::runtime_error("invalid value for " + key + " at line " + std::to_string(line_number) + ": " + error.what());
        }
    }

    config.validate();
    return config;
}

void Config::validate() const {
    if (symbols.empty()) {
        throw std::invalid_argument("symbols must not be empty");
    }
    if (sampling_interval_ms == 0 || rolling_window == 0) {
        throw std::invalid_argument("sampling interval and rolling window must be positive");
    }
    if (!std::isfinite(beta) || beta <= 0.0) {
        throw std::invalid_argument("beta must be finite and positive");
    }
    if (!std::isfinite(entry_threshold) || !std::isfinite(exit_threshold) || entry_threshold <= exit_threshold || exit_threshold < 0.0) {
        throw std::invalid_argument("strategy thresholds are invalid");
    }
    if (signal_port == 0) {
        throw std::invalid_argument("signal port must be non-zero");
    }
    if (metrics_port == 0) {
        throw std::invalid_argument("metrics port must be non-zero");
    }
}

}