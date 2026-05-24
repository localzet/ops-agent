#pragma once

#include <chrono>
#include <string>

namespace ops_agent::common {

class Clock {
public:
    virtual ~Clock() = default;
    virtual std::chrono::system_clock::time_point now() const = 0;
};

class SystemClock final : public Clock {
public:
    std::chrono::system_clock::time_point now() const override;
};

std::string toIso8601Utc(std::chrono::system_clock::time_point time_point);

} // namespace ops_agent::common
