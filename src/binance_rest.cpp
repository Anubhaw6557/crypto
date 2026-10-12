#include "crypto/binance_rest.hpp"

#include <curl/curl.h>

#include <cctype>
#include <cmath>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace crypto {
namespace {

void skip_whitespace(std::string_view text, std::size_t& position) {
    while (position < text.size() && std::isspace(static_cast<unsigned char>(text[position]))) {
        ++position;
    }
}

bool expect(std::string_view text, std::size_t& position, const char expected) {
    skip_whitespace(text, position);
    if (position >= text.size() || text[position] != expected) {
        return false;
    }
    ++position;
    return true;
}

bool parse_unsigned(std::string_view text, std::size_t& position, std::uint64_t& value) {
    skip_whitespace(text, position);
    const auto start = position;
    while (position < text.size() && std::isdigit(static_cast<unsigned char>(text[position]))) {
        ++position;
    }
    if (start == position) {
        return false;
    }
    value = static_cast<std::uint64_t>(std::stoull(std::string(text.substr(start, position - start))));
    return true;
}

bool parse_quoted_number(std::string_view text, std::size_t& position, double& value) {
    skip_whitespace(text, position);
    if (!expect(text, position, '"')) {
        return false;
    }
    const auto start = position;
    while (position < text.size() && text[position] != '"') {
        ++position;
    }
    if (position >= text.size() || start == position) {
        return false;
    }
    const auto number = std::string(text.substr(start, position - start));
    ++position;

    try {
        std::size_t parsed = 0;
        value = std::stod(number, &parsed);
        return parsed == number.size() && std::isfinite(value);
    } catch (const std::exception&) {
        return false;
    }
}

bool parse_level_array(std::string_view text, std::string_view key, std::vector<PriceLevelUpdate>& levels) {
    const std::string key_text = std::string("\"") + std::string(key) + std::string("\"");
    const auto key_position = text.find(key_text);
    if (key_position == std::string_view::npos) {
        return false;
    }
    auto position = text.find(':', key_position + key_text.size());
    if (position == std::string_view::npos) {
        return false;
    }
    ++position;
    if (!expect(text, position, '[')) {
        return false;
    }

    levels.clear();
    skip_whitespace(text, position);
    while (position < text.size() && text[position] != ']') {
        if (!expect(text, position, '[')) {
            return false;
        }
        PriceLevelUpdate level;
        if (!parse_quoted_number(text, position, level.price) ||
            !expect(text, position, ',') ||
            !parse_quoted_number(text, position, level.quantity) ||
            !expect(text, position, ']')) {
            return false;
        }
        if (level.price <= 0.0 || level.quantity < 0.0) {
            return false;
        }
        levels.push_back(level);
        skip_whitespace(text, position);
        if (position < text.size() && text[position] == ',') {
            ++position;
            skip_whitespace(text, position);
        } else if (position >= text.size() || text[position] != ']') {
            return false;
        }
    }

    if (!expect(text, position, ']')) {
        return false;
    }
    return !levels.empty();
}

std::size_t write_response(char* contents, std::size_t size, std::size_t nmemb, std::string* output) {
    const auto total = size * nmemb;
    output->append(contents, total);
    return total;
}

}

bool parse_snapshot_json(const std::string& symbol, const std::string& payload, BookSnapshot& snapshot) {
    if (symbol.empty() || payload.empty()) {
        return false;
    }

    BookSnapshot parsed;
    parsed.symbol = symbol;
    parsed.event_time = std::chrono::system_clock::now();
    parsed.received_at = std::chrono::steady_clock::now();

    const std::string last_update_key = "\"lastUpdateId\"";
    const auto last_update_pos = payload.find(last_update_key);
    if (last_update_pos == std::string::npos) {
        return false;
    }
    std::size_t position = payload.find(':', last_update_pos + last_update_key.size());
    if (position == std::string::npos) {
        return false;
    }
    ++position;
    std::uint64_t last_update_id = 0;
    if (!parse_unsigned(payload, position, last_update_id)) {
        return false;
    }
    parsed.last_update_id = last_update_id;

    std::vector<PriceLevelUpdate> bids;
    std::vector<PriceLevelUpdate> asks;
    if (!parse_level_array(payload, "bids", bids) || !parse_level_array(payload, "asks", asks)) {
        return false;
    }

    parsed.bids = std::move(bids);
    parsed.asks = std::move(asks);
    snapshot = std::move(parsed);
    return true;
}

bool fetch_snapshot(const std::string& symbol, BookSnapshot& snapshot, const std::chrono::milliseconds timeout_ms) {
    const auto url = std::string("https://api.binance.com/api/v3/depth?symbol=") + symbol + "&limit=10";
    CURL* curl = curl_easy_init();
    if (curl == nullptr) {
        return false;
    }

    std::string response;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_response);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS, 3000L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, timeout_ms.count());

    const auto result = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    if (result != CURLE_OK) {
        return false;
    }

    return parse_snapshot_json(symbol, response, snapshot);
}

}
