#pragma once

#include <cstdint>
#include <string>

namespace ops_agent::features::system {

struct CpuLoadDto {
    double one_minute{0.0};
    double five_minutes{0.0};
    double fifteen_minutes{0.0};
};

struct MemoryDto {
    std::uint64_t total_bytes{0};
    std::uint64_t available_bytes{0};
    std::uint64_t used_bytes{0};
    double used_percent{0.0};
};

struct DiskDto {
    std::string path{"/"};
    std::uint64_t total_bytes{0};
    std::uint64_t available_bytes{0};
    std::uint64_t used_bytes{0};
    double used_percent{0.0};
};

struct SystemDto {
    std::string hostname;
    std::int64_t uptime_sec{0};
    CpuLoadDto cpu_load;
    MemoryDto memory;
    DiskDto disk;
};

} // namespace ops_agent::features::system
