#include "nmea/listener.hpp"
#include "nmea/connection.hpp"
#include "nmea/message.hpp"
#include "nmea/visit.hpp"
#include <cstdlib>
#include <poll.h>
#include <print>

constexpr int POLL_TIMEOUT = -1;

void process_message(nmea::NmeaMessage msg) {
    nmea::visit(
        msg,
        [](nmea::message::CogSog &m) {
            // Can use the message payload as necessary, eg:
            //  auto speed = m.sog;
            // The message inherits from the string formatter and can be used
            // as below to print the following:
            //  COGSOG(SID=1, Reference=0, COG=1 radians, SOG=0.4 m/s)
            std::println("{}", m);
        },
        [](nmea::message::Temperature &m) {
            // auto temp = m.actual_temperature;
            // auto source = m.source;
            std::println("{}", m);
        },
        [](auto &) { std::println("Unhandled message type"); });
}

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
    pollfd pollfd{.fd = listener.sockfd(), .events = POLLIN, .revents = {}};
    while (true) {
        auto events_num = poll(&pollfd, 1, POLL_TIMEOUT);

        if (events_num > 0) {
            auto msg = listener.read();
            if (msg) {
                process_message(*msg);
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
