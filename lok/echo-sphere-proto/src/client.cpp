// client.cpp — Device client skeleton.
//
// Framing, transport, and session primitives are real; the connect/read
// loops below are skeletons with TODOs marking where design decisions
// (redelivery timers, half-open detection) still need to land.
#include "echosphere/client.hpp"

#include <random>

namespace echosphere {

std::chrono::milliseconds nextBackoff(const BackoffPolicy& policy, int attempt) {
    double delay = static_cast<double>(policy.base.count());
    for (int i = 0; i < attempt; ++i) {
        delay *= 2.0;
        if (delay >= policy.cap.count()) {
            delay = static_cast<double>(policy.cap.count());
            break;
        }
    }
    thread_local std::mt19937 rng{std::random_device{}()};
    std::uniform_real_distribution<double> d(1.0 - policy.jitter, 1.0 + policy.jitter);
    return std::chrono::milliseconds(static_cast<long>(delay * d(rng)));
}

EchoClient::EchoClient(std::string client_id, std::unique_ptr<ITransport> transport)
    : client_id_(std::move(client_id)), transport_(std::move(transport)) {}

EchoClient::~EchoClient() { stop(); }

bool EchoClient::start(const std::string& host, uint16_t port) {
    if (running_) return true;
    running_ = true;
    worker_ = std::thread([this, host, port] { connectLoop(host, port); });
    return true;
}

void EchoClient::stop() {
    running_ = false;
    transport_->close();  // unblocks recv in the worker
    if (worker_.joinable()) worker_.join();
}

bool EchoClient::publish(const std::string& topic, const std::vector<uint8_t>& data,
                         QoS qos, std::chrono::milliseconds ack_timeout) {
    // TODO: implement — build PUBLISH via makePublish, assign msg id from
    // session_.nextMsgId(), track in session_, send, and wait for PUBACK
    // (or return immediately and let redelivery handle loss).
    (void)topic;
    (void)data;
    (void)qos;
    (void)ack_timeout;
    return false;
}

void EchoClient::onMessage(MessageHandler handler) { handler_ = std::move(handler); }

void EchoClient::connectLoop(const std::string& host, uint16_t port) {
    int attempt = 0;
    while (running_) {
        if (transport_->connect(host, port)) {
            // TODO: send CONNECT, await CONNACK, check session_present,
            // then readLoop(). On clean_session=false resend session
            // pending publishes with the duplicate flag.
            attempt = 0;
            readLoop();
            transport_->close();
        }
        std::this_thread::sleep_for(nextBackoff(backoff_, attempt++));
    }
}

void EchoClient::readLoop() {
    // TODO: read frames via decodeFrame into a growing buffer (partial
    // reads), dispatch: PUBLISH -> handler_ (+ PUBACK if QoS 1),
    // PUBACK -> session_.ackPending, PINGRESP -> reset keepalive timer,
    // CONNACK -> complete handshake.
}

bool EchoClient::sendFrame(const Frame& frame) {
    std::lock_guard<std::mutex> lock(send_mutex_);
    auto bytes = encodeFrame(frame);
    return transport_->send(bytes.data(), bytes.size()) ==
           static_cast<long>(bytes.size());
}

}  // namespace echosphere
