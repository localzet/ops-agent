#pragma once

#include <atomic>
#include <string>

namespace ops_agent::features::metrics {

class MetricsService {
public:
    void recordRequest(int status_code, long long duration_ms) noexcept;
    std::string renderPrometheus() const;

private:
    std::atomic<unsigned long long> http_requests_total_{0};
    std::atomic<unsigned long long> http_errors_total_{0};
    std::atomic<unsigned long long> http_request_duration_ms_total_{0};
};

} // namespace ops_agent::features::metrics
