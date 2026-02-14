#include "nmea/nmea.hpp"
#include <cstdlib>
#include <poll.h>
#include <print>

constexpr int POLL_TIMEOUT = -1;

int main(int argc, const char **argv) {
    if (argc != 2) {
        std::println("Usage: {} <interface>", argv[0]);
        return EXIT_FAILURE;
    }

    nmea::Listener listener;
    auto res = listener.connect(argv[1]);
    if (!res) {
        std::println("Error on connection: {}", res.error());
        return EXIT_FAILURE;
    }

    pollfd pollfd{.fd = *listener.socketfd(), .events = POLLIN, .revents = {}};
    while (true) {
        auto events_num = poll(&pollfd, 1, POLL_TIMEOUT);

        if (events_num > 0) {
            auto msg = listener.read();
            if (msg) {
                std::println("Received: {}", *msg);
            } else {
                std::println("Error: {}", msg.error());
            }
        } else if (events_num < 0) {
            std::println("Poll error");
            return EXIT_FAILURE;
        }
    }

    return EXIT_SUCCESS;
}
