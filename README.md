# NMEA

Modern C++ library to communicate with NMEA2000 networks.

## Usage

This library can be used with polling as follows:

```cpp
int main() {
    auto conn = nmea::connect("can0");
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
```

It provides a visitor abstraction over `std::visit` to process the message:

```cpp
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
            // Prints the following:
            //  Temperature(SID=1, Instance=1, Source=2 Actual Temperature=3.14 K, Set Temperature=0.00 K)
            std::println("{}", m);
        },
        [](auto &) { std::println("Unhandled message type"); });
}
```

## References

This is a hobby project to learn the NMEA standard, for use in the real world refer to the following libraries:

- [Canboat](https://github.com/canboat/canboat)
- [ttlappalainen/NMEA2000](https://github.com/ttlappalainen/NMEA2000)
