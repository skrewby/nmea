#include "nmea/nmea.hpp"
#include "utils.hpp"

#include <format>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <linux/can.h>
#include <linux/can/raw.h>
#include <utility>

namespace nmea {

Listener::~Listener() {
    if (m_socketfd) {
        close(*m_socketfd);
    }
}

Listener::Listener(Listener &&other) noexcept
    : m_socketfd(std::exchange(other.m_socketfd, std::nullopt)) {}

Listener &Listener::operator=(Listener &&other) noexcept {
    if (this != &other) {
        if (m_socketfd) {
            close(*m_socketfd);
        }
        m_socketfd = std::exchange(other.m_socketfd, std::nullopt);
    }

    return *this;
}

std::expected<void, std::string> Listener::connect(std::string_view interface) {
    if (m_socketfd) {
        close(*m_socketfd);
        m_socketfd = std::nullopt;
    }

    int sockfd = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (sockfd == -1) {
        return std::unexpected("Error while opening socket");
    }
    auto guard = scope_exit([&] { close(sockfd); });

    ifreq ifr{};
    interface.copy(ifr.ifr_name, IFNAMSIZ - 1);
    auto res = ioctl(sockfd, SIOCGIFINDEX, &ifr);
    if (res == -1) {
        return std::unexpected("Network interface not found");
    }

    sockaddr_can addr{
        .can_family = AF_CAN,
        .can_ifindex = ifr.ifr_ifindex,
        .can_addr = {},
    };

    res = bind(sockfd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr));
    if (res == -1) {
        return std::unexpected("Error while binding socket");
    }

    guard.release();
    m_socketfd = sockfd;
    return {};
}

std::expected<std::string, std::string> Listener::read() {
    if (!m_socketfd) {
        return std::unexpected("Not connected");
    }

    can_frame frame{};
    auto nbytes = ::read(*m_socketfd, &frame, sizeof(frame));
    if (nbytes < 0) {
        return std::unexpected("Unable to read from socket");
    }
    if (nbytes < static_cast<ssize_t>(sizeof(frame))) {
        return std::unexpected("Incomplete CAN frame");
    }

    auto result = std::format("{:03X}#", frame.can_id & CAN_EFF_MASK);
    for (int i = 0; i < frame.can_dlc; ++i) {
        std::format_to(std::back_inserter(result), "{:02X}", frame.data[i]);
    }

    return result;
}

} // namespace nmea
