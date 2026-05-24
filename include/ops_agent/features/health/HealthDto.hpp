#pragma once

#include <cstdint>
#include <string>

namespace ops_agent::features::health {

struct HealthDto {
    std::string status;
    std::string hostname;
    std::string version;
    std::int64_t uptime_sec;
    std::string timestamp;
};

} // namespace ops_agent::features::health
