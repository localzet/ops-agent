#include "ops_agent/app/Application.hpp"

#include "ops_agent/infrastructure/http/HttpResponse.hpp"

#include <exception>
#include <csignal>
#include <stdexcept>
#include <string>

namespace ops_agent::app {
namespace {

template <typename Fn>
crow::response safely(Fn&& fn)
{
    try {
        return fn();
    } catch (const std::invalid_argument& error) {
        return infrastructure::http::errorResponse(error.what(), 400);
    } catch (const std::exception&) {
        return infrastructure::http::errorResponse("internal server error", 500);
    }
}

std::string queryValue(const crow::request& request, const char* name)
{
    const char* value = request.url_params.get(name);
    return value != nullptr ? std::string{value} : std::string{};
}

} // namespace

Application::Application(config::Config config,
                         std::shared_ptr<spdlog::logger> logger,
                         std::shared_ptr<features::health::HealthService> health_service,
                         std::shared_ptr<features::system::SystemService> system_service,
                         std::shared_ptr<features::services::ServicesService> services_service,
                         std::shared_ptr<features::tcp_check::TcpCheckService> tcp_check_service,
                         std::shared_ptr<features::metrics::MetricsService> metrics_service)
    : config_(std::move(config)),
      logger_(std::move(logger)),
      health_service_(std::move(health_service)),
      system_service_(std::move(system_service)),
      services_service_(std::move(services_service)),
      tcp_check_service_(std::move(tcp_check_service)),
      metrics_service_(std::move(metrics_service))
{
    app_.get_middleware<infrastructure::http::RequestLoggingMiddleware>().setLogger(logger_);
    app_.get_middleware<infrastructure::http::RequestLoggingMiddleware>().setMetrics(metrics_service_);
    app_.get_middleware<infrastructure::http::RequestSecurityMiddleware>().setMaxRequestBodyBytes(
        config_.server.max_request_body_bytes);
    app_.get_middleware<infrastructure::http::ApiKeyAuthMiddleware>().setApiKey(config_.security.api_key);
    app_.get_middleware<infrastructure::http::RateLimitMiddleware>().configure(config_.rate_limit);
    registerRoutes();
}

void Application::run()
{
    logger_->info("{{\"event\":\"start\",\"host\":\"{}\",\"port\":{},\"threads\":{},\"auth_enabled\":{}}}",
                  config_.server.host,
                  config_.server.port,
                  config_.server.threads,
                  config_.security.api_key.empty() ? "false" : "true");
    app_.server_name("")
        .bindaddr(config_.server.host)
        .port(config_.server.port)
        .concurrency(config_.server.threads)
        .timeout(config_.server.timeout_sec)
        .signal_clear()
        .signal_add(SIGINT)
        .signal_add(SIGTERM)
        .run();
}

void Application::stop()
{
    logger_->info("{{\"event\":\"shutdown\"}}");
    app_.stop();
}

void Application::registerRoutes()
{
    CROW_ROUTE(app_, "/health").methods(crow::HTTPMethod::GET)([this]() {
        return safely([this]() {
            return infrastructure::http::jsonResponse(infrastructure::http::toJson(health_service_->getHealth()));
        });
    });

    CROW_ROUTE(app_, "/health/live").methods(crow::HTTPMethod::GET)([this]() {
        return safely([this]() {
            return infrastructure::http::jsonResponse({{"status", "ok"}});
        });
    });

    CROW_ROUTE(app_, "/health/ready").methods(crow::HTTPMethod::GET)([this]() {
        return safely([this]() {
            return infrastructure::http::jsonResponse(infrastructure::http::toJson(health_service_->getHealth()));
        });
    });

    CROW_ROUTE(app_, "/system").methods(crow::HTTPMethod::GET)([this]() {
        return safely([this]() {
            return infrastructure::http::jsonResponse(infrastructure::http::toJson(system_service_->getSystem()));
        });
    });

    CROW_ROUTE(app_, "/services").methods(crow::HTTPMethod::GET)([this](const crow::request& request) {
        return safely([this, &request]() {
            const auto names = queryValue(request, "names");
            return infrastructure::http::jsonResponse(infrastructure::http::toJson(services_service_->getStatuses(names)));
        });
    });

    CROW_ROUTE(app_, "/check/tcp").methods(crow::HTTPMethod::GET)([this](const crow::request& request) {
        return safely([this, &request]() {
            const auto host = queryValue(request, "host");
            const auto port = queryValue(request, "port");
            return infrastructure::http::jsonResponse(infrastructure::http::toJson(tcp_check_service_->check(host, port)));
        });
    });

    CROW_ROUTE(app_, "/metrics").methods(crow::HTTPMethod::GET)([this]() {
        return safely([this]() {
            crow::response response{metrics_service_->renderPrometheus()};
            response.add_header("Content-Type", "text/plain; version=0.0.4; charset=utf-8");
            return response;
        });
    });
}

} // namespace ops_agent::app
