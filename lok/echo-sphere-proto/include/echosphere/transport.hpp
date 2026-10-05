// transport.hpp — Transport abstraction.
//
// The protocol layer talks only to ITransport. Everything socket-specific
// lives in the adapters below, so the protocol stays transport-agnostic
// and cloud-portable: a future TLS or cloud-queue adapter implements the
// same interface without touching the protocol.
#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

namespace echosphere {

class ITransport {
public:
    virtual ~ITransport() = default;

    // Open a connection. Returns true on success.
    virtual bool connect(const std::string& host, uint16_t port) = 0;

    // Send exactly n bytes (blocking, handles partial writes). Returns
    // bytes sent, or -1 on fatal error.
    virtual long send(const uint8_t* data, std::size_t n) = 0;

    // Read up to n bytes (blocking). Returns bytes read, 0 on orderly
    // shutdown, -1 on fatal error.
    virtual long recv(uint8_t* buf, std::size_t n) = 0;

    virtual void close() = 0;
    virtual bool isOpen() const = 0;
};

// Blocking TCP transport over POSIX sockets.
// TODO: read/write timeouts (setsockopt SO_RCVTIMEO/SO_SNDTIMEO).
class TcpTransport : public ITransport {
public:
    TcpTransport();
    ~TcpTransport() override;

    TcpTransport(const TcpTransport&) = delete;
    TcpTransport& operator=(const TcpTransport&) = delete;

    bool connect(const std::string& host, uint16_t port) override;
    long send(const uint8_t* data, std::size_t n) override;
    long recv(uint8_t* buf, std::size_t n) override;
    void close() override;
    bool isOpen() const override;

private:
    int fd_ = -1;
};

// TODO: TlsTransport (OpenSSL or mbedTLS) implementing ITransport,
// so security stays a transport-layer concern outside the protocol.

}  // namespace echosphere
