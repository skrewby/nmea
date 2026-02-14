#include "nmea/listener.hpp"
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

std::expected<NmeaMessage, std::string> Listener::read() {
    can_frame frame{};
    auto nbytes = ::read(m_conn, &frame, sizeof(frame));
    if (nbytes < 0) {
        return std::unexpected("Unable to read from socket");
    }
    if (nbytes < static_cast<ssize_t>(sizeof(frame))) {
        return std::unexpected("Incomplete CAN frame");
    }

    return parse(frame.can_id, frame.data);
}
} // namespace nmea
