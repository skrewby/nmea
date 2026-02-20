#pragma once

#include "nmea/connection.hpp"
#include "nmea/definitions.hpp"
#include "nmea/message.hpp"
#include <cstdint>
#include <expected>
#include <future>
#include <optional>
#include <string>

namespace nmea {

/// Device information (NAME). This is a 64 bit frame. If any field is larger than the
/// maximum length, then the upper bits are ignored.
struct DeviceName {
    uint32_t unique_number;             // 21 bits
    ManufacturerCode manufacturer_code; // 11 bits
    uint8_t device_instance_lower;      // 3 bits
    uint8_t device_instance_upper;      // 5 bits
    DeviceFunction device_function;     // class (7 bits) + function (8 bits)
    uint8_t system_instance;            // 4 bits
    IndustryCode industry_group;        // 3 bits
    bool arbitrary_address_capable;     // 1 bit
};

class Device {
public:
    /// Create a NMEA2000 device that is able to communicate on the bus
    ///
    /// This object will then own the socket it is connected to and will be responsible
    /// for closing it once done
    explicit Device(connection_t conn);
    Device() = delete;
    ~Device();

    Device(const Device &other) = delete;
    Device &operator=(const Device &other) = delete;
    Device(Device &&other) noexcept;
    Device &operator=(Device &&other) noexcept;

    int sockfd() const { return m_conn; }

    std::optional<uint8_t> address() const { return m_address; }

    std::shared_future<std::expected<void, std::string>> claim(DeviceName name);
    std::expected<void, std::string> send(const NmeaMessage &msg, uint8_t priority = 6);

private:
    connection_t m_conn;
    std::optional<uint8_t> m_address;
    std::shared_future<std::expected<void, std::string>> m_claim_future;
};

} // namespace nmea
