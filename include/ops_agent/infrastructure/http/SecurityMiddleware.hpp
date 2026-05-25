#pragma once

#include "ops_agent/config/Config.hpp"

#include <crow.h>

#include <algorithm>
#include <chrono>
#include <cctype>
#include <mutex>
#include <string>
#include <unordered_map>
#include <utility>

namespace ops_agent::infrastructure::http {

inline bool isHealthEndpoint(const std::string& path)
{
    return path == "/health" || path == "/health/live" || path == "/health/ready";
}

inline std::string jsonError(const std::string& message)
{
    return "{\"error\":\"" + message + "\"}";
}

class RequestSecurityMiddleware {
public:
    struct context {};

    void setMaxRequestBodyBytes(std::size_t value)
    {
        max_request_body_bytes_ = value;
    }

    void before_handle(crow::request& request, crow::response& response, context&)
    {
        const auto content_length = request.get_header_value("Content-Length");
        if (!content_length.empty()) {
            try {
                if (std::stoull(content_length) > max_request_body_bytes_) {
                    reject(response, 413, "request body too large");
                    return;
                }
            } catch (const std::exception&) {
                reject(response, 400, "invalid content length");
                return;
            }
        }

        if (request.body.size() > max_request_body_bytes_) {
            reject(response, 413, "request body too large");
        }
    }

    void after_handle(crow::request&, crow::response&, context&) {}

private:
    static void reject(crow::response& response, int status, const std::string& message)
    {
        response.code = status;
        response.body = jsonError(message);
        response.add_header("Content-Type", "application/json");
        response.end();
    }

    std::size_t max_request_body_bytes_{65536};
};

class ApiKeyAuthMiddleware {
public:
    struct context {};

    void setApiKey(std::string api_key)
    {
        api_key_ = std::move(api_key);
    }

    void before_handle(crow::request& request, crow::response& response, context&)
    {
        if (api_key_.empty() || isHealthEndpoint(request.url)) {
            return;
        }

        const auto provided = request.get_header_value("X-API-Key");
        if (!constantTimeEquals(provided, api_key_)) {
            response.code = 401;
            response.body = jsonError("unauthorized");
            response.add_header("Content-Type", "application/json");
            response.end();
        }
    }

    void after_handle(crow::request&, crow::response&, context&) {}

private:
    static bool constantTimeEquals(const std::string& lhs, const std::string& rhs)
    {
        const auto max_size = std::max(lhs.size(), rhs.size());
        unsigned char diff = static_cast<unsigned char>(lhs.size() ^ rhs.size());
        for (std::size_t i = 0; i < max_size; ++i) {
            const auto left = i < lhs.size() ? static_cast<unsigned char>(lhs[i]) : 0;
            const auto right = i < rhs.size() ? static_cast<unsigned char>(rhs[i]) : 0;
            diff |= static_cast<unsigned char>(left ^ right);
        }
        return diff == 0;
    }

    std::string api_key_;
};

class RateLimitMiddleware {
public:
    struct context {};

    void configure(config::RateLimitConfig config)
    {
        enabled_ = config.enabled;
        refill_per_second_ = std::max(0.1, config.requests_per_second);
        burst_ = std::max<std::size_t>(1, config.burst);
    }

    void before_handle(crow::request& request, crow::response& response, context&)
    {
        if (!enabled_ || isHealthEndpoint(request.url)) {
            return;
        }

        if (!allow(request.remote_ip_address)) {
            response.code = 429;
            response.body = jsonError("rate limit exceeded");
            response.add_header("Content-Type", "application/json");
            response.add_header("Retry-After", "1");
            response.end();
        }
    }

    void after_handle(crow::request&, crow::response&, context&) {}

private:
    struct Bucket {
        double tokens{0.0};
        std::chrono::steady_clock::time_point updated_at{std::chrono::steady_clock::now()};
    };

    bool allow(const std::string& raw_ip)
    {
        const auto now = std::chrono::steady_clock::now();
        const auto ip = raw_ip.empty() ? "unknown" : raw_ip;

        std::lock_guard<std::mutex> lock{mutex_};
        auto [it, inserted] = buckets_.try_emplace(ip, Bucket{static_cast<double>(burst_), now});
        auto& bucket = it->second;

        const auto elapsed = std::chrono::duration<double>(now - bucket.updated_at).count();
        bucket.tokens = std::min(static_cast<double>(burst_), bucket.tokens + elapsed * refill_per_second_);
        bucket.updated_at = now;

        if (bucket.tokens < 1.0) {
            return false;
        }

        bucket.tokens -= 1.0;
        return true;
    }

    bool enabled_{true};
    double refill_per_second_{10.0};
    std::size_t burst_{20};
    std::mutex mutex_;
    std::unordered_map<std::string, Bucket> buckets_;
};

class SecurityHeadersMiddleware {
public:
    struct context {};

    void before_handle(crow::request&, crow::response&, context&) {}

    void after_handle(crow::request&, crow::response& response, context&)
    {
        response.add_header("X-Content-Type-Options", "nosniff");
        response.add_header("Cache-Control", "no-store");
        response.add_header("Referrer-Policy", "no-referrer");
        response.add_header("Server", "");
    }
};

} // namespace ops_agent::infrastructure::http
