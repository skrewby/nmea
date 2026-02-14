#pragma once

#include <expected>
#include <string>
#include <string_view>

namespace nmea {
using connection_t = int;

std::expected<connection_t, std::string> connect(std::string_view interface);
} // namespace nmea
