#pragma once

#include "ops_agent/features/services/ServiceManager.hpp"

namespace ops_agent::infrastructure::linux {

class SystemdServiceManager final : public features::services::ServiceManager {
public:
    features::services::ServiceStatusDto status(const std::string& name) const override;
};

} // namespace ops_agent::infrastructure::linux
