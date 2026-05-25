#pragma once

#include "ops_agent/common/Clock.hpp"
#include "ops_agent/config/Config.hpp"
#include "ops_agent/features/health/HealthService.hpp"
#include "ops_agent/features/metrics/MetricsService.hpp"
#include "ops_agent/features/services/ServicesService.hpp"
#include "ops_agent/features/system/SystemService.hpp"
#include "ops_agent/features/tcp_check/TcpCheckService.hpp"
#include "ops_agent/infrastructure/http/RequestLoggingMiddleware.hpp"
#include "ops_agent/infrastructure/http/SecurityMiddleware.hpp"

#include <crow.h>
#include <spdlog/spdlog.h>

#include <memory>

namespace ops_agent::app {

class Application {
public:
    using CrowApp = crow::App<infrastructure::http::RequestLoggingMiddleware,
                              infrastructure::http::RequestSecurityMiddleware,
                              infrastructure::http::ApiKeyAuthMiddleware,
                              infrastructure::http::RateLimitMiddleware,
                              infrastructure::http::SecurityHeadersMiddleware>;

    Application(config::Config config,
                std::shared_ptr<spdlog::logger> logger,
                std::shared_ptr<features::health::HealthService> health_service,
                std::shared_ptr<features::system::SystemService> system_service,
                std::shared_ptr<features::services::ServicesService> services_service,
                std::shared_ptr<features::tcp_check::TcpCheckService> tcp_check_service,
                std::shared_ptr<features::metrics::MetricsService> metrics_service);

    void run();
    void stop();

private:
    void registerRoutes();

    config::Config config_;
    std::shared_ptr<spdlog::logger> logger_;
    std::shared_ptr<features::health::HealthService> health_service_;
    std::shared_ptr<features::system::SystemService> system_service_;
    std::shared_ptr<features::services::ServicesService> services_service_;
    std::shared_ptr<features::tcp_check::TcpCheckService> tcp_check_service_;
    std::shared_ptr<features::metrics::MetricsService> metrics_service_;
    CrowApp app_;
};

} // namespace ops_agent::app
