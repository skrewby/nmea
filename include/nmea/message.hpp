#pragma once

#include <cstdint>
#include <expected>
#include <format>
#include <span>
#include <string>
#include <variant>

namespace nmea {
namespace pgn {
constexpr uint32_t COG_SOG = 129026;
constexpr uint32_t TEMPERATURE = 130312;
} // namespace pgn

namespace message {
/// PGN 129026 - COG & SOG, Rapid Update
struct CogSog {
    uint8_t sid;
    uint8_t cog_reference; // 0 = true, 1 = magnetic
    double cog;            // radians
    double sog;            // m/s
};

/// PGN 130312 - Temperature
struct Temperature {
    uint8_t sid;
    uint8_t instance;
    uint8_t source;
    double actual_temperature; // K
    double set_temperature;    // K
};

} // namespace message

using NmeaMessage = std::variant<message::CogSog, message::Temperature>;

std::expected<NmeaMessage, std::string> parse(uint32_t id, std::span<const uint8_t> data);
} // namespace nmea

template <> struct std::formatter<nmea::message::CogSog> : std::formatter<std::string> {
    auto format(const nmea::message::CogSog &m, auto &ctx) const {
        return std::formatter<std::string>::format(
            std::format("COGSOG(SID={}, Reference={}, COG={} radians, SOG={} m/s)", m.sid,
                        m.cog_reference, m.cog, m.sog),
            ctx);
    }
};

template <> struct std::formatter<nmea::message::Temperature> : std::formatter<std::string> {
    auto format(const nmea::message::Temperature &m, auto &ctx) const {
        return std::formatter<std::string>::format(
            std::format("Temperature(SID={}, Instance={}, Source={} Actual Temperature={} K, Set "
                        "Temperature={} K)",
                        m.sid, m.instance, m.source, m.actual_temperature, m.set_temperature),
            ctx);
    }
};

template <> struct std::formatter<nmea::NmeaMessage> : std::formatter<std::string> {
    auto format(const nmea::NmeaMessage &msg, auto &ctx) const {
        return std::visit(
            [&](const auto &m) {
                return std::formatter<std::string>::format(std::format("{}", m), ctx);
            },
            msg);
    }
};
