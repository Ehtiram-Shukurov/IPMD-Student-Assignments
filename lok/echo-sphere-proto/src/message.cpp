// message.cpp — Payload codecs.
#include "echosphere/message.hpp"

#include <cstring>

namespace echosphere {

namespace {

// Length-prefixed string: u16 length + bytes.
void putStr(std::vector<uint8_t>& out, const std::string& s) {
    uint16_t len = static_cast<uint16_t>(s.size());
    out.push_back(static_cast<uint8_t>(len >> 8));
    out.push_back(static_cast<uint8_t>(len & 0xff));
    out.insert(out.end(), s.begin(), s.end());
}

bool getStr(const uint8_t*& p, const uint8_t* end, std::string& s) {
    if (end - p < 2) return false;
    uint16_t len = static_cast<uint16_t>(p[0] << 8 | p[1]);
    p += 2;
    if (end - p < len) return false;
    s.assign(reinterpret_cast<const char*>(p), len);
    p += len;
    return true;
}

Frame makeFrame(MsgType type, std::vector<uint8_t> payload, uint16_t flags = 0) {
    Frame f;
    f.header.type = type;
    f.header.flags = flags;
    f.payload = std::move(payload);
    return f;
}

}  // namespace

Frame makeConnect(const ConnectPayload& p) {
    std::vector<uint8_t> b;
    putStr(b, p.client_id);
    b.push_back(static_cast<uint8_t>(p.keepalive_sec >> 8));
    b.push_back(static_cast<uint8_t>(p.keepalive_sec & 0xff));
    b.push_back(p.clean_session ? 1 : 0);
    return makeFrame(MsgType::CONNECT, std::move(b));
}

bool parseConnect(const Frame& f, ConnectPayload& out) {
    const uint8_t* p = f.payload.data();
    const uint8_t* end = p + f.payload.size();
    if (!getStr(p, end, out.client_id) || end - p < 3) return false;
    out.keepalive_sec = static_cast<uint16_t>(p[0] << 8 | p[1]);
    out.clean_session = p[2] != 0;
    return true;
}

Frame makeConnack(const ConnackPayload& p) {
    std::vector<uint8_t> b = {static_cast<uint8_t>(p.session_present ? 1 : 0),
                              p.return_code};
    return makeFrame(MsgType::CONNACK, std::move(b));
}

bool parseConnack(const Frame& f, ConnackPayload& out) {
    if (f.payload.size() != 2) return false;
    out.session_present = f.payload[0] != 0;
    out.return_code = f.payload[1];
    return true;
}

Frame makePublish(const PublishPayload& p) {
    std::vector<uint8_t> b;
    putStr(b, p.topic);
    const bool qos1 = p.qos == QoS::AtLeastOnce;
    if (qos1) {
        b.push_back(static_cast<uint8_t>(p.msg_id >> 8));
        b.push_back(static_cast<uint8_t>(p.msg_id & 0xff));
    }
    b.insert(b.end(), p.data.begin(), p.data.end());
    return makeFrame(MsgType::PUBLISH, std::move(b),
                     qos1 ? kFlagQoS1 : 0);
}

bool parsePublish(const Frame& f, PublishPayload& out) {
    const uint8_t* p = f.payload.data();
    const uint8_t* end = p + f.payload.size();
    if (!getStr(p, end, out.topic)) return false;
    out.qos = (f.header.flags & kFlagQoS1) ? QoS::AtLeastOnce : QoS::AtMostOnce;
    if (out.qos == QoS::AtLeastOnce) {
        if (end - p < 2) return false;
        out.msg_id = static_cast<uint16_t>(p[0] << 8 | p[1]);
        p += 2;
    }
    out.data.assign(p, end);
    return true;
}

Frame makePuback(uint16_t msg_id) {
    std::vector<uint8_t> b = {static_cast<uint8_t>(msg_id >> 8),
                              static_cast<uint8_t>(msg_id & 0xff)};
    return makeFrame(MsgType::PUBACK, std::move(b));
}

bool parsePuback(const Frame& f, uint16_t& msg_id) {
    if (f.payload.size() != 2) return false;
    msg_id = static_cast<uint16_t>(f.payload[0] << 8 | f.payload[1]);
    return true;
}

Frame makePingreq() { return makeFrame(MsgType::PINGREQ, {}); }
Frame makePingresp() { return makeFrame(MsgType::PINGRESP, {}); }
Frame makeDisconnect() { return makeFrame(MsgType::DISCONNECT, {}); }

Frame makeSubscribe(const SubscribePayload& p) {
    std::vector<uint8_t> b;
    putStr(b, p.topic_filter);
    return makeFrame(MsgType::SUBSCRIBE, std::move(b));
}

bool parseSubscribe(const Frame& f, SubscribePayload& out) {
    const uint8_t* p = f.payload.data();
    const uint8_t* end = p + f.payload.size();
    return getStr(p, end, out.topic_filter);
}

Frame makeSuback(const SubackPayload& p) {
    std::vector<uint8_t> b = {static_cast<uint8_t>(p.granted_qos)};
    return makeFrame(MsgType::SUBACK, std::move(b));
}

bool parseSuback(const Frame& f, SubackPayload& out) {
    if (f.payload.size() != 1) return false;
    out.granted_qos = static_cast<QoS>(f.payload[0]);
    return true;
}

}  // namespace echosphere
