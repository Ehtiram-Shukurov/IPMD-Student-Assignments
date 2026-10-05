// session.hpp — Explicit, bounded session state.
//
// The protocol is stateless on the wire; everything the server remembers
// lives here and is deliberately small: message IDs, unacknowledged
// QoS-1 publishes (for redelivery), recently seen IDs (for dedup), and
// the retained store. A persistent session survives reconnect; a clean
// session resets.
#pragma once

#include <chrono>
#include <cstdint>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "echosphere/protocol.hpp"

namespace echosphere {

struct PendingPublish {
    Frame frame;                                    // PUBLISH awaiting PUBACK
    std::chrono::steady_clock::time_point sent_at;  // for redelivery timeout
    int attempts = 1;
};

class Session {
public:
    explicit Session(bool clean = true) : clean_(clean) {}

    // Allocate the next message ID (QoS-1 publishes). Wraps at 65535,
    // skipping 0 (reserved).
    uint16_t nextMsgId();

    // Record a QoS-1 publish as unacknowledged.
    void trackPending(uint16_t msg_id, Frame frame);

    // Mark a publish acknowledged; returns true if it was pending.
    bool ackPending(uint16_t msg_id);

    // Publishes whose ACK is overdue (caller resends them with the
    // duplicate flag set).
    std::vector<uint16_t> overduePending(std::chrono::milliseconds timeout) const;

    // Deduplication: true if this message ID was already delivered.
    // Keeps only a bounded window of recent IDs.
    bool alreadySeen(uint16_t msg_id);

    // Retained store: last PUBLISH per topic, served to new subscribers.
    void retain(const std::string& topic, std::vector<uint8_t> payload);
    const std::vector<uint8_t>* retained(const std::string& topic) const;

    // Reset all state (clean session).
    void reset();

    bool isClean() const { return clean_; }

private:
    bool clean_;
    uint16_t next_id_ = 1;
    std::map<uint16_t, PendingPublish> pending_;
    std::set<uint16_t> seen_;  // TODO: bound this window (e.g. last 1024 IDs)
    std::map<std::string, std::vector<uint8_t>> retained_;
    // TODO: disk-backed store so sessions survive server restart.
};

}  // namespace echosphere
