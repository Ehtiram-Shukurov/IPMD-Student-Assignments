# EchoSphere Protocol Prototype (v0 skeleton)

C++17 socket-level prototype of the EchoSphere device-to-cloud messaging
protocol. Implements the framing layer, a transport abstraction, session
state for at-least-once delivery, and a reconnecting device client plus a
reference server.

## Layout

```
include/echosphere/
  protocol.hpp   Wire format: frame header, encode/decode, CRC-16
  message.hpp    Payload codecs: CONNECT, PUBLISH, PUBACK, ...
  transport.hpp  ITransport interface (transport-agnostic) + TcpTransport
  session.hpp    Session state: message IDs, pending ACKs, dedup, retained store
  client.hpp     EchoClient: connect loop, jittered backoff, keepalive
  server.hpp     EchoServer: accept loop, session dispatch
src/             Implementations (several marked TODO)
examples/        echo_server, echo_client
```

## Build

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
./build/echo_server 1883 &
./build/echo_client 127.0.0.1 1883
```

## What's real vs. stub

- Real: frame encode/decode, CRC-16/CCITT, TCP transport, backoff policy.
- TODO: streaming decode across partial reads, persistent (disk-backed)
  session store, TLS transport adapter, full redelivery timers.

## Design constraints (from the design doc)

- Protocol is transport-agnostic: everything above `ITransport` never
  touches sockets directly.
- Stateless where possible: the wire carries what the server needs;
  session state is explicit and bounded.
- Cloud-portable: no Linux-only or R630-only dependencies in the
  protocol layer; the TCP transport is the only OS-touching adapter.
