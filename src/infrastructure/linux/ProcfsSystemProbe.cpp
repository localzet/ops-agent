#include "ops_agent/infrastructure/linux/ProcfsSystemProbe.hpp"

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>

#if defined(__linux__)
#include <sys/statvfs.h>
#include <unistd.h>
#endif

namespace ops_agent::infrastructure::linux {
namespace {

std::string readHostname()
{
#if defined(__linux__)
    char buffer[256]{};
    if (gethostname(buffer, sizeof(buffer)) != 0) {
        throw std::runtime_error("failed to read hostname");
    }
    return std::string{buffer};
#else
    return "unsupported";
#endif
}

features::system::CpuLoadDto readCpuLoad()
{
    std::ifstream file{"/proc/loadavg"};
    if (!file.is_open()) {
        return {};
    }

    features::system::CpuLoadDto dto;
    file >> dto.one_minute >> dto.five_minutes >> dto.fifteen_minutes;
    return dto;
}

features::system::MemoryDto readMemory()
{
    std::ifstream file{"/proc/meminfo"};
    if (!file.is_open()) {
        return {};
    }

    std::string key;
    std::uint64_t value_kb = 0;
    std::string unit;
    std::uint64_t total_kb = 0;
    std::uint64_t available_kb = 0;

    while (file >> key >> value_kb >> unit) {
        if (key == "MemTotal:") {
            total_kb = value_kb;
        } else if (key == "MemAvailable:") {
            available_kb = value_kb;
        }
    }

    features::system::MemoryDto dto;
    dto.total_bytes = total_kb * 1024;
    dto.available_bytes = available_kb * 1024;
    dto.used_bytes = dto.total_bytes > dto.available_bytes ? dto.total_bytes - dto.available_bytes : 0;
    dto.used_percent = dto.total_bytes == 0 ? 0.0 : static_cast<double>(dto.used_bytes) * 100.0 / dto.total_bytes;
    return dto;
}

features::system::DiskDto readDisk()
{
    features::system::DiskDto dto;
#if defined(__linux__)
    struct statvfs stats {};
    if (statvfs("/", &stats) != 0) {
        return dto;
    }

    dto.total_bytes = static_cast<std::uint64_t>(stats.f_blocks) * stats.f_frsize;
    dto.available_bytes = static_cast<std::uint64_t>(stats.f_bavail) * stats.f_frsize;
    dto.used_bytes = dto.total_bytes > dto.available_bytes ? dto.total_bytes - dto.available_bytes : 0;
    dto.used_percent = dto.total_bytes == 0 ? 0.0 : static_cast<double>(dto.used_bytes) * 100.0 / dto.total_bytes;
#endif
    return dto;
}

std::int64_t readUptime()
{
    std::ifstream file{"/proc/uptime"};
    if (!file.is_open()) {
        return 0;
    }

    double uptime = 0.0;
    file >> uptime;
    return static_cast<std::int64_t>(uptime);
}

} // namespace

features::system::SystemDto ProcfsSystemProbe::read() const
{
    return features::system::SystemDto{
        .hostname = readHostname(),
        .uptime_sec = readUptime(),
        .cpu_load = readCpuLoad(),
        .memory = readMemory(),
        .disk = readDisk(),
    };
}

} // namespace ops_agent::infrastructure::linux
