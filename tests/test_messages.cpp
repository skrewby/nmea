#include "nmea/definitions.hpp"
#include "nmea/device.hpp"
#include "nmea/listener.hpp"
#include "nmea/message.hpp"
#include <gtest/gtest.h>
#include <optional>
#include <sys/socket.h>
#include <unistd.h>
#include <utility>

class MessageTest : public ::testing::Test {
protected:
    std::optional<nmea::Device> device;
    std::optional<nmea::Listener> listener;

    nmea::DeviceName name{
        .unique_number = 42,
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
        device.emplace(fds[0]);
        listener.emplace(fds[1]);
        device->claim(name).get();
        // Needed to flush address claim from the buffer
        auto _ = listener->read();
    }
};

TEST_F(MessageTest, CogSog) {
    nmea::message::CogSog original{
        .sid = 1,
        .cog_reference = 0,
        .cog = 0x1234 * 0.0001,
        .sog = 0x5678 * 0.01,
    };

    ASSERT_TRUE(device->send(original).has_value());

    auto result = listener->read();
    ASSERT_TRUE(result.has_value());
    auto *msg = std::get_if<nmea::message::CogSog>(&*result);
    ASSERT_NE(msg, nullptr);
    EXPECT_EQ(msg->sid, original.sid);
    EXPECT_EQ(msg->cog_reference, original.cog_reference);
    EXPECT_DOUBLE_EQ(msg->cog, original.cog);
    EXPECT_DOUBLE_EQ(msg->sog, original.sog);
}

TEST_F(MessageTest, Temperature) {
    nmea::message::Temperature original{
        .sid = 2,
        .instance = 1,
        .source = 3,
        .actual_temperature = 0x1234 * 0.01,
        .set_temperature = 0x5678 * 0.01,
    };

    ASSERT_TRUE(device->send(original).has_value());

    auto result = listener->read();
    ASSERT_TRUE(result.has_value());
    auto *msg = std::get_if<nmea::message::Temperature>(&*result);
    ASSERT_NE(msg, nullptr);
    EXPECT_EQ(msg->sid, original.sid);
    EXPECT_EQ(msg->instance, original.instance);
    EXPECT_EQ(msg->source, original.source);
    EXPECT_DOUBLE_EQ(msg->actual_temperature, original.actual_temperature);
    EXPECT_DOUBLE_EQ(msg->set_temperature, original.set_temperature);
}

TEST_F(MessageTest, Attitude) {
    nmea::message::Attitude original{
        .sid = 1,
        .yaw = 0x1234 * 0.0001,
        .pitch = 0x5678 * 0.0001,
        .roll = 0x3ABC * 0.0001,
    };

    ASSERT_TRUE(device->send(original).has_value());

    auto result = listener->read();
    ASSERT_TRUE(result.has_value());
    auto *msg = std::get_if<nmea::message::Attitude>(&*result);
    ASSERT_NE(msg, nullptr);
    EXPECT_EQ(msg->sid, original.sid);
    EXPECT_DOUBLE_EQ(msg->yaw, original.yaw);
    EXPECT_DOUBLE_EQ(msg->pitch, original.pitch);
    EXPECT_DOUBLE_EQ(msg->roll, original.roll);
}

TEST_F(MessageTest, VesselSpeedComponents) {
    // clang-format off
    nmea::message::VesselSpeedComponents original{
        .longitudinal = {.water = 0x0102 * 0.001, .ground = 0x0304 * 0.001},
        .transverse   = {.water = 0x0506 * 0.001, .ground = 0x0708 * 0.001},
        .stern        = {.water = 0x090A * 0.001, .ground = 0x0B0C * 0.001},
    };
    // clang-format on

    ASSERT_TRUE(device->send(original).has_value());

    auto result = listener->read();
    ASSERT_TRUE(result.has_value());
    auto *msg = std::get_if<nmea::message::VesselSpeedComponents>(&*result);
    ASSERT_NE(msg, nullptr);
    EXPECT_DOUBLE_EQ(msg->longitudinal.water, original.longitudinal.water);
    EXPECT_DOUBLE_EQ(msg->longitudinal.ground, original.longitudinal.ground);
    EXPECT_DOUBLE_EQ(msg->transverse.water, original.transverse.water);
    EXPECT_DOUBLE_EQ(msg->transverse.ground, original.transverse.ground);
    EXPECT_DOUBLE_EQ(msg->stern.water, original.stern.water);
    EXPECT_DOUBLE_EQ(msg->stern.ground, original.stern.ground);
}

