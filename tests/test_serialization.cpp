#include "nmea/message.hpp"
#include <gtest/gtest.h>

TEST(SerializationTest, CogSogRoundTrip) {
    nmea::message::CogSog original{
        .sid = 1,
        .cog_reference = 0,
        .cog = 0x1234 * 1e-4,
        .sog = 0x5678 * 1e-2,
    };

    auto serialized = nmea::serialize(original);
    ASSERT_EQ(serialized.pgn, nmea::pgn::COG_SOG);

    auto parsed = nmea::parse(serialized.pgn << 8, serialized.data);
    ASSERT_TRUE(parsed.has_value());

    auto *msg = std::get_if<nmea::message::CogSog>(&*parsed);
    ASSERT_NE(msg, nullptr);
    EXPECT_EQ(msg->sid, original.sid);
    EXPECT_EQ(msg->cog_reference, original.cog_reference);
    EXPECT_DOUBLE_EQ(msg->cog, original.cog);
    EXPECT_DOUBLE_EQ(msg->sog, original.sog);
}

TEST(SerializationTest, TemperatureRoundTrip) {
    nmea::message::Temperature original{
        .sid = 1,
        .instance = 2,
        .source = 3,
        .actual_temperature = 0x1234 * 0.01,
        .set_temperature = 0x5678 * 0.01,
    };

    auto serialized = nmea::serialize(original);
    ASSERT_EQ(serialized.pgn, nmea::pgn::TEMPERATURE);

    auto parsed = nmea::parse(serialized.pgn << 8, serialized.data);
    ASSERT_TRUE(parsed.has_value());

    auto *msg = std::get_if<nmea::message::Temperature>(&*parsed);
    ASSERT_NE(msg, nullptr);
    EXPECT_EQ(msg->sid, original.sid);
    EXPECT_EQ(msg->instance, original.instance);
    EXPECT_EQ(msg->source, original.source);
    EXPECT_DOUBLE_EQ(msg->actual_temperature, original.actual_temperature);
    EXPECT_DOUBLE_EQ(msg->set_temperature, original.set_temperature);
}

TEST(SerializationTest, VesselSpeedComponentsRoundTrip) {
    // clang-format off
    nmea::message::VesselSpeedComponents original{
        .longitudinal = {
            .water = 0x0102 * 0.001,
            .ground = 0x0304 * 0.001,
        },
        .transverse = {
            .water = 0x0506 * 0.001,
            .ground = 0x0708 * 0.001,
        },
        .stern = {
            .water = 0x090A * 0.001,
            .ground = 0x0B0C * 0.001,
        },
    };
    // clang-format on

    auto serialized = nmea::serialize(original);
    ASSERT_EQ(serialized.pgn, nmea::pgn::VESSEL_SPEED);

    auto parsed = nmea::parse(serialized.pgn << 8, serialized.data);
    ASSERT_TRUE(parsed.has_value());

    auto *msg = std::get_if<nmea::message::VesselSpeedComponents>(&*parsed);
    ASSERT_NE(msg, nullptr);
    EXPECT_EQ(msg->longitudinal.water, original.longitudinal.water);
    EXPECT_EQ(msg->longitudinal.ground, original.longitudinal.ground);
    EXPECT_EQ(msg->transverse.water, original.transverse.water);
    EXPECT_EQ(msg->transverse.ground, original.transverse.ground);
    EXPECT_EQ(msg->stern.water, original.stern.water);
    EXPECT_EQ(msg->stern.ground, original.stern.ground);
}

TEST(SerializationTest, AttitudeRoundTrip) {
    nmea::message::Attitude original{
        .sid = 1,
        .yaw = 0x1234 * 0.0001,
        .pitch = 0x5678 * 0.0001,
        .roll = 0x3ABC * 0.0001,
    };

    auto serialized = nmea::serialize(original);
    ASSERT_EQ(serialized.pgn, nmea::pgn::ATTITUDE);

    auto parsed = nmea::parse(serialized.pgn << 8, serialized.data);
    ASSERT_TRUE(parsed.has_value());

    auto *msg = std::get_if<nmea::message::Attitude>(&*parsed);
    ASSERT_NE(msg, nullptr);
    EXPECT_EQ(msg->sid, original.sid);
    EXPECT_DOUBLE_EQ(msg->yaw, original.yaw);
    EXPECT_DOUBLE_EQ(msg->pitch, original.pitch);
    EXPECT_DOUBLE_EQ(msg->roll, original.roll);
}
