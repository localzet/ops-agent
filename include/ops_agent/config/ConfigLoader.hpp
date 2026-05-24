#pragma once

#include "ops_agent/config/Config.hpp"

#include <optional>
#include <string>

namespace ops_agent::config {

class ConfigLoader {
public:
    Config load(const std::optional<std::string>& path) const;

private:
    static void applyEnv(Config& config);
};

} // namespace ops_agent::config
