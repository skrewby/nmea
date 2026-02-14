#include "nmea/connection.hpp"
#include "utils.hpp"

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
} // namespace nmea
