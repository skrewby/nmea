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

namespace nmea {

std::expected<connection_t, std::string> connect(std::string_view interface) {
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
    return sockfd;
}

Listener::Listener(connection_t conn) : m_conn(conn) {}

Listener::Listener(Listener &&other) noexcept : m_conn(other.m_conn) {}

Listener &Listener::operator=(Listener &&other) noexcept {
    if (this != &other) {
        m_conn = other.m_conn;
    }

    return *this;
}

std::expected<std::string, std::string> Listener::read() {
    can_frame frame{};
    auto nbytes = ::read(m_conn, &frame, sizeof(frame));
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
