// echo_server — reference server entry point.
#include <cstdio>

#include "echosphere/server.hpp"

int main(int argc, char** argv) {
    uint16_t port = 1883;
    if (argc > 1) port = static_cast<uint16_t>(std::atoi(argv[1]));

    echosphere::EchoServer server(port);
    if (!server.start()) {
        std::fprintf(stderr, "failed to start server on port %u\n", port);
        return 1;
    }
    std::printf("EchoSphere reference server listening on port %u (Ctrl-C to stop)\n",
                port);
    // TODO: install a SIGINT/SIGTERM handler calling server.stop().
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(60));
    }
    return 0;
}
