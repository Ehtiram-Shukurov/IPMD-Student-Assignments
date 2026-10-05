// protocol.hpp — EchoSphere wire format (v0).
//
// Frame layout on the wire (all multi-byte fields big-endian):
//   magic[4]   'E','S','P','1'
//   version    1 byte   (protocol version)
//   type       1 byte   (MsgType)
//   flags      2 bytes  (bit 0: QoS1 redelivery-required ...)
//   length     4 bytes  (payload length in bytes, 0 .. 16 MiB)
//   payload[length]
//   crc16      2 bytes  (CRC-16/CCITT over everything above)
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace echosphere {

inline constexpr std::array<uint8_t, 4> kMagic = {'E', 'S', 'P', '1'};
inline constexpr uint8_t kProtocolVersion = 1;
inline constexpr std::size_t kHeaderSize = 12;      // magic + ver + type + flags + length
inline constexpr std::size_t kTrailerSize = 2;      // crc16
inline constexpr uint32_t kMaxPayload = 16u * 1024u * 1024u;

enum class MsgType : uint8_t {
    CONNECT = 1,
    CONNACK = 2,
    PUBLISH = 3,
    PUBACK = 4,
    PINGREQ = 5,
    PINGRESP = 6,
    DISCONNECT = 7,
    SUBSCRIBE = 8,
    SUBACK = 9,
};

enum class QoS : uint8_t {
    AtMostOnce = 0,
    AtLeastOnce = 1,
    // QoS 2 (exactly-once) deliberately omitted: the four-way handshake
    // buys little for sensor telemetry; see design doc §4.
};

struct FrameHeader {
    uint8_t version = kProtocolVersion;
    MsgType type = MsgType::PINGREQ;
    uint16_t flags = 0;
    uint32_t length = 0;
};

struct Frame {
    FrameHeader header;
    std::vector<uint8_t> payload;
};

uint16_t crc16(const uint8_t* data, std::size_t n);

// Encode a complete frame (header + payload + crc trailer).
std::vector<uint8_t> encodeFrame(const Frame& frame);

// Decode one complete frame from `data` (must contain the full frame).
// Returns bytes consumed, or 0 if the buffer holds an incomplete frame.
std::size_t decodeFrame(const uint8_t* data, std::size_t n, Frame& out);

// Flag helpers.
inline constexpr uint16_t kFlagQoS1 = 1u << 0;
inline constexpr uint16_t kFlagDuplicate = 1u << 1;  // redelivery marker

}  // namespace echosphere
