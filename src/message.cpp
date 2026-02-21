#include "nmea/message.hpp"
#include "nmea/visit.hpp"
#include <cmath>
#include <cstdint>
#include <format>
#include <vector>

namespace nmea {
static uint16_t read_u16(std::span<const uint8_t> data, size_t idx) {
    return static_cast<uint16_t>(data[idx] | (data[idx + 1] << 8));
}

static int16_t read_i16(std::span<const uint8_t> data, size_t idx) {
    return static_cast<int16_t>(data[idx] | (data[idx + 1] << 8));
}

static void write_u16(std::vector<uint8_t> &data, size_t idx, uint16_t val) {
    data[idx] = static_cast<uint8_t>(val);
    data[idx + 1] = static_cast<uint8_t>(val >> 8);
}

// ============================== 129026 - COG & SOG, Rapid Update ============================== //
static message::CogSog parse_cogsog(std::span<const uint8_t> data) {
    message::CogSog msg{};

    msg.sid = data[0];
    msg.cog_reference = data[1] & 0x03;
    msg.cog = read_u16(data, 2) * 0.0001;
    msg.sog = read_u16(data, 4) * 0.01;

    return msg;
}

static SerializedMessage serialize_cogsog(const message::CogSog &msg) {
    std::vector<uint8_t> data(8, 0);
    data[0] = msg.sid;
    data[1] = msg.cog_reference & 0x03;
    write_u16(data, 2, static_cast<uint16_t>(std::lround(msg.cog / 0.0001)));
    write_u16(data, 4, static_cast<uint16_t>(std::lround(msg.sog / 0.01)));
    return {pgn::COG_SOG, data};
}

// ==================================== 130312 - Temperature ==================================== //
static message::Temperature parse_temperature(std::span<const uint8_t> data) {
    message::Temperature msg{};

    msg.sid = data[0];
    msg.instance = data[1];
    msg.source = data[2];
    msg.actual_temperature = read_u16(data, 3) * 0.01;
    msg.set_temperature = read_u16(data, 5) * 0.01;

    return msg;
}

static SerializedMessage serialize_temperature(const message::Temperature &msg) {
    std::vector<uint8_t> data(8, 0);
    data[0] = msg.sid;
    data[1] = msg.instance;
    data[2] = msg.source;
    write_u16(data, 3, static_cast<uint16_t>(std::lround(msg.actual_temperature / 0.01)));
    write_u16(data, 5, static_cast<uint16_t>(std::lround(msg.set_temperature / 0.01)));
    return {pgn::TEMPERATURE, data};
}

// ============================== 130578 - Vessel Speed Components ============================== //
static message::VesselSpeedComponents parse_vessel_speed_components(std::span<const uint8_t> data) {
    message::VesselSpeedComponents msg{};

    msg.longitudinal.water = read_i16(data, 0) * 0.001;
    msg.transverse.water = read_i16(data, 2) * 0.001;
    msg.longitudinal.ground = read_i16(data, 4) * 0.001;
    msg.transverse.ground = read_i16(data, 6) * 0.001;
    msg.stern.water = read_i16(data, 8) * 0.001;
    msg.stern.ground = read_i16(data, 10) * 0.001;

    return msg;
}

static SerializedMessage
serialize_vessel_speed_components(const message::VesselSpeedComponents &msg) {
    std::vector<uint8_t> data(12, 0);

    write_u16(data, 0, static_cast<uint16_t>(std::lround(msg.longitudinal.water / 0.001)));
    write_u16(data, 2, static_cast<uint16_t>(std::lround(msg.transverse.water / 0.001)));
    write_u16(data, 4, static_cast<uint16_t>(std::lround(msg.longitudinal.ground / 0.001)));
    write_u16(data, 6, static_cast<uint16_t>(std::lround(msg.transverse.ground / 0.001)));
    write_u16(data, 8, static_cast<uint16_t>(std::lround(msg.stern.water / 0.001)));
    write_u16(data, 10, static_cast<uint16_t>(std::lround(msg.stern.ground / 0.001)));

    return {pgn::VESSEL_SPEED, data};
}

// ===================================== 127257 - Attitude ====================================== //
static message::Attitude parse_attitude(std::span<const uint8_t> data) {
    message::Attitude msg{};

    msg.sid = data[0];
    msg.yaw = read_i16(data, 1) * 0.0001;
    msg.pitch = read_i16(data, 3) * 0.0001;
    msg.roll = read_i16(data, 5) * 0.0001;

    return msg;
}

static SerializedMessage serialize_attitude(const message::Attitude &msg) {
    std::vector<uint8_t> data(8, 0);

    data[0] = msg.sid;
    write_u16(data, 1, static_cast<uint16_t>(std::lround(msg.yaw / 0.0001)));
    write_u16(data, 3, static_cast<uint16_t>(std::lround(msg.pitch / 0.0001)));
    write_u16(data, 5, static_cast<uint16_t>(std::lround(msg.roll / 0.0001)));

    return {pgn::ATTITUDE, data};
}

// ================================== Public API Implementation ================================= //
std::expected<NmeaMessage, std::string> parse(uint32_t id, std::span<const uint8_t> data) {
    auto msg_pgn = (id >> 8) & 0x3FFFF;
    switch (msg_pgn) {
    case pgn::COG_SOG:
        return parse_cogsog(data);
    case pgn::TEMPERATURE:
        return parse_temperature(data);
    case pgn::VESSEL_SPEED:
        return parse_vessel_speed_components(data);
    case pgn::ATTITUDE:
        return parse_attitude(data);
    default:
        return std::unexpected(std::format("PGN {} not supported", msg_pgn));
    }
}

SerializedMessage serialize(const NmeaMessage &msg) {
    return nmea::visit(
        msg, [](const message::CogSog &m) { return serialize_cogsog(m); },
        [](const message::Temperature &m) { return serialize_temperature(m); },
        [](const message::VesselSpeedComponents &m) {
            return serialize_vessel_speed_components(m);
        },
        [](const message::Attitude &m) { return serialize_attitude(m); });
}
} // namespace nmea
