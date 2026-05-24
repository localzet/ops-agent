#include "ops_agent/infrastructure/linux/HostInfoProvider.hpp"

#include <array>
#include <chrono>
#include <stdexcept>
#include <string>

#if defined(__linux__)
#include <unistd.h>
#endif

namespace ops_agent::infrastructure::linux {
namespace {

const auto process_started_at = std::chrono::steady_clock::now();

} // namespace

std::string HostInfoProvider::hostname() const
{
#if defined(__linux__)
    std::array<char, 256> buffer{};
    if (gethostname(buffer.data(), buffer.size()) != 0) {
        throw std::runtime_error("failed to read hostname");
    }
    buffer.back() = '\0';
    return std::string{buffer.data()};
#else
    return "unsupported";
#endif
}

std::chrono::seconds HostInfoProvider::processUptime() const
{
    return std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::steady_clock::now() - process_started_at);
}

} // namespace ops_agent::infrastructure::linux
