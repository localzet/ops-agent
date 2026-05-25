#pragma once

#include <string>
#include <vector>

namespace ops_agent::features::services {

struct ServiceStatusDto {
    std::string name;
    std::string status;
    std::string load_state;
    std::string active_state;
    std::string sub_state;
    std::string description;
    std::string error;
};

struct ServicesResponseDto {
    std::vector<ServiceStatusDto> services;
};

} // namespace ops_agent::features::services
