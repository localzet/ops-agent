#pragma once

#include "ops_agent/config/Config.hpp"
#include "ops_agent/features/services/ServiceManager.hpp"
#include "ops_agent/features/services/ServiceStatusDto.hpp"

#include <memory>
#include <string>

namespace ops_agent::features::services {

class ServicesService {
public:
    ServicesService(std::shared_ptr<ServiceManager> manager,
                    config::DiagnosticsConfig diagnostics_config);

    ServicesResponseDto getStatuses(const std::string& names_query) const;

private:
    std::shared_ptr<ServiceManager> manager_;
    config::DiagnosticsConfig diagnostics_config_;
};

} // namespace ops_agent::features::services
