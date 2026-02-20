#include "nmea/message.hpp"
#include <array>
#include <cmath>
#include <cstdint>
#include <format>

namespace nmea {
static uint16_t read_u16(std::span<const uint8_t> data, size_t idx) {
    return static_cast<uint16_t>(data[idx] | (data[idx + 1] << 8));
}

static void write_u16(std::array<uint8_t, 8> &data, size_t idx, uint16_t val) {
    data[idx] = static_cast<uint8_t>(val);
    data[idx + 1] = static_cast<uint8_t>(val >> 8);
}

static message::CogSog parse_cogsog(std::span<const uint8_t> data) {
    message::CogSog msg{};

    msg.sid = data[0];
    msg.cog_reference = data[1] & 0x03;
    msg.cog = read_u16(data, 2) * 0.0001;
    msg.sog = read_u16(data, 4) * 0.01;

    return msg;
}

static SerializedMessage serialize_cogsog(const message::CogSog &msg) {
    std::array<uint8_t, 8> data{};
    data[0] = msg.sid;
    data[1] = msg.cog_reference & 0x03;
    write_u16(data, 2, static_cast<uint16_t>(std::lround(msg.cog / 0.0001)));
    write_u16(data, 4, static_cast<uint16_t>(std::lround(msg.sog / 0.01)));
    return {pgn::COG_SOG, data};
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

static SerializedMessage serialize_temperature(const message::Temperature &msg) {
    std::array<uint8_t, 8> data{};
    data[0] = msg.sid;
    data[1] = msg.instance;
    data[2] = msg.source;
    write_u16(data, 3, static_cast<uint16_t>(std::lround(msg.actual_temperature / 0.01)));
    write_u16(data, 5, static_cast<uint16_t>(std::lround(msg.set_temperature / 0.01)));
    return {pgn::TEMPERATURE, data};
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

SerializedMessage serialize(const NmeaMessage &msg) {
    return std::visit(
        [](const auto &m) -> SerializedMessage {
            using T = std::decay_t<decltype(m)>;
            if constexpr (std::is_same_v<T, message::CogSog>) {
                return serialize_cogsog(m);
            } else {
                return serialize_temperature(m);
            }
        },
        msg);
}
} // namespace nmea
