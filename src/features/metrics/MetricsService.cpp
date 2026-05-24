#include "ops_agent/features/metrics/MetricsService.hpp"

#include <sstream>

namespace ops_agent::features::metrics {

void MetricsService::recordRequest(int status_code, long long duration_ms) noexcept
{
    http_requests_total_.fetch_add(1, std::memory_order_relaxed);
    http_request_duration_ms_total_.fetch_add(static_cast<unsigned long long>(duration_ms), std::memory_order_relaxed);
    if (status_code >= 500) {
        http_errors_total_.fetch_add(1, std::memory_order_relaxed);
    }
}

std::string MetricsService::renderPrometheus() const
{
    std::ostringstream out;
    out << "# HELP ops_agent_http_requests_total Total HTTP requests.\n";
    out << "# TYPE ops_agent_http_requests_total counter\n";
    out << "ops_agent_http_requests_total " << http_requests_total_.load(std::memory_order_relaxed) << "\n";
    out << "# HELP ops_agent_http_errors_total Total HTTP 5xx responses.\n";
    out << "# TYPE ops_agent_http_errors_total counter\n";
    out << "ops_agent_http_errors_total " << http_errors_total_.load(std::memory_order_relaxed) << "\n";
    out << "# HELP ops_agent_http_request_duration_ms_total Total HTTP request duration in milliseconds.\n";
    out << "# TYPE ops_agent_http_request_duration_ms_total counter\n";
    out << "ops_agent_http_request_duration_ms_total "
        << http_request_duration_ms_total_.load(std::memory_order_relaxed) << "\n";
    return out.str();
}

} // namespace ops_agent::features::metrics
