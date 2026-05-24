#pragma once

#include "ops_agent/common/Clock.hpp"
#include "ops_agent/features/health/HealthDto.hpp"
#include "ops_agent/infrastructure/linux/HostInfoProvider.hpp"

#include <memory>

namespace ops_agent::features::health {

class HealthService {
public:
    HealthService(std::shared_ptr<common::Clock> clock,
                  std::shared_ptr<infrastructure::linux::HostInfoProvider> host_info);

    HealthDto getHealth() const;

private:
    std::shared_ptr<common::Clock> clock_;
    std::shared_ptr<infrastructure::linux::HostInfoProvider> host_info_;
};

} // namespace ops_agent::features::health
