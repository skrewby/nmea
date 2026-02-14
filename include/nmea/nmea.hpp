#pragma once

#include <expected>
#include <optional>
#include <string>
#include <string_view>

namespace nmea {

class Listener {
public:
    Listener() = default;
    ~Listener();

    Listener(const Listener &other) = delete;
    Listener &operator=(const Listener &other) = delete;
    Listener(Listener &&other) noexcept;
    Listener &operator=(Listener &&other) noexcept;

    std::expected<void, std::string> connect(std::string_view interface);

    std::expected<std::string, std::string> read();

    std::optional<int> socketfd() const { return m_socketfd; }

private:
    std::optional<int> m_socketfd;
};

} // namespace nmea
