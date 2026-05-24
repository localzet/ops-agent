#pragma once

#include "ops_agent/config/Config.hpp"
#include "ops_agent/features/tcp_check/TcpCheckDto.hpp"
#include "ops_agent/features/tcp_check/TcpClient.hpp"

#include <memory>
#include <string>

namespace ops_agent::features::tcp_check {

class TcpCheckService {
public:
    TcpCheckService(std::shared_ptr<TcpClient> tcp_client,
                    config::DiagnosticsConfig diagnostics_config);

    TcpCheckDto check(const std::string& host, const std::string& port_value) const;

private:
    std::shared_ptr<TcpClient> tcp_client_;
    config::DiagnosticsConfig diagnostics_config_;
};

} // namespace ops_agent::features::tcp_check
