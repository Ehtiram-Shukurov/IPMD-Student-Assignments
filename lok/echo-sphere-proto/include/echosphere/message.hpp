// message.hpp — Payload codecs for each message type.
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "echosphere/protocol.hpp"

namespace echosphere {

// --- CONNECT payload ---
struct ConnectPayload {
    std::string client_id;
    uint16_t keepalive_sec = 60;
    bool clean_session = true;  // false => ask server to resume prior session
};

Frame makeConnect(const ConnectPayload& p);
bool parseConnect(const Frame& f, ConnectPayload& out);

// --- CONNACK payload ---
struct ConnackPayload {
    bool session_present = false;  // true if the server resumed a session
    uint8_t return_code = 0;       // 0 = accepted
};

Frame makeConnack(const ConnackPayload& p);
bool parseConnack(const Frame& f, ConnackPayload& out);

// --- PUBLISH payload ---
struct PublishPayload {
    std::string topic;
    uint16_t msg_id = 0;  // meaningful only for QoS 1
    QoS qos = QoS::AtMostOnce;
    std::vector<uint8_t> data;
};

Frame makePublish(const PublishPayload& p);
bool parsePublish(const Frame& f, PublishPayload& out);

// --- PUBACK payload: just the message id being acknowledged ---
Frame makePuback(uint16_t msg_id);
bool parsePuback(const Frame& f, uint16_t& msg_id);

// --- PINGREQ / PINGRESP / DISCONNECT carry no payload ---
Frame makePingreq();
Frame makePingresp();
Frame makeDisconnect();

// --- SUBSCRIBE / SUBACK (topic filter, QoS granted) ---
struct SubscribePayload {
    std::string topic_filter;
};

struct SubackPayload {
    QoS granted_qos = QoS::AtMostOnce;
};

Frame makeSubscribe(const SubscribePayload& p);
bool parseSubscribe(const Frame& f, SubscribePayload& out);
Frame makeSuback(const SubackPayload& p);
bool parseSuback(const Frame& f, SubackPayload& out);

}  // namespace echosphere
