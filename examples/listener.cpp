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

    auto conn = nmea::connect(argv[1]);
    if (!conn) {
        std::println("Error on connection: {}", conn.error());
        return EXIT_FAILURE;
    }

    nmea::Listener listener(*conn);
    pollfd pollfd{.fd = *conn, .events = POLLIN, .revents = {}};
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
