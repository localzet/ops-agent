#include "ops_agent/common/Clock.hpp"

#include <ctime>
#include <iomanip>
#include <sstream>

namespace ops_agent::common {

std::chrono::system_clock::time_point SystemClock::now() const
{
    return std::chrono::system_clock::now();
}

std::string toIso8601Utc(std::chrono::system_clock::time_point time_point)
{
    const auto time = std::chrono::system_clock::to_time_t(time_point);
    std::tm utc{};
#if defined(_WIN32)
    gmtime_s(&utc, &time);
#else
    gmtime_r(&time, &utc);
#endif
    std::ostringstream out;
    out << std::put_time(&utc, "%Y-%m-%dT%H:%M:%SZ");
    return out.str();
}

} // namespace ops_agent::common
