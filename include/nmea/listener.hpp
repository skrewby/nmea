#pragma once

#include <expected>
#include <string>

#include "nmea/connection.hpp"
#include "nmea/message.hpp"

namespace nmea {

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
    connection_t m_conn;
};

} // namespace nmea
