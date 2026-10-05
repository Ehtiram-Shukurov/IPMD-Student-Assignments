// protocol.cpp — Frame codec implementation.
#include "echosphere/protocol.hpp"

#include <cstring>
#include <random>

namespace echosphere {

namespace {

// Big-endian helpers.
void putU16(uint8_t* p, uint16_t v) {
    p[0] = static_cast<uint8_t>(v >> 8);
    p[1] = static_cast<uint8_t>(v & 0xff);
}

void putU32(uint8_t* p, uint32_t v) {
    p[0] = static_cast<uint8_t>(v >> 24);
    p[1] = static_cast<uint8_t>((v >> 16) & 0xff);
    p[2] = static_cast<uint8_t>((v >> 8) & 0xff);
    p[3] = static_cast<uint8_t>(v & 0xff);
}

uint16_t getU16(const uint8_t* p) {
    return static_cast<uint16_t>(p[0] << 8 | p[1]);
}

uint32_t getU32(const uint8_t* p) {
    return static_cast<uint32_t>(p[0]) << 24 | static_cast<uint32_t>(p[1]) << 16 |
           static_cast<uint32_t>(p[2]) << 8 | static_cast<uint32_t>(p[3]);
}

}  // namespace

// CRC-16/CCITT (poly 0x1021, init 0xFFFF).
uint16_t crc16(const uint8_t* data, std::size_t n) {
    uint16_t crc = 0xFFFF;
    for (std::size_t i = 0; i < n; ++i) {
        crc ^= static_cast<uint16_t>(data[i]) << 8;
        for (int b = 0; b < 8; ++b) {
            crc = (crc & 0x8000) ? static_cast<uint16_t>((crc << 1) ^ 0x1021)
                                 : static_cast<uint16_t>(crc << 1);
        }
    }
    return crc;
}

std::vector<uint8_t> encodeFrame(const Frame& frame) {
    if (frame.payload.size() > kMaxPayload) {
        return {};  // TODO: return an error instead of an empty frame.
    }
    std::vector<uint8_t> out(kHeaderSize + frame.payload.size() + kTrailerSize);
    uint8_t* p = out.data();
    std::memcpy(p, kMagic.data(), 4);
    p[4] = frame.header.version;
    p[5] = static_cast<uint8_t>(frame.header.type);
    putU16(p + 6, frame.header.flags);
    putU32(p + 8, static_cast<uint32_t>(frame.payload.size()));
    std::memcpy(p + kHeaderSize, frame.payload.data(), frame.payload.size());
    putU16(p + kHeaderSize + frame.payload.size(),
           crc16(out.data(), kHeaderSize + frame.payload.size()));
    return out;
}

std::size_t decodeFrame(const uint8_t* data, std::size_t n, Frame& out) {
    if (n < kHeaderSize + kTrailerSize) {
        return 0;  // incomplete
    }
    if (std::memcmp(data, kMagic.data(), 4) != 0) {
        return 0;  // TODO: resync strategy for a streaming decoder.
    }
    FrameHeader h;
    h.version = data[4];
    h.type = static_cast<MsgType>(data[5]);
    h.flags = getU16(data + 6);
    h.length = getU32(data + 8);
    if (h.version != kProtocolVersion || h.length > kMaxPayload) {
        return 0;
    }
    const std::size_t total = kHeaderSize + h.length + kTrailerSize;
    if (n < total) {
        return 0;  // incomplete
    }
    const uint16_t want = getU16(data + kHeaderSize + h.length);
    if (crc16(data, kHeaderSize + h.length) != want) {
        return 0;  // TODO: surface CRC errors distinctly from truncation.
    }
    out.header = h;
    out.payload.assign(data + kHeaderSize, data + kHeaderSize + h.length);
    return total;
}

}  // namespace echosphere
