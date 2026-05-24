#pragma once

#include "ops_agent/features/system/SystemProbe.hpp"

namespace ops_agent::infrastructure::linux {

class ProcfsSystemProbe final : public features::system::SystemProbe {
public:
    features::system::SystemDto read() const override;
};

} // namespace ops_agent::infrastructure::linux
