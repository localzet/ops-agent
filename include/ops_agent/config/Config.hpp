#pragma once

#include <cstdint>
#include <string>

namespace ops_agent::config {

struct ServerConfig {
    std::string host{"127.0.0.1"};
    std::uint16_t port{8080};
    std::uint16_t threads{4};
};

struct DiagnosticsConfig {
    int tcp_timeout_ms{2000};
    int systemctl_timeout_ms{2000};
    std::size_t max_services_per_request{20};
};

struct LoggingConfig {
    std::string level{"info"};
};

struct StorageConfig {
    std::string sqlite_path{"/var/lib/ops-agent/ops-agent.db"};
};

struct Config {
    ServerConfig server{};
    DiagnosticsConfig diagnostics{};
    LoggingConfig logging{};
    StorageConfig storage{};
};

} // namespace ops_agent::config
