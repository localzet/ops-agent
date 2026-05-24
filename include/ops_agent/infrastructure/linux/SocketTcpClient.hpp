#pragma once

#include "ops_agent/features/tcp_check/TcpClient.hpp"

namespace ops_agent::infrastructure::linux {

class SocketTcpClient final : public features::tcp_check::TcpClient {
public:
    features::tcp_check::TcpCheckDto check(const std::string& host,
                                           std::uint16_t port,
                                           int timeout_ms) const override;
};

} // namespace ops_agent::infrastructure::linux
