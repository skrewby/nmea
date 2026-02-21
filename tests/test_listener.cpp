#include "nmea/listener.hpp"
#include "nmea/message.hpp"
#include <cstdint>
#include <gtest/gtest.h>
#include <linux/can.h>
#include <optional>
#include <sys/socket.h>
#include <unistd.h>

static constexpr uint8_t bam_control_byte = 0x20;

static canid_t make_can_id(uint32_t pgn, uint8_t priority = 6, uint8_t source = 0x16) {
    return (priority << 26) | (pgn << 8) | source;
}

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

    void send_tp(uint32_t pgn, uint8_t priority, std::initializer_list<uint8_t> payload) {
        const uint8_t source = 0x16;
        const auto data = std::vector<uint8_t>(payload);
        const auto total_packets = static_cast<uint8_t>((data.size() + 6) / 7);

        can_frame bam{};
        bam.can_id = (canid_t(priority) << 26) | (0xEC << 16) | (0xFF << 8) | source;
        bam.can_dlc = 8;
        bam.data[0] = bam_control_byte;
        bam.data[1] = data.size() & 0xFF;
        bam.data[2] = (data.size() >> 8) & 0xFF;
        bam.data[3] = total_packets;
        bam.data[4] = 0xFF;
        bam.data[5] = pgn & 0xFF;
        bam.data[6] = (pgn >> 8) & 0xFF;
        bam.data[7] = (pgn >> 16) & 0xFF;
        write(writer, &bam, sizeof(bam));

        for (uint8_t seq = 1; seq <= total_packets; seq++) {
            can_frame dt{};
            dt.can_id = (canid_t(priority) << 26) | (0xEB << 16) | (0xFF << 8) | source;
            dt.can_dlc = 8;
            dt.data[0] = seq;
            const size_t offset = (seq - 1) * 7;
            for (int i = 0; i < 7; i++) {
                const size_t idx = offset + i;
                dt.data[i + 1] = idx < data.size() ? data[idx] : 0xFF;
            }
            write(writer, &dt, sizeof(dt));
        }
    }
};

TEST_F(ListenerTest, Parse129026) {
    send_frame(make_can_id(nmea::pgn::COG_SOG, 2),
               {0x01, 0x00, 0x20, 0x4E, 0x34, 0x12, 0x00, 0x00});

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
    send_frame(make_can_id(nmea::pgn::TEMPERATURE, 5),
               {0x01, 0x01, 0x02, 0xE8, 0x03, 0xFF, 0xFF, 0xFF});

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

TEST_F(ListenerTest, Parse130578) {
    send_tp(nmea::pgn::VESSEL_SPEED, 5,
            {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0D, 0x0C});

    auto result = listener->read();
    ASSERT_TRUE(result.has_value());

    auto *msg = std::get_if<nmea::message::VesselSpeedComponents>(&*result);
    ASSERT_NE(msg, nullptr);
    EXPECT_DOUBLE_EQ(msg->longitudinal.water, 0x0201 * 0.001);
    EXPECT_DOUBLE_EQ(msg->transverse.water, 0x0403 * 0.001);
    EXPECT_DOUBLE_EQ(msg->longitudinal.ground, 0x0605 * 0.001);
    EXPECT_DOUBLE_EQ(msg->transverse.ground, 0x0807 * 0.001);
    EXPECT_DOUBLE_EQ(msg->stern.water, 0x0A09 * 0.001);
    EXPECT_DOUBLE_EQ(msg->stern.ground, 0x0C0D * 0.001);
}
