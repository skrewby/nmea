#include "nmea/device.hpp"
#include "nmea/message.hpp"
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <future>
#include <linux/can.h>
#include <poll.h>
#include <unistd.h>
#include <utility>

namespace nmea {

constexpr uint32_t PGN_ADDRESS_CLAIM_PRIORITY = 6u;
constexpr uint32_t PGN_ADDRESS_CLAIM_PF = 0xEEu;
constexpr uint32_t DESTINATION_GLOBAL = 0xFFu;
constexpr uint8_t NULL_ADDRESS = 254u;

Device::Device(connection_t conn) : m_conn(conn) {}

Device::~Device() {
    if (m_claim_future.valid()) {
        m_claim_future.wait();
    }
    if (m_conn != -1) {
        close(m_conn);
    }
}

Device::Device(Device &&other) noexcept
    : m_conn(std::exchange(other.m_conn, -1)), m_address(std::move(other.m_address)),
      m_claim_future(std::move(other.m_claim_future)) {}

Device &Device::operator=(Device &&other) noexcept {
    if (this != &other) {
        if (m_claim_future.valid()) {
            m_claim_future.wait();
        }
        if (m_conn != -1) {
            close(m_conn);
        }
        m_conn = std::exchange(other.m_conn, -1);
        m_address = std::move(other.m_address);
        m_claim_future = std::move(other.m_claim_future);
    }

    return *this;
}

static uint64_t pack_name(const DeviceName &name) {
    uint64_t n = 0;
    n |= static_cast<uint64_t>(name.unique_number) & 0x1FFFFF;
    n |= (static_cast<uint64_t>(static_cast<uint16_t>(name.manufacturer_code)) & 0x7FF) << 21;
    n |= (static_cast<uint64_t>(name.device_instance_lower) & 0x07) << 32;
    n |= (static_cast<uint64_t>(name.device_instance_upper) & 0x1F) << 35;
    n |= static_cast<uint64_t>(name.device_function.function) << 40;
    n |= (static_cast<uint64_t>(static_cast<uint8_t>(name.device_function.device_class)) & 0x7F)
         << 49;
    n |= (static_cast<uint64_t>(name.system_instance) & 0x0F) << 56;
    n |= (static_cast<uint64_t>(static_cast<uint8_t>(name.industry_group)) & 0x07) << 60;
    n |= static_cast<uint64_t>(name.arbitrary_address_capable) << 63;
    return n;
}

static std::expected<void, std::string> send_address_claim(int sockfd, uint8_t sa,
                                                           uint64_t packed_name) {
    can_frame frame{};
    frame.can_id = CAN_EFF_FLAG | (PGN_ADDRESS_CLAIM_PRIORITY << 26) |
                   (PGN_ADDRESS_CLAIM_PF << 16) | (DESTINATION_GLOBAL << 8) | sa;
    frame.can_dlc = 8;
    for (int i = 0; i < 8; ++i) {
        frame.data[i] = static_cast<uint8_t>(packed_name >> (i * 8));
    }
    if (::write(sockfd, &frame, sizeof(frame)) < 0) {
        return std::unexpected("Failed to send address claim frame");
    }
    return {};
}

static std::expected<bool, std::string> wait_for_response(int sockfd, uint8_t &address,
                                                          DeviceName name, uint64_t packed_name) {
    using namespace std::chrono;
    auto deadline = steady_clock::now() + milliseconds(250);
    while (true) {
        auto remaining = duration_cast<milliseconds>(deadline - steady_clock::now());
        if (remaining.count() <= 0) {
            break;
        }

        pollfd pfd{.fd = sockfd, .events = POLLIN, .revents = 0};
        if (::poll(&pfd, 1, static_cast<int>(remaining.count())) <= 0) {
            break;
        }

        can_frame frame{};
        if (::read(sockfd, &frame, sizeof(frame)) != sizeof(frame)) {
            continue;
        }

        if (((frame.can_id >> 16) & 0xFF) != PGN_ADDRESS_CLAIM_PF) {
            continue;
        }
        if ((frame.can_id & 0xFF) != address) {
            continue;
        }

        uint64_t res_name = 0;
        for (int i = 0; i < 8; ++i) {
            res_name |= static_cast<uint64_t>(frame.data[i]) << (i * 8);
        }

        if (res_name < packed_name) {
            if (name.arbitrary_address_capable) {
                address = static_cast<uint8_t>((address + 1) % 252);
                return true;
            } else {
                auto _ = send_address_claim(sockfd, NULL_ADDRESS, packed_name);
                return std::unexpected("Address conflict. Device not arbitrary address capable");
            }
        }
    }

    return false;
}

static std::expected<uint8_t, std::string> start_address_claim(int sockfd, DeviceName name) {
    auto packed_name = pack_name(name);
    uint8_t address = static_cast<uint8_t>(name.unique_number % 252);
    const uint8_t start_address = address;

    do {
        if (auto sent = send_address_claim(sockfd, address, packed_name); !sent) {
            return std::unexpected(sent.error());
        }
        auto response = wait_for_response(sockfd, address, name, packed_name);
        if (!response) {
            return std::unexpected(response.error());
        }
        if (!response.value()) {
            return address;
        }
    } while (address != start_address);

    return std::unexpected("No available addresses on the network");
}

std::shared_future<std::expected<void, std::string>> Device::claim(DeviceName name) {
    if (m_claim_future.valid() &&
        m_claim_future.wait_for(std::chrono::seconds(0)) != std::future_status::ready) {
        std::promise<std::expected<void, std::string>> p;
        p.set_value(std::unexpected<std::string>("Address claim already in progress"));
        return p.get_future().share();
    }
    m_claim_future =
        std::async(std::launch::async, [this, name]() -> std::expected<void, std::string> {
            auto result = start_address_claim(m_conn, name);
            if (!result) {
                return std::unexpected(result.error());
            }
            m_address = result.value();
            return {};
        }).share();
    return m_claim_future;
}

static std::expected<void, std::string> send_single_frame(int sockfd, uint32_t can_id,
                                                          const std::vector<uint8_t> &data) {
    can_frame frame{};
    frame.can_id = can_id;
    frame.can_dlc = static_cast<uint8_t>(data.size());
    std::copy(data.begin(), data.end(), frame.data);
    if (::write(sockfd, &frame, sizeof(frame)) < 0) {
        return std::unexpected("Failed to send message");
    }
    return {};
}

static std::expected<void, std::string> send_tp(int sockfd, uint8_t priority, uint8_t source,
                                                uint32_t pgn, const std::vector<uint8_t> &data) {
    const auto total_packets = static_cast<uint8_t>((data.size() + 6) / 7);

    can_frame bam{};
    bam.can_id = CAN_EFF_FLAG | (uint32_t(priority) << 26) | (0xECu << 16) | (0xFFu << 8) | source;
    bam.can_dlc = 8;
    bam.data[0] = 0x20;
    bam.data[1] = static_cast<uint8_t>(data.size());
    bam.data[2] = static_cast<uint8_t>(data.size() >> 8);
    bam.data[3] = total_packets;
    bam.data[4] = 0xFF;
    bam.data[5] = static_cast<uint8_t>(pgn);
    bam.data[6] = static_cast<uint8_t>(pgn >> 8);
    bam.data[7] = static_cast<uint8_t>(pgn >> 16);
    if (::write(sockfd, &bam, sizeof(bam)) < 0) {
        return std::unexpected("Failed to send TP BAM frame");
    }

    for (uint8_t seq = 1; seq <= total_packets; seq++) {
        can_frame dt{};
        dt.can_id =
            CAN_EFF_FLAG | (uint32_t(priority) << 26) | (0xEBu << 16) | (0xFFu << 8) | source;
        dt.can_dlc = 8;
        dt.data[0] = seq;
        const size_t offset = (seq - 1) * 7;
        for (int i = 0; i < 7; i++) {
            const size_t idx = offset + i;
            dt.data[i + 1] = idx < data.size() ? data[idx] : 0xFF;
        }
        if (::write(sockfd, &dt, sizeof(dt)) < 0) {
            return std::unexpected("Failed to send TP data frame");
        }
    }

    return {};
}

std::expected<void, std::string> Device::send(const NmeaMessage &msg, uint8_t priority) {
    if (!m_address) {
        return std::unexpected("Device has not claimed an address");
    }
    auto serialized = serialize(msg);
    const uint32_t can_id =
        CAN_EFF_FLAG | (uint32_t(priority) << 26) | (serialized.pgn << 8) | uint32_t(*m_address);
    if (serialized.data.size() <= 8) {
        return send_single_frame(m_conn, can_id, serialized.data);
    }
    return send_tp(m_conn, priority, *m_address, serialized.pgn, serialized.data);
}

std::expected<void, std::string> Device::send(const NmeaMessage &msg) {
    uint8_t priority = std::visit([](const auto &m) { return message::default_priority(m); }, msg);
    return send(msg, priority);
}

} // namespace nmea
