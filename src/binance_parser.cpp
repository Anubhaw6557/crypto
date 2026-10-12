#include "crypto/binance_parser.hpp"

#include <chrono>
#include <cstddef>
#include <string>

namespace crypto {
namespace {

void skip_spaces(std::string_view text, std::size_t& position) {
    while (position < text.size() && (text[position] == ' ' || text[position] == '\n' || text[position] == '\r' || text[position] == '\t')) {
        ++position;
    }
}

bool find_key(std::string_view text, std::string_view key, std::size_t& position) {
    const auto match = text.find('"' + std::string(key) + '"');
    if (match == std::string_view::npos) {
        return false;
    }
    position = match + key.size() + 2;
    skip_spaces(text, position);
    if (position >= text.size() || text[position] != ':') {
        return false;
    }
    ++position;
    skip_spaces(text, position);
    return position < text.size();
}

bool parse_string(std::string_view text, std::size_t& position, std::string& value) {
    if (position >= text.size() || text[position] != '"') {
        return false;
    }
    ++position;
    const auto start = position;
    while (position < text.size() && text[position] != '"') {
        if (text[position] == '\\') {
            return false;
        }
        ++position;
    }
    if (position >= text.size()) {
        return false;
    }
    value.assign(text.substr(start, position - start));
    ++position;
    return true;
}

bool parse_unsigned(std::string_view text, std::size_t& position, std::uint64_t& value) {
    const auto start = position;
    while (position < text.size() && text[position] >= '0' && text[position] <= '9') {
        ++position;
    }
    if (start == position) {
        return false;
    }
    try {
        value = std::stoull(std::string(text.substr(start, position - start)));
    } catch (...) {
        return false;
    }
    return true;
}

bool parse_price(std::string_view text, std::size_t& position, double& value) {
    std::string number;
    if (!parse_string(text, position, number)) {
        return false;
    }
    try {
        std::size_t parsed = 0;
        value = std::stod(number, &parsed);
        return parsed == number.size();
    } catch (...) {
        return false;
    }
}

bool parse_levels(std::string_view text, std::size_t& position, std::vector<PriceLevelUpdate>& levels) {
    if (position >= text.size() || text[position] != '[') {
        return false;
    }
    ++position;
    skip_spaces(text, position);
    while (position < text.size() && text[position] != ']') {
        if (text[position] != '[') {
            return false;
        }
        ++position;
        skip_spaces(text, position);
        PriceLevelUpdate level;
        if (!parse_price(text, position, level.price)) {
            return false;
        }
        skip_spaces(text, position);
        if (position >= text.size() || text[position] != ',') {
            return false;
        }
        ++position;
        skip_spaces(text, position);
        if (!parse_price(text, position, level.quantity)) {
            return false;
        }
        skip_spaces(text, position);
        if (position >= text.size() || text[position] != ']') {
            return false;
        }
        ++position;
        if (level.price <= 0.0 || level.quantity < 0.0) {
            return false;
        }
        levels.push_back(level);
        skip_spaces(text, position);
        if (position < text.size() && text[position] == ',') {
            ++position;
            skip_spaces(text, position);
        } else if (position >= text.size() || text[position] != ']') {
            return false;
        }
    }
    if (position >= text.size() || text[position] != ']') {
        return false;
    }
    ++position;
    return true;
}

bool parse_field_string(std::string_view text, std::string_view key, std::string& value) {
    std::size_t position = 0;
    return find_key(text, key, position) && parse_string(text, position, value);
}

bool parse_field_unsigned(std::string_view text, std::string_view key, std::uint64_t& value) {
    std::size_t position = 0;
    return find_key(text, key, position) && parse_unsigned(text, position, value);
}

bool parse_field_levels(std::string_view text, std::string_view key, std::vector<PriceLevelUpdate>& levels) {
    std::size_t position = 0;
    return find_key(text, key, position) && parse_levels(text, position, levels);
}

}

bool parse_binance_depth(const std::string_view payload, DepthUpdate& update, const MonotonicTime received_at) {
    std::string event;
    std::string symbol;
    std::uint64_t event_time_ms = 0;
    std::uint64_t first_update_id = 0;
    std::uint64_t final_update_id = 0;
    if (!parse_field_string(payload, "e", event) || event != "depthUpdate" ||
        !parse_field_string(payload, "s", symbol) || !parse_field_unsigned(payload, "E", event_time_ms) ||
        !parse_field_unsigned(payload, "U", first_update_id) || !parse_field_unsigned(payload, "u", final_update_id) ||
        first_update_id > final_update_id) {
        return false;
    }

    DepthUpdate parsed;
    parsed.symbol = std::move(symbol);
    parsed.first_update_id = first_update_id;
    parsed.final_update_id = final_update_id;
    parsed.event_time = EventTime{std::chrono::milliseconds(event_time_ms)};
    parsed.received_at = received_at;
    if (!parse_field_levels(payload, "b", parsed.bids) || !parse_field_levels(payload, "a", parsed.asks)) {
        return false;
    }
    update = std::move(parsed);
    return true;
}

}