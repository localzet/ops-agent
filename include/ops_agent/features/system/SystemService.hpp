#pragma once

#include "ops_agent/features/system/SystemDto.hpp"
#include "ops_agent/features/system/SystemProbe.hpp"

#include <memory>

namespace ops_agent::features::system {

class SystemService {
public:
    explicit SystemService(std::shared_ptr<SystemProbe> probe);

    SystemDto getSystem() const;

private:
    std::shared_ptr<SystemProbe> probe_;
};

} // namespace ops_agent::features::system
