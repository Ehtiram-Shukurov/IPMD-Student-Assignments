// server.cpp — Reference server skeleton.
#include "echosphere/server.hpp"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

namespace echosphere {

EchoServer::EchoServer(uint16_t port) : port_(port) {}

EchoServer::~EchoServer() { stop(); }

bool EchoServer::start() {
    listen_fd_ = ::socket(AF_INET6, SOCK_STREAM, 0);
    if (listen_fd_ < 0) return false;
    int off = 0;
    setsockopt(listen_fd_, IPPROTO_IPV6, IPV6_V6ONLY, &off, sizeof(off));
    int on = 1;
    setsockopt(listen_fd_, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
    sockaddr_in6 addr{};
    addr.sin6_family = AF_INET6;
    addr.sin6_port = htons(port_);
    if (::bind(listen_fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0 ||
        ::listen(listen_fd_, 16) != 0) {
        ::close(listen_fd_);
        listen_fd_ = -1;
        return false;
    }
    running_ = true;
    accept_thread_ = std::thread([this] { acceptLoop(); });
    return true;
}

void EchoServer::stop() {
    running_ = false;
    if (listen_fd_ >= 0) {
        // Shutdown unblocks accept().
        ::shutdown(listen_fd_, SHUT_RDWR);
        ::close(listen_fd_);
        listen_fd_ = -1;
    }
    if (accept_thread_.joinable()) accept_thread_.join();
}

std::size_t EchoServer::connectionCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return sessions_.size();
}

void EchoServer::acceptLoop() {
    while (running_) {
        int fd = ::accept(listen_fd_, nullptr, nullptr);
        if (fd < 0) break;  // stop() shut us down
        // TODO: spawn a detached thread calling handleConnection(fd, peer).
        ::close(fd);  // placeholder until handleConnection is wired up
    }
}

void EchoServer::handleConnection(int fd, std::string peer) {
    // TODO: per-connection loop —
    //   1. Read CONNECT, look up/create Session by client_id
    //      (clean_session=false => resume, reply session_present=1).
    //   2. PUBLISH (QoS 1): if !session.alreadySeen(msg_id) deliver, then PUBACK.
    //   3. Track keepalive; close on missed PINGREQ deadline.
    //   4. On disconnect keep the session for clean_session=false clients.
    (void)peer;
    ::close(fd);
}

}  // namespace echosphere
