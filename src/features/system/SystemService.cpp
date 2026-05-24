#include "ops_agent/features/system/SystemService.hpp"

namespace ops_agent::features::system {

SystemService::SystemService(std::shared_ptr<SystemProbe> probe)
    : probe_(std::move(probe))
{
}

SystemDto SystemService::getSystem() const
{
    return probe_->read();
}

} // namespace ops_agent::features::system
