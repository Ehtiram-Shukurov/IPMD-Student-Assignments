cmake_minimum_required(VERSION 3.16)
project(echo_sphere_proto VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Library: framing codec + session state + client/server cores.
add_library(echosphere STATIC
    src/protocol.cpp
    src/message.cpp
    src/tcp_transport.cpp
    src/session.cpp
    src/client.cpp
    src/server.cpp
)
target_include_directories(echosphere PUBLIC include)
target_compile_options(echosphere PRIVATE -Wall -Wextra -Wpedantic)

# Examples: minimal device client and server talking over the wire.
add_executable(echo_server examples/echo_server.cpp)
target_link_libraries(echo_server PRIVATE echosphere)

add_executable(echo_client examples/echo_client.cpp)
target_link_libraries(echo_client PRIVATE echosphere)
