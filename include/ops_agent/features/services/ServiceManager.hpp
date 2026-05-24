#pragma once

#include "ops_agent/features/services/ServiceStatusDto.hpp"

#include <string>

namespace ops_agent::features::services {

class ServiceManager {
public:
    virtual ~ServiceManager() = default;
    virtual ServiceStatusDto status(const std::string& name) const = 0;
};

} // namespace ops_agent::features::services
