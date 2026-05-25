#pragma once

#include <cstdint>
#include <string>

namespace ops_agent::config {

struct ServerConfig {
    std::string host{"127.0.0.1"};
    std::uint16_t port{8080};
    std::uint16_t threads{4};
    std::uint8_t timeout_sec{5};
    std::size_t max_request_body_bytes{65536};
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

struct SecurityConfig {
    std::string api_key;
};

struct RateLimitConfig {
    bool enabled{true};
    double requests_per_second{10.0};
    std::size_t burst{20};
};

struct Config {
    ServerConfig server{};
    DiagnosticsConfig diagnostics{};
    LoggingConfig logging{};
    StorageConfig storage{};
    SecurityConfig security{};
    RateLimitConfig rate_limit{};
};

} // namespace ops_agent::config
