#include "nmea/nmea.hpp"
#include <gtest/gtest.h>

TEST(ListenerTest, ReadBeforeConnection) {
    nmea::Listener listener;
    EXPECT_FALSE(listener.socketfd().has_value());

    auto res = listener.read();
    EXPECT_FALSE(res.has_value());
}
