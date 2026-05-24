#pragma once

#include "ops_agent/features/tcp_check/TcpCheckDto.hpp"

#include <cstdint>
#include <string>

namespace ops_agent::features::tcp_check {

class TcpClient {
public:
    virtual ~TcpClient() = default;
    virtual TcpCheckDto check(const std::string& host, std::uint16_t port, int timeout_ms) const = 0;
};

} // namespace ops_agent::features::tcp_check
