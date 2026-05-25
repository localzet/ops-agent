#include "ops_agent/app/Application.hpp"
#include "ops_agent/common/Clock.hpp"
#include "ops_agent/config/ConfigLoader.hpp"
#include "ops_agent/features/health/HealthService.hpp"
#include "ops_agent/features/metrics/MetricsService.hpp"
#include "ops_agent/features/services/ServicesService.hpp"
#include "ops_agent/features/system/SystemService.hpp"
#include "ops_agent/features/tcp_check/TcpCheckService.hpp"
#include "ops_agent/infrastructure/linux/HostInfoProvider.hpp"
#include "ops_agent/infrastructure/linux/ProcfsSystemProbe.hpp"
#include "ops_agent/infrastructure/linux/SocketTcpClient.hpp"
#include "ops_agent/infrastructure/linux/SystemdServiceManager.hpp"
#include "ops_agent/infrastructure/persistence/SQLiteConnection.hpp"

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <cstdlib>
#include <iostream>
#include <memory>
#include <optional>
#include <string>

namespace {

std::optional<std::string> configPath()
{
    if (const char* value = std::getenv("OPS_AGENT_CONFIG"); value != nullptr && *value != '\0') {
        return std::string{value};
    }
    return std::nullopt;
}

spdlog::level::level_enum parseLogLevel(const std::string& level)
{
    if (level == "trace") {
        return spdlog::level::trace;
    }
    if (level == "debug") {
        return spdlog::level::debug;
    }
    if (level == "warn") {
        return spdlog::level::warn;
    }
    if (level == "error") {
        return spdlog::level::err;
    }
    return spdlog::level::info;
}

void printHelp()
{
    std::cout << "ops-agent " << OPS_AGENT_VERSION << "\n"
              << "Environment:\n"
              << "  OPS_AGENT_CONFIG=/path/to/config.yml\n"
              << "  OPS_AGENT_HOST=127.0.0.1\n"
              << "  OPS_AGENT_PORT=8080\n";
}

} // namespace

int main(int argc, char** argv)
{
    if (argc > 1 && std::string{argv[1]} == "--help") {
        printHelp();
        return 0;
    }

    try {
        ops_agent::config::ConfigLoader loader;
        auto config = loader.load(configPath());

        auto logger = spdlog::stdout_color_mt("ops-agent");
        logger->set_level(parseLogLevel(config.logging.level));
        logger->set_pattern("%Y-%m-%dT%H:%M:%S.%eZ level=%l logger=%n %v");

        auto sqlite = std::make_shared<ops_agent::infrastructure::persistence::SQLiteConnection>(config.storage.sqlite_path);
        try {
            sqlite->initialize();
        } catch (const std::exception& error) {
            logger->warn("event=sqlite_init_failed error={}", error.what());
        }

        auto clock = std::make_shared<ops_agent::common::SystemClock>();
        auto host_info = std::make_shared<ops_agent::infrastructure::linux::HostInfoProvider>();
        auto system_probe = std::make_shared<ops_agent::infrastructure::linux::ProcfsSystemProbe>();
        auto service_manager = std::make_shared<ops_agent::infrastructure::linux::SystemdServiceManager>(
            config.diagnostics.systemctl_timeout_ms);
        auto tcp_client = std::make_shared<ops_agent::infrastructure::linux::SocketTcpClient>();
        auto metrics_service = std::make_shared<ops_agent::features::metrics::MetricsService>();

        auto health_service = std::make_shared<ops_agent::features::health::HealthService>(clock, host_info);
        auto system_service = std::make_shared<ops_agent::features::system::SystemService>(system_probe);
        auto services_service = std::make_shared<ops_agent::features::services::ServicesService>(service_manager, config.diagnostics);
        auto tcp_check_service = std::make_shared<ops_agent::features::tcp_check::TcpCheckService>(tcp_client, config.diagnostics);

        ops_agent::app::Application app{
            config,
            logger,
            health_service,
            system_service,
            services_service,
            tcp_check_service,
            metrics_service,
        };
        app.run();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "fatal: " << error.what() << '\n';
        return 1;
    }
}
