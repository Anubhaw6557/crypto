#include "crypto/logger.hpp"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>

namespace crypto {

void log(const LogLevel level, const std::string_view component, const std::string_view message) {
    const auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    const auto* level_name = level == LogLevel::info ? "info" : level == LogLevel::warning ? "warning" : "error";
    std::tm local_time{};
    localtime_r(&now, &local_time);
    std::clog << '[' << std::put_time(&local_time, "%FT%T") << "] [" << level_name << "] ["
              << component << "] " << message << '\n';
}

}