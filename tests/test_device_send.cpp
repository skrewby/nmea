#include "nmea/definitions.hpp"
#include "nmea/device.hpp"
#include "nmea/message.hpp"
#include <gtest/gtest.h>
#include <linux/can.h>
#include <optional>
#include <sys/socket.h>
#include <unistd.h>

constexpr uint32_t unique_number = 42;
constexpr uint8_t expected_address = unique_number % 252;

class DeviceSendTest : public ::testing::Test {
protected:
    int other_device;
    std::optional<nmea::Device> device;

    nmea::DeviceName name{
        .unique_number = unique_number,
        .manufacturer_code = ManufacturerCode::ACTISENSE,
        .device_instance_lower = 0,
        .device_instance_upper = 0,
        .device_function = device_function::RADAR,
        .system_instance = 0,
        .industry_group = IndustryCode::MARINE,
        .arbitrary_address_capable = true,
    };

    void SetUp() override {
        int fds[2];
        socketpair(AF_UNIX, SOCK_STREAM, 0, fds);
        other_device = fds[1];
        device.emplace(fds[0]);
        device->claim(name).get();
        // Required to remove the claim from the socket buffer
        can_frame frame{};
        read(other_device, &frame, sizeof(frame));
    }

    void TearDown() override { close(other_device); }

    can_frame read_frame() {
        can_frame frame{};
        read(other_device, &frame, sizeof(frame));
        return frame;
    }
};

TEST(DeviceSendTestBeforeClaim, SendWithoutClaimReturnsError) {
    int fds[2];
    socketpair(AF_UNIX, SOCK_STREAM, 0, fds);
    close(fds[1]);
    nmea::Device device(fds[0]);

    auto result = device.send(nmea::message::CogSog{});
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), "Device has not claimed an address");
}

TEST_F(DeviceSendTest, SendCogSog) {
    nmea::message::CogSog original{
        .sid = 1,
        .cog_reference = 0,
        .cog = 0x1234 * 1e-4,
        .sog = 0x5678 * 1e-2,
    };

    ASSERT_TRUE(device->send(original).has_value());

    auto frame = read_frame();
    EXPECT_EQ((frame.can_id >> 8) & 0x3FFFF, nmea::pgn::COG_SOG);
    EXPECT_EQ(frame.can_id & 0xFF, expected_address);

    auto parsed = nmea::parse(frame.can_id, std::span(frame.data, frame.can_dlc));
    ASSERT_TRUE(parsed.has_value());
    auto *msg = std::get_if<nmea::message::CogSog>(&*parsed);
    ASSERT_NE(msg, nullptr);
    EXPECT_EQ(msg->sid, original.sid);
    EXPECT_EQ(msg->cog_reference, original.cog_reference);
    EXPECT_DOUBLE_EQ(msg->cog, original.cog);
    EXPECT_DOUBLE_EQ(msg->sog, original.sog);
}

TEST_F(DeviceSendTest, SendTemperature) {
    nmea::message::Temperature original{
        .sid = 2,
        .instance = 1,
        .source = 3,
        .actual_temperature = 0x1234 * 0.01,
        .set_temperature = 0x5678 * 0.01,
    };

    ASSERT_TRUE(device->send(original).has_value());

    auto frame = read_frame();
    EXPECT_EQ((frame.can_id >> 8) & 0x3FFFF, nmea::pgn::TEMPERATURE);
    EXPECT_EQ(frame.can_id & 0xFF, expected_address);

    auto parsed = nmea::parse(frame.can_id, std::span(frame.data, frame.can_dlc));
    ASSERT_TRUE(parsed.has_value());
    auto *msg = std::get_if<nmea::message::Temperature>(&*parsed);
    ASSERT_NE(msg, nullptr);
    EXPECT_EQ(msg->sid, original.sid);
    EXPECT_EQ(msg->instance, original.instance);
    EXPECT_EQ(msg->source, original.source);
    EXPECT_DOUBLE_EQ(msg->actual_temperature, original.actual_temperature);
    EXPECT_DOUBLE_EQ(msg->set_temperature, original.set_temperature);
}
