#pragma once

#include <cstdint>
#include <string>

namespace ops_agent::features::tcp_check {

struct TcpCheckDto {
    std::string host;
    std::uint16_t port{0};
    bool reachable{false};
    int latency_ms{0};
    std::string error;
};

} // namespace ops_agent::features::tcp_check
