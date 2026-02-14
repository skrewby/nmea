#include "nmea/nmea.hpp"
#include <gtest/gtest.h>
#include <linux/can.h>
#include <sys/socket.h>
#include <unistd.h>

class ListenerTest : public ::testing::Test {
protected:
    int conn;
    int writer;

    void SetUp() override {
        int fds[2];
        socketpair(AF_UNIX, SOCK_STREAM, 0, fds);
        conn = fds[0];
        writer = fds[1];
    }

    void TearDown() override {
        close(conn);
        close(writer);
    }

    void send_frame(canid_t id, std::initializer_list<uint8_t> data) {
        can_frame frame{.can_id = id, .can_dlc = static_cast<uint8_t>(data.size())};
        std::copy(data.begin(), data.end(), frame.data);
        write(writer, &frame, sizeof(frame));
    }
};

TEST_F(ListenerTest, ReadAndFormatCanFrames) {
    nmea::Listener listener(conn);
    send_frame(0x123, {0xAB, 0xCD, 0xEF});

    auto result = listener.read();
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, "123#ABCDEF");
}
