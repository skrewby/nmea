# NMEA

Modern C++ library to communicate with NMEA2000 networks.

## Usage

### Listener

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
void process_message(nmea::NmeaMessage &msg) {
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

### Device

The library can also function as a device on the bus and is able to send the supported messages

```cpp
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

    nmea::DeviceName name{
        .unique_number = 120,
        .manufacturer_code = ManufacturerCode::ACTISENSE,
        .device_instance_lower = 0,
        .device_instance_upper = 0,
        .device_function = device_function::ATMOSPHERIC,
        .system_instance = 0,
        .industry_group = IndustryCode::MARINE,
        .arbitrary_address_capable = true,
    };

    nmea::Device device(*conn);
    auto result = device.claim(name).get();
    if (!result) {
        std::println("Failed to claim address: {}", result.error());
        return EXIT_FAILURE;
    }

    nmea::message::CogSog cogsog{
        .sid = 1,
        .cog_reference = 0,
        .cog = 0x1234 * 1e-4,
        .sog = 0x5678 * 1e-2,
    };
    auto send_res = device.send(cogsog);
    if (!send_res) {
        std::println("Error sending COGSOG message: {}", send_res.error());
        return EXIT_FAILURE;
    }

    nmea::message::Temperature temp{
        .sid = 2,
        .instance = 1,
        .source = 3,
        .actual_temperature = 0x1234 * 0.01,
        .set_temperature = 0x5678 * 0.01,
    };
    send_res = device.send(temp);
    if (!send_res) {
        std::println("Error sending Temperature message: {}", send_res.error());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
```

### Loopback

It is possible to test the listener and device functionality by making them communicate with each other over a virtual can interface. First, set it up with

```sh
./run.sh setup
```

Then start up the listener

```sh
./run.sh listener
```

And finally send the messages and confirm the listener prints the formatted output

```sh
./run.sh device
```

## Examples

See the `examples` folder or projects that utilize this library:

- [Actisense NGX-1-ISO Tester](https://github.com/skrewby/actisense_gateway_tester)

## References

This is a hobby project to learn the NMEA standard, for use in the real world refer to the following libraries:

- [Canboat](https://github.com/canboat/canboat)
- [ttlappalainen/NMEA2000](https://github.com/ttlappalainen/NMEA2000)
