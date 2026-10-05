// echo_client — device client entry point.
#include <cstdio>
#include <string>

#include "echosphere/client.hpp"
#include "echosphere/transport.hpp"

int main(int argc, char** argv) {
    std::string host = "127.0.0.1";
    uint16_t port = 1883;
    if (argc > 1) host = argv[1];
    if (argc > 2) port = static_cast<uint16_t>(std::atoi(argv[2]));

    echosphere::EchoClient client("robot-001",
                                  std::make_unique<echosphere::TcpTransport>());
    client.onMessage([](const echosphere::PublishPayload& msg) {
        std::printf("received on %s (%zu bytes)\n", msg.topic.c_str(),
                    msg.data.size());
    });

    if (!client.start(host, port)) {
        std::fprintf(stderr, "failed to start client\n");
        return 1;
    }
    std::printf("client connecting to %s:%u (Ctrl-C to stop)\n", host.c_str(),
                port);

    // Demo publish loop.
    int n = 0;
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(5));
        std::string payload = "telemetry #" + std::to_string(n++);
        client.publish("robot/telemetry",
                       std::vector<uint8_t>(payload.begin(), payload.end()),
                       echosphere::QoS::AtLeastOnce);
    }
    return 0;
}
