// tcp_transport.cpp — Blocking POSIX TCP transport.
#include "echosphere/transport.hpp"

#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>

namespace echosphere {

TcpTransport::TcpTransport() = default;

TcpTransport::~TcpTransport() { close(); }

bool TcpTransport::connect(const std::string& host, uint16_t port) {
    addrinfo hints{};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    addrinfo* list = nullptr;
    if (getaddrinfo(host.c_str(), std::to_string(port).c_str(), &hints, &list) != 0) {
        return false;
    }
    int fd = -1;
    for (addrinfo* ai = list; ai != nullptr; ai = ai->ai_next) {
        fd = ::socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
        if (fd < 0) continue;
        if (::connect(fd, ai->ai_addr, ai->ai_addrlen) == 0) break;
        ::close(fd);
        fd = -1;
    }
    freeaddrinfo(list);
    if (fd < 0) return false;
    fd_ = fd;
    return true;
}

long TcpTransport::send(const uint8_t* data, std::size_t n) {
    std::size_t sent = 0;
    while (sent < n) {
        ssize_t r = ::send(fd_, data + sent, n - sent, MSG_NOSIGNAL);
        if (r <= 0) return -1;
        sent += static_cast<std::size_t>(r);
    }
    return static_cast<long>(sent);
}

long TcpTransport::recv(uint8_t* buf, std::size_t n) {
    ssize_t r = ::recv(fd_, buf, n, 0);
    if (r < 0) return -1;
    return static_cast<long>(r);  // 0 = orderly shutdown
}

void TcpTransport::close() {
    if (fd_ >= 0) {
        ::close(fd_);
        fd_ = -1;
    }
}

bool TcpTransport::isOpen() const { return fd_ >= 0; }

}  // namespace echosphere
