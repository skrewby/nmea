#include "nmea/device.hpp"
#include "nmea/message.hpp"
#include <gtest/gtest.h>
#include <sys/socket.h>
#include <unistd.h>

TEST(DeviceTest, SendWithoutClaimReturnsError) {
    int fds[2];
    socketpair(AF_UNIX, SOCK_STREAM, 0, fds);
    close(fds[1]);
    nmea::Device device(fds[0]);

    auto result = device.send(nmea::message::CogSog{});
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), "Device has not claimed an address");
}
