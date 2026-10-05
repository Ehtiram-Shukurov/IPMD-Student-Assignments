// session.cpp — Session state implementation.
#include "echosphere/session.hpp"

namespace echosphere {

uint16_t Session::nextMsgId() {
    uint16_t id = next_id_;
    next_id_ = (next_id_ == 0xFFFF) ? 1 : static_cast<uint16_t>(next_id_ + 1);
    return id;
}

void Session::trackPending(uint16_t msg_id, Frame frame) {
    pending_[msg_id] = PendingPublish{std::move(frame),
                                      std::chrono::steady_clock::now(), 1};
}

bool Session::ackPending(uint16_t msg_id) {
    return pending_.erase(msg_id) > 0;
}

std::vector<uint16_t> Session::overduePending(std::chrono::milliseconds timeout) const {
    std::vector<uint16_t> out;
    const auto now = std::chrono::steady_clock::now();
    for (const auto& [id, p] : pending_) {
        if (now - p.sent_at > timeout) out.push_back(id);
    }
    return out;
}

bool Session::alreadySeen(uint16_t msg_id) {
    if (seen_.count(msg_id)) return true;
    seen_.insert(msg_id);
    // TODO: bound the window; evict oldest when it exceeds the cap.
    return false;
}

void Session::retain(const std::string& topic, std::vector<uint8_t> payload) {
    retained_[topic] = std::move(payload);
}

const std::vector<uint8_t>* Session::retained(const std::string& topic) const {
    auto it = retained_.find(topic);
    return it == retained_.end() ? nullptr : &it->second;
}

void Session::reset() {
    next_id_ = 1;
    pending_.clear();
    seen_.clear();
    if (clean_) retained_.clear();
}

}  // namespace echosphere