TEST_F(MessageTest, VesselHeading) {
    nmea::message::VesselHeading original{
        .sid = 1,
        .heading = 0x1234 * 0.0001,
        .deviation = 0x5678 * 0.0001,
        .variation = 0x3ABC * 0.0001,
        .reference = DirectionReference::MAGNETIC,
    };

    ASSERT_TRUE(device->send(original).has_value());

    auto result = listener->read();
    ASSERT_TRUE(result.has_value());
    auto *msg = std::get_if<nmea::message::VesselHeading>(&*result);
    ASSERT_NE(msg, nullptr);
    EXPECT_EQ(msg->sid, original.sid);
    EXPECT_DOUBLE_EQ(msg->heading, original.heading);
    EXPECT_DOUBLE_EQ(msg->deviation, original.deviation);
    EXPECT_DOUBLE_EQ(msg->variation, original.variation);
    EXPECT_DOUBLE_EQ(std::to_underlying(msg->reference), std::to_underlying(original.reference));
}

TEST_F(MessageTest, RateOfTurn) {
    nmea::message::RateOfTurn original{
        .sid = 1,
        .rate = 0x12345678 * 3.125e-08,
    };

    ASSERT_TRUE(device->send(original).has_value());

    auto result = listener->read();
    ASSERT_TRUE(result.has_value());
    auto *msg = std::get_if<nmea::message::RateOfTurn>(&*result);
    ASSERT_NE(msg, nullptr);
    EXPECT_EQ(msg->sid, original.sid);
    EXPECT_DOUBLE_EQ(msg->rate, original.rate);
}

TEST_F(MessageTest, Heave) {
    nmea::message::Heave original{
        .sid = 1,
        .heave = 0x1234 * 0.01,
    };

    ASSERT_TRUE(device->send(original).has_value());

    auto result = listener->read();
    ASSERT_TRUE(result.has_value());
    auto *msg = std::get_if<nmea::message::Heave>(&*result);
    ASSERT_NE(msg, nullptr);
    EXPECT_EQ(msg->sid, original.sid);
    EXPECT_DOUBLE_EQ(msg->heave, original.heave);
}

TEST_F(MessageTest, Position) {
    nmea::message::Position original{
        .latitude = 0x12345678 * 1e-07,
        .longitude = 0x07654321 * 1e-07,
    };

    ASSERT_TRUE(device->send(original).has_value());

    auto result = listener->read();
    ASSERT_TRUE(result.has_value());
    auto *msg = std::get_if<nmea::message::Position>(&*result);
    ASSERT_NE(msg, nullptr);
    EXPECT_DOUBLE_EQ(msg->latitude, original.latitude);
    EXPECT_DOUBLE_EQ(msg->longitude, original.longitude);
}

TEST_F(MessageTest, EnvironmentalParameters) {
    nmea::message::EnvironmentalParameters original{
        .sid = 1,
        .temperature_source = TemperatureSource::ENGINE_ROOM_TEMPERATURE,
        .humidity_source = HumiditySource::INSIDE,
        .temperature = 0x1234 * 0.01,
        .humidity = 0x5678 * 0.004,
        .atmospheric_pressure = 6553200 / 100,
    };

    ASSERT_TRUE(device->send(original).has_value());

    auto result = listener->read();
    ASSERT_TRUE(result.has_value());
    auto *msg = std::get_if<nmea::message::EnvironmentalParameters>(&*result);
    ASSERT_NE(msg, nullptr);
    EXPECT_EQ(msg->sid, original.sid);
    EXPECT_EQ(msg->temperature_source, original.temperature_source);
    EXPECT_EQ(msg->humidity_source, original.humidity_source);
    EXPECT_DOUBLE_EQ(msg->temperature, original.temperature);
    EXPECT_DOUBLE_EQ(msg->humidity, original.humidity);
    EXPECT_EQ(msg->atmospheric_pressure, original.atmospheric_pressure);
}
