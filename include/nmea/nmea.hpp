#pragma once

#include <expected>
#include <string>
#include <string_view>

namespace nmea {

using connection_t = int;

std::expected<connection_t, std::string> connect(std::string_view interface);

class Listener {
public:
    explicit Listener(connection_t conn);
    Listener() = delete;
    ~Listener() = default;

    Listener(const Listener &other) = delete;
    Listener &operator=(const Listener &other) = delete;
    Listener(Listener &&other) noexcept;
    Listener &operator=(Listener &&other) noexcept;

    std::expected<std::string, std::string> read();

private:
    connection_t m_conn;
};

} // namespace nmea
