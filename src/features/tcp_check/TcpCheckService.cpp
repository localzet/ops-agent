#include "ops_agent/features/tcp_check/TcpCheckService.hpp"

#include <algorithm>
#include <stdexcept>

namespace ops_agent::features::tcp_check {

TcpCheckService::TcpCheckService(std::shared_ptr<TcpClient> tcp_client,
                                 config::DiagnosticsConfig diagnostics_config)
    : tcp_client_(std::move(tcp_client)),
      diagnostics_config_(diagnostics_config)
{
}

TcpCheckDto TcpCheckService::check(const std::string& host, const std::string& port_value) const
{
    if (host.empty()) {
        throw std::invalid_argument("query parameter 'host' is required");
    }
    if (host.size() > 253) {
        throw std::invalid_argument("host is too long");
    }
    if (port_value.empty()) {
        throw std::invalid_argument("query parameter 'port' is required");
    }

    const auto parsed_port = std::stoul(port_value);
    if (parsed_port == 0 || parsed_port > 65535) {
        throw std::invalid_argument("port must be in range 1..65535");
    }

    const int timeout_ms = std::clamp(diagnostics_config_.tcp_timeout_ms, 1, 5000);
    return tcp_client_->check(host, static_cast<std::uint16_t>(parsed_port), timeout_ms);
}

} // namespace ops_agent::features::tcp_check
