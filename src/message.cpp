#include "nmea/message.hpp"
#include <cstdint>
#include <format>

namespace nmea {
static uint16_t read_u16(std::span<const uint8_t> data, size_t idx) {
    return static_cast<uint16_t>(data[idx] | (data[idx + 1] << 8));
}

static message::CogSog parse_cogsog(std::span<const uint8_t> data) {
    message::CogSog msg{};

    msg.sid = data[0];
    msg.cog_reference = data[1] & 0x03;
    msg.cog = read_u16(data, 2) * 0.0001;
    msg.sog = read_u16(data, 4) * 0.01;

    return msg;
}

static message::Temperature parse_temperature(std::span<const uint8_t> data) {
    message::Temperature msg{};

    msg.sid = data[0];
    msg.instance = data[1];
    msg.source = data[2];
    msg.actual_temperature = read_u16(data, 3) * 0.01;
    msg.set_temperature = read_u16(data, 5) * 0.01;

    return msg;
}

std::expected<NmeaMessage, std::string> parse(uint32_t id, std::span<const uint8_t> data) {
    auto msg_pgn = (id >> 8) & 0x3FFFF;
    switch (msg_pgn) {
    case pgn::COG_SOG:
        return parse_cogsog(data);
    case pgn::TEMPERATURE:
        return parse_temperature(data);
    default:
        return std::unexpected(std::format("PGN {} not supported", msg_pgn));
    }
}
} // namespace nmea
