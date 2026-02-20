#include "nmea/definitions.hpp"
#include "nmea/device.hpp"
#include <chrono>
#include <cstdint>
#include <gtest/gtest.h>
#include <linux/can.h>
#include <optional>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

constexpr uint32_t unique_number = 42;

class AddressClaimTest : public ::testing::Test {
protected:
    int other_device;
    std::optional<nmea::Device> device;

    nmea::DeviceName arbitrary_name{
        .unique_number = unique_number,
        .manufacturer_code = ManufacturerCode::ACTISENSE,
        .device_instance_lower = 0,
        .device_instance_upper = 0,
        .device_function = device_function::RADAR,
        .system_instance = 0,
        .industry_group = IndustryCode::MARINE,
        .arbitrary_address_capable = true,
    };

    nmea::DeviceName non_arbitrary_name{
        .unique_number = unique_number,
        .manufacturer_code = ManufacturerCode::ACTISENSE,
        .device_instance_lower = 0,
        .device_instance_upper = 0,
        .device_function = device_function::RADAR,
        .system_instance = 0,
        .industry_group = IndustryCode::MARINE,
        .arbitrary_address_capable = false,
    };

    void SetUp() override {
        int fds[2];
        socketpair(AF_UNIX, SOCK_STREAM, 0, fds);
        other_device = fds[1];
        device.emplace(fds[0]);
    }

    void TearDown() override { close(other_device); }

    void send_other_device_address(uint8_t address) {
        can_frame frame{};
        frame.can_id = CAN_EFF_FLAG | (6u << 26) | (0xEEu << 16) | (0xFFu << 8) | address;
        frame.can_dlc = 8;
        std::fill(std::begin(frame.data), std::end(frame.data), 0);
        write(other_device, &frame, sizeof(frame));
    }
};

TEST_F(AddressClaimTest, SuccessfulClaimWithNoConflict) {
    auto result = device->claim(arbitrary_name).get();
    ASSERT_TRUE(result.has_value());
}

TEST_F(AddressClaimTest, ConflictCausesAddressChange) {
    auto future = device->claim(arbitrary_name);

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    send_other_device_address(unique_number);

    auto result = future.get();
    ASSERT_TRUE(result.has_value());
}

TEST_F(AddressClaimTest, ConflictCausesAddressToIncrement) {
    auto future = device->claim(arbitrary_name);

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    send_other_device_address(unique_number);

    ASSERT_TRUE(future.get().has_value());
    EXPECT_EQ(device->address(), unique_number + 1);
}

TEST_F(AddressClaimTest, SecondClaimWhileInProgressReturnsError) {
    auto first = device->claim(arbitrary_name);
    auto second = device->claim(arbitrary_name);

    EXPECT_TRUE(first.get().has_value());
    ASSERT_FALSE(second.get().has_value());
    EXPECT_EQ(second.get().error(), "Address claim already in progress");
}

TEST_F(AddressClaimTest, CanClaimAgainAfterCompletion) {
    ASSERT_TRUE(device->claim(arbitrary_name).get().has_value());
    ASSERT_TRUE(device->claim(arbitrary_name).get().has_value());
}

TEST_F(AddressClaimTest, ConflictFailsWhenNotArbitraryAddressCapable) {
    auto future = device->claim(non_arbitrary_name);

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    send_other_device_address(unique_number);

    auto result = future.get();
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), "Address conflict. Device not arbitrary address capable");
}
