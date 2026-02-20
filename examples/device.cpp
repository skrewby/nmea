#include "nmea/device.hpp"
#include "nmea/connection.hpp"
#include "nmea/definitions.hpp"
#include "nmea/device.hpp"
#include <cstdlib>
#include <print>

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
