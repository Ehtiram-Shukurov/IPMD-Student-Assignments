// client.hpp — Device-side client.
//
// Connects, keeps the connection alive, and publishes with at-least-once
// semantics. On failure it reconnects with exponential backoff + jitter
// and resumes the session (re-sending unacknowledged QoS-1 publishes with
// the duplicate flag).
#pragma once

#include <atomic>
#include <cstdint>
#include <functional>
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

using MessageHandler = std::function<void(const PublishPayload&)>;

// Backoff policy: delay = min(cap, base * 2^attempt) with ±jitter.
struct BackoffPolicy {
    std::chrono::milliseconds base{1000};
    std::chrono::milliseconds cap{60000};
    double jitter = 0.25;  // ±25%
};

std::chrono::milliseconds nextBackoff(const BackoffPolicy& policy, int attempt);

class EchoClient {
public:
    EchoClient(std::string client_id, std::unique_ptr<ITransport> transport);
    ~EchoClient();

    EchoClient(const EchoClient&) = delete;
    EchoClient& operator=(const EchoClient&) = delete;

    // Block until connected (retries forever with backoff) or stop() is called.
    bool start(const std::string& host, uint16_t port);

    void stop();

    // Publish a message. QoS 1 blocks until PUBACK or timeout; on timeout
    // the message stays tracked and is redelivered after reconnect.
    bool publish(const std::string& topic, const std::vector<uint8_t>& data,
                 QoS qos = QoS::AtLeastOnce,
                 std::chrono::milliseconds ack_timeout = std::chrono::milliseconds(5000));

    void onMessage(MessageHandler handler);

private:
    void connectLoop(const std::string& host, uint16_t port);
    void readLoop();
    bool sendFrame(const Frame& frame);

    std::string client_id_;
    std::unique_ptr<ITransport> transport_;
    Session session_{false};  // persistent session: resume on reconnect
    BackoffPolicy backoff_;
    MessageHandler handler_;
    std::atomic<bool> running_{false};
    std::thread worker_;
    std::mutex send_mutex_;
    // TODO: redelivery timer thread scanning session_.overduePending().
};

}  // namespace echosphere
