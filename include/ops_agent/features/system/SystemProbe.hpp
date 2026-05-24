#pragma once

#include "ops_agent/features/system/SystemDto.hpp"

namespace ops_agent::features::system {

class SystemProbe {
public:
    virtual ~SystemProbe() = default;
    virtual SystemDto read() const = 0;
};

} // namespace ops_agent::features::system
