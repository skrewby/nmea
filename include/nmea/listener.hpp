#pragma once

#include <cstdint>
#include <expected>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "nmea/connection.hpp"
#include "nmea/message.hpp"

struct can_frame;

namespace nmea {

struct TpTransfer {
    uint32_t pgn;
    uint16_t total_size;
    uint8_t total_packets;
    uint8_t next_packet;
    std::vector<uint8_t> buffer;
};

class Listener {
public:
    /// Create a listener that listens to the passed socketfd.
    ///
    /// This object will then own the socket it is connected to and will be responsible
    /// for closing it once done
    explicit Listener(connection_t conn);
    Listener() = delete;
    ~Listener();

    Listener(const Listener &other) = delete;
    Listener &operator=(const Listener &other) = delete;
    Listener(Listener &&other) noexcept;
    Listener &operator=(Listener &&other) noexcept;

    std::expected<NmeaMessage, std::string> read();

    int sockfd() const { return m_conn; }

private:
    void handle_tp_bam(uint8_t source, const can_frame &frame);
    std::optional<std::expected<NmeaMessage, std::string>> handle_tp_dt(uint8_t source,
                                                                        const can_frame &frame);

    connection_t m_conn;
    std::unordered_map<uint8_t, TpTransfer> m_tp_transfers;
};

} // namespace nmea
