#pragma once

#include "ops_agent/features/metrics/MetricsService.hpp"

#include <crow.h>
#include <spdlog/spdlog.h>

#include <chrono>
#include <algorithm>
#include <atomic>
#include <cctype>
#include <sstream>
#include <memory>
#include <string>

namespace ops_agent::infrastructure::http {

class RequestLoggingMiddleware {
public:
    struct context {
        std::chrono::steady_clock::time_point started_at;
        std::string request_id;
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
        ctx.started_at = std::chrono::steady_clock::now();
        ctx.request_id = validRequestId(request.get_header_value("X-Request-Id"))
            ? request.get_header_value("X-Request-Id")
            : generateRequestId();
    }

    void after_handle(crow::request& request, crow::response& response, context& ctx)
    {
        const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - ctx.started_at);

        if (metrics_ != nullptr) {
            metrics_->recordRequest(response.code, elapsed.count());
        }

        response.add_header("X-Request-Id", ctx.request_id);

        if (logger_ != nullptr) {
            logger_->info(
                "{{\"event\":\"request\",\"request_id\":\"{}\",\"method\":\"{}\",\"endpoint\":\"{}\","
                "\"status_code\":{},\"latency_ms\":{},\"remote_ip\":\"{}\"}}",
                jsonEscape(ctx.request_id),
                jsonEscape(crow::method_name(request.method)),
                jsonEscape(sanitizeHeaderValue(request.url)),
                response.code,
                elapsed.count(),
                jsonEscape(sanitizeHeaderValue(request.remote_ip_address)));
        }
    }

private:
    static bool validRequestId(const std::string& value)
    {
        if (value.empty() || value.size() > 64) {
            return false;
        }
        return std::all_of(value.begin(), value.end(), [](unsigned char ch) {
            return std::isalnum(ch) != 0 || ch == '-' || ch == '_';
        });
    }

    static std::string generateRequestId()
    {
        static std::atomic<unsigned long long> counter{0};
        const auto now = std::chrono::steady_clock::now().time_since_epoch().count();
        std::ostringstream out;
        out << std::hex << now << "-" << counter.fetch_add(1, std::memory_order_relaxed);
        return out.str();
    }

    static std::string sanitizeHeaderValue(const std::string& value)
    {
        std::string sanitized;
        sanitized.reserve(std::min<std::size_t>(value.size(), 256));
        for (const unsigned char ch : value) {
            if (sanitized.size() >= 256) {
                break;
            }
            if (ch >= 32 && ch != 127) {
                sanitized.push_back(static_cast<char>(ch));
            }
        }
        return sanitized;
    }

    static std::string jsonEscape(const std::string& value)
    {
        std::string escaped;
        escaped.reserve(value.size());
        for (const char ch : value) {
            switch (ch) {
            case '\\':
                escaped += "\\\\";
                break;
            case '"':
                escaped += "\\\"";
                break;
            default:
                escaped.push_back(ch);
                break;
            }
        }
        return escaped;
    }

    std::shared_ptr<spdlog::logger> logger_;
    std::shared_ptr<features::metrics::MetricsService> metrics_;
};

} // namespace ops_agent::infrastructure::http
