#include "ops_agent/features/health/HealthService.hpp"

namespace ops_agent::features::health {

HealthService::HealthService(std::shared_ptr<common::Clock> clock,
                             std::shared_ptr<infrastructure::linux::HostInfoProvider> host_info)
    : clock_(std::move(clock)),
      host_info_(std::move(host_info))
{
}

HealthDto HealthService::getHealth() const
{
    return HealthDto{
        .status = "ok",
        .hostname = host_info_->hostname(),
        .version = OPS_AGENT_VERSION,
        .uptime_sec = host_info_->processUptime().count(),
        .timestamp = common::toIso8601Utc(clock_->now()),
    };
}

} // namespace ops_agent::features::health
