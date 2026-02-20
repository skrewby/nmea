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
