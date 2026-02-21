#include "nmea/listener.hpp"
#include <algorithm>
#include <format>
#include <linux/can.h>
#include <unistd.h>
#include <utility>

namespace nmea {

Listener::Listener(connection_t conn) : m_conn(conn) {}

Listener::~Listener() {
    if (m_conn != -1) {
        close(m_conn);
    }
}

Listener::Listener(Listener &&other) noexcept : m_conn(std::exchange(other.m_conn, -1)) {}

Listener &Listener::operator=(Listener &&other) noexcept {
    if (this != &other) {
        if (m_conn != -1) {
            close(m_conn);
        }
        m_conn = std::exchange(other.m_conn, -1);
    }

    return *this;
}

void Listener::handle_tp_bam(uint8_t source, const can_frame &frame) {
    TpTransfer transfer;
    transfer.total_size = static_cast<uint16_t>(frame.data[1] | (frame.data[2] << 8));
    transfer.total_packets = frame.data[3];
    transfer.pgn =
        uint32_t(frame.data[5]) | (uint32_t(frame.data[6]) << 8) | (uint32_t(frame.data[7]) << 16);
    transfer.next_packet = 1;
    transfer.buffer.resize(transfer.total_size);
    m_tp_transfers[source] = std::move(transfer);
}

std::optional<std::expected<NmeaMessage, std::string>>
Listener::handle_tp_dt(uint8_t source, const can_frame &frame) {
    auto iter = m_tp_transfers.find(source);
    if (iter == m_tp_transfers.end()) {
        return std::unexpected(std::format("Unexpected TP data packet from source {:02X}", source));
    }
    auto &transfer = iter->second;
    const uint8_t seq = frame.data[0];
    if (seq != transfer.next_packet) {
        m_tp_transfers.erase(iter);
        return std::unexpected(
            std::format("Out of order TP packet: expected {}, got {}", transfer.next_packet, seq));
    }
    const size_t offset = (seq - 1) * 7;
    const size_t bytes = std::min<size_t>(7, transfer.total_size - offset);
    std::copy(frame.data + 1, frame.data + 1 + static_cast<ptrdiff_t>(bytes),
              transfer.buffer.begin() + static_cast<ptrdiff_t>(offset));
    transfer.next_packet++;
    if (seq < transfer.total_packets) {
        // Not all packets have been sent yet
        return std::nullopt;
    }
    auto result = parse(transfer.pgn << 8, transfer.buffer);
    m_tp_transfers.erase(iter);
    return result;
}

std::expected<NmeaMessage, std::string> Listener::read() {
    while (true) {
        can_frame frame{};
        auto nbytes = ::read(m_conn, &frame, sizeof(frame));
        if (nbytes < 0) {
            return std::unexpected("Unable to read from socket");
        }
        if (nbytes < static_cast<ssize_t>(sizeof(frame))) {
            return std::unexpected("Incomplete CAN frame");
        }

        const uint8_t source = frame.can_id & 0xFF;
        const uint8_t pf = (frame.can_id >> 16) & 0xFF;

        // Handle transport protocol
        // Ref: https://embeddedflakes.com/j1939-transport-protocol/
        if (pf == 0xEC && frame.data[0] == 0x20) {
            handle_tp_bam(source, frame);
        } else if (pf == 0xEB) {
            if (auto result = handle_tp_dt(source, frame)) {
                return *result;
            }
        } else {
            return parse(frame.can_id, frame.data);
        }
    }
}
} // namespace nmea
