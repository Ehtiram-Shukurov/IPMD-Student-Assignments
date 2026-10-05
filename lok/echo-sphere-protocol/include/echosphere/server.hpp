// server.hpp — Reference server.
//
// Accepts TCP connections, runs one session per client, and implements
// the server side of at-least-once delivery: PUBACK on PUBLISH, dedup of
// redelivered messages, retained store, keepalive tracking.
//
// This is a reference implementation for the prototype, not a production
// broker: one thread per connection, in-memory state.
#pragma once

#include <atomic>
#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "echosphere/message.hpp"
#include "echosphere/protocol.hpp"
#include "echosphere/session.hpp"
#include "echosphere/transport.hpp"

namespace echosphere {

class EchoServer {
public:
    explicit EchoServer(uint16_t port);
    ~EchoServer();

    EchoServer(const EchoServer&) = delete;
    EchoServer& operator=(const EchoServer&) = delete;

    // Start the accept loop (returns immediately; use stop() to shut down).
    bool start();
    void stop();

    // Number of currently connected clients.
    std::size_t connectionCount() const;

private:
    void acceptLoop();
    void handleConnection(int fd, std::string peer);

    uint16_t port_;
    int listen_fd_ = -1;
    std::atomic<bool> running_{false};
    std::thread accept_thread_;

    mutable std::mutex mutex_;
    std::map<std::string, std::unique_ptr<Session>> sessions_;  // by client_id
    // TODO: retained store shared across sessions lives on Session today;
    // move to a broker-level store when pub/sub routing is added.
};

}  // namespace echosphere
