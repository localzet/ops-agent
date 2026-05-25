#pragma once

#include "ops_agent/features/services/ServiceManager.hpp"

namespace ops_agent::infrastructure::linux {

class SystemdServiceManager final : public features::services::ServiceManager {
public:
    explicit SystemdServiceManager(int timeout_ms);

    features::services::ServiceStatusDto status(const std::string& name) const override;

private:
    int timeout_ms_;
};

} // namespace ops_agent::infrastructure::linux
