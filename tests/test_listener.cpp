#include "nmea/listener.hpp"
#include "nmea/message.hpp"
#include <gtest/gtest.h>
#include <linux/can.h>
#include <optional>
#include <sys/socket.h>
#include <unistd.h>

class ListenerTest : public ::testing::Test {
protected:
    int writer;
    std::optional<nmea::Listener> listener;

    void SetUp() override {
        int fds[2];
        socketpair(AF_UNIX, SOCK_STREAM, 0, fds);
        writer = fds[1];

        listener.emplace(fds[0]);
    }

    void TearDown() override { close(writer); }

    void send_frame(canid_t id, std::initializer_list<uint8_t> data) {
        can_frame frame{.can_id = id, .can_dlc = static_cast<uint8_t>(data.size())};
        std::copy(data.begin(), data.end(), frame.data);
        write(writer, &frame, sizeof(frame));
    }
};

TEST_F(ListenerTest, Parse129026) {
    send_frame(0x09F80216, {0x01, 0x00, 0x20, 0x4E, 0x34, 0x12, 0x00, 0x00});

    auto result = listener->read();
    ASSERT_TRUE(result.has_value());

    auto *msg = std::get_if<nmea::message::CogSog>(&*result);
    ASSERT_NE(msg, nullptr);
    EXPECT_EQ(msg->sid, 0x01);
    EXPECT_EQ(msg->cog_reference, 0x00);
    EXPECT_DOUBLE_EQ(msg->cog, 0x4E20 * 1e-4);
    EXPECT_DOUBLE_EQ(msg->sog, 0x1234 * 1e-2);
}

TEST_F(ListenerTest, Parse130312) {
    send_frame(0x15FD0816, {0x01, 0x01, 0x02, 0xE8, 0x03, 0xFF, 0xFF, 0xFF});

    auto result = listener->read();
    ASSERT_TRUE(result.has_value());

    auto *msg = std::get_if<nmea::message::Temperature>(&*result);
    ASSERT_NE(msg, nullptr);
    EXPECT_EQ(msg->sid, 0x01);
    EXPECT_EQ(msg->instance, 0x01);
    EXPECT_EQ(msg->source, 0x02);
    EXPECT_DOUBLE_EQ(msg->actual_temperature, 0x03E8 * 0.01);
    EXPECT_DOUBLE_EQ(msg->set_temperature, 0xFFFF * 0.01);
}
