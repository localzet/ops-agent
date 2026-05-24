#include "ops_agent/infrastructure/linux/SystemdServiceManager.hpp"

#include <regex>

namespace ops_agent::infrastructure::linux {
namespace {

bool isValidUnitName(const std::string& name)
{
    static const std::regex pattern{"^[A-Za-z0-9_.@:-]+$"};
    return !name.empty() && name.size() <= 128 && std::regex_match(name, pattern);
}

} // namespace

features::services::ServiceStatusDto SystemdServiceManager::status(const std::string& name) const
{
    features::services::ServiceStatusDto dto;
    dto.name = name;

    if (!isValidUnitName(name)) {
        dto.error = "invalid service name";
        return dto;
    }

    dto.load_state = "unknown";
    dto.active_state = "unknown";
    dto.sub_state = "unknown";
    dto.description = "systemd integration placeholder";
    dto.error = "systemctl query is not implemented in this skeleton";
    return dto;
}

} // namespace ops_agent::infrastructure::linux
