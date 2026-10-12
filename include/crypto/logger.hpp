#pragma once

#include <string_view>

namespace crypto {

enum class LogLevel { info, warning, error };

void log(LogLevel level, std::string_view component, std::string_view message);

}