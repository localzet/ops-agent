#pragma once

#include "ops_agent/features/metrics/MetricsService.hpp"

#include <crow.h>
#include <spdlog/spdlog.h>

#include <chrono>
#include <memory>

namespace ops_agent::infrastructure::http {

class RequestLoggingMiddleware {
public:
    struct context {
        std::chrono::steady_clock::time_point started_at;
    };

    void setLogger(std::shared_ptr<spdlog::logger> logger)
    {
        logger_ = std::move(logger);
    }

    void setMetrics(std::shared_ptr<features::metrics::MetricsService> metrics)
    {
        metrics_ = std::move(metrics);
    }

    void before_handle(crow::request& request, crow::response&, context& ctx)
    {
        (void)request;
        ctx.started_at = std::chrono::steady_clock::now();
    }

    void after_handle(crow::request& request, crow::response& response, context& ctx)
    {
        const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - ctx.started_at);

        if (metrics_ != nullptr) {
            metrics_->recordRequest(response.code, elapsed.count());
        }

        if (logger_ != nullptr) {
            logger_->info("event=request method={} path={} status={} duration_ms={}",
                          crow::method_name(request.method),
                          request.url,
                          response.code,
                          elapsed.count());
        }
    }

private:
    std::shared_ptr<spdlog::logger> logger_;
    std::shared_ptr<features::metrics::MetricsService> metrics_;
};

class SecurityHeadersMiddleware {
public:
    struct context {};

    void before_handle(crow::request&, crow::response&, context&) {}

    void after_handle(crow::request&, crow::response& response, context&)
    {
        response.add_header("X-Content-Type-Options", "nosniff");
        response.add_header("Cache-Control", "no-store");
    }
};

} // namespace ops_agent::infrastructure::http
