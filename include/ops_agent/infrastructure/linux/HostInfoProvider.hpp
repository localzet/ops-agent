#pragma once

#include <chrono>
#include <string>

namespace ops_agent::infrastructure::linux {

class HostInfoProvider {
public:
    std::string hostname() const;
    std::chrono::seconds processUptime() const;
};

} // namespace ops_agent::infrastructure::linux
