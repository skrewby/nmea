#pragma once

#include "nmea/definitions.hpp"
#include <cstdint>
#include <expected>
#include <format>
#include <span>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace nmea {
namespace pgn {
constexpr uint32_t TP_CM = 60416;
constexpr uint32_t TP_DT = 60160;
constexpr uint32_t VESSEL_HEADING = 127250;
constexpr uint32_t RATE_OF_TURN = 127251;
constexpr uint32_t HEAVE = 127252;
constexpr uint32_t ATTITUDE = 127257;
constexpr uint32_t POSITION = 129025;
constexpr uint32_t COG_SOG = 129026;
constexpr uint32_t TEMPERATURE = 130312;
constexpr uint32_t VESSEL_SPEED = 130578;
constexpr uint32_t ENVIRONMENTAL_PARAMETERS = 130311;
constexpr uint32_t ACTUAL_PRESSURE = 130314;
} // namespace pgn

namespace message {
/// PGN 129026 - COG & SOG, Rapid Update
struct CogSog {
    static constexpr uint8_t priority = 2;
    uint8_t sid;
    uint8_t cog_reference; // 0 = true, 1 = magnetic
    double cog;            // radians
    double sog;            // m/s
};

/// PGN 130312 - Temperature
struct Temperature {
    static constexpr uint8_t priority = 6;
    uint8_t sid;
    uint8_t instance;
    uint8_t source;
    double actual_temperature; // K
    double set_temperature;    // K
};

/// PGN 130578 - Vessel Speed Components
struct VesselSpeedComponents {
    static constexpr uint8_t priority = 2;

    struct Ref {
        double water;
        double ground;
    };

    Ref longitudinal; // m/s
    Ref transverse;   // m/s
    Ref stern;        // m/s
};

// PGN 127257 - Attitude
struct Attitude {
    static constexpr uint8_t priority = 3;
    uint8_t sid;
    double yaw;   // radians
    double pitch; // radians
    double roll;  // radians
};

// PGN 127250 - Vessel Heading
struct VesselHeading {
    static constexpr uint8_t priority = 2;
    uint8_t sid;
    double heading;   // radians
    double deviation; // radians
    double variation; // radians
    DirectionReference reference;
};

// PGN 127251 - Rate of Turn
struct RateOfTurn {
    static constexpr uint8_t priority = 2;
    uint8_t sid;
    double rate; // radians
};

// PGN 127252 - Heave
struct Heave {
    static constexpr uint8_t priority = 3;
    uint8_t sid;
    double heave; // m
};

// PGN 129025 - Position, Rapid Update
struct Position {
    static constexpr uint8_t priority = 2;
    double latitude;  // degrees
    double longitude; // degrees
};

// PGN 130311 - Environmental Parameters
struct EnvironmentalParameters {
    static constexpr uint8_t priority = 5;
    uint8_t sid;
    TemperatureSource temperature_source;
    HumiditySource humidity_source;
    double temperature;            // K
    double humidity;               // %
    uint16_t atmospheric_pressure; // Pa
};

// PGN130314 - Actual Pressure
struct ActualPressure {
    static constexpr uint8_t priority = 5;
    uint8_t sid;
    uint8_t instance;
    PressureSource source;
    double pressure; // Pa
};

template <typename T> constexpr uint8_t default_priority(const T &) { return T::priority; }

} // namespace message

using NmeaMessage =
    std::variant<message::CogSog, message::Temperature, message::VesselSpeedComponents,
                 message::Attitude, message::VesselHeading, message::RateOfTurn, message::Heave,
                 message::Position, message::EnvironmentalParameters, message::ActualPressure>;

struct SerializedMessage {
    uint32_t pgn;
    std::vector<uint8_t> data;
};

std::expected<NmeaMessage, std::string> parse(uint32_t id, std::span<const uint8_t> data);
SerializedMessage serialize(const NmeaMessage &msg);
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

template <>
struct std::formatter<nmea::message::VesselSpeedComponents> : std::formatter<std::string> {
    auto format(const nmea::message::VesselSpeedComponents &m, auto &ctx) const {
        return std::formatter<std::string>::format(
            std::format("VesselSpeed(Longitudinal=(Ground: {}, Water: {}), Transverse=(Ground: {}, "
                        "Water: {}), Stern=(Ground: {}, Water: {}))",
                        m.longitudinal.ground, m.longitudinal.water, m.transverse.ground,
                        m.transverse.water, m.stern.ground, m.stern.water),
            ctx);
    }
};

template <> struct std::formatter<nmea::message::Attitude> : std::formatter<std::string> {
    auto format(const nmea::message::Attitude &m, auto &ctx) const {
        return std::formatter<std::string>::format(
            std::format("Attitude(SID={}, Yaw={}, Pitch={}, Roll={})", m.sid, m.yaw, m.pitch,
                        m.roll),
            ctx);
    }
};

template <> struct std::formatter<nmea::message::VesselHeading> : std::formatter<std::string> {
    auto format(const nmea::message::VesselHeading &m, auto &ctx) const {
        return std::formatter<std::string>::format(
            std::format(
                "Vessel Heading(SID={}, Heading={}, Deviation={}, Variation={}, Reference={})",
                m.sid, m.heading, m.deviation, m.variation, std::to_underlying(m.reference)),
            ctx);
    }
};

template <> struct std::formatter<nmea::message::RateOfTurn> : std::formatter<std::string> {
    auto format(const nmea::message::RateOfTurn &m, auto &ctx) const {
        return std::formatter<std::string>::format(
            std::format("Rate of Turn(SID={}, Rate={})", m.sid, m.rate), ctx);
    }
};

template <> struct std::formatter<nmea::message::Heave> : std::formatter<std::string> {
    auto format(const nmea::message::Heave &m, auto &ctx) const {
        return std::formatter<std::string>::format(
            std::format("Heave(SID={}, Heave={})", m.sid, m.heave), ctx);
    }
};

template <> struct std::formatter<nmea::message::Position> : std::formatter<std::string> {
    auto format(const nmea::message::Position &m, auto &ctx) const {
        return std::formatter<std::string>::format(
            std::format("Position(Latitude={}, Longitude={})", m.latitude, m.longitude), ctx);
    }
};

template <>
struct std::formatter<nmea::message::EnvironmentalParameters> : std::formatter<std::string> {
    auto format(const nmea::message::EnvironmentalParameters &m, auto &ctx) const {
        return std::formatter<std::string>::format(
            std::format("Environmental Parameters(SID={}, Temperature Source={}, Humidity "
                        "Source={}, Temperature={}, Humidity={}, Atmospheric Pressure={})",
                        m.sid, std::to_underlying(m.temperature_source),
                        std::to_underlying(m.humidity_source), m.temperature, m.humidity,
                        m.atmospheric_pressure),
            ctx);
    }
};

template <> struct std::formatter<nmea::message::ActualPressure> : std::formatter<std::string> {
    auto format(const nmea::message::ActualPressure &m, auto &ctx) const {
        return std::formatter<std::string>::format(
            std::format("ActualPressure(SID={}, Instance={}, Source={}, Pressure={})", m.sid,
                        m.instance, std::to_underlying(m.source), m.pressure),
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
