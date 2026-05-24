#include "ops_agent/config/ConfigLoader.hpp"

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace ops_agent::config {
namespace {

std::string trim(std::string value)
{
    const auto begin = value.find_first_not_of(" \t\r\n\"");
    if (begin == std::string::npos) {
        return {};
    }
    const auto end = value.find_last_not_of(" \t\r\n\"");
    return value.substr(begin, end - begin + 1);
}

std::optional<std::string> envValue(const char* name)
{
    if (const char* value = std::getenv(name); value != nullptr && *value != '\0') {
        return std::string{value};
    }
    return std::nullopt;
}

std::uint16_t parseUint16(const std::string& value, const char* field)
{
    const auto parsed = std::stoul(value);
    if (parsed > 65535) {
        throw std::runtime_error(std::string{field} + " is out of range");
    }
    return static_cast<std::uint16_t>(parsed);
}

int parseInt(const std::string& value, const char* field)
{
    const auto parsed = std::stoi(value);
    if (parsed < 0) {
        throw std::runtime_error(std::string{field} + " must be non-negative");
    }
    return parsed;
}

} // namespace

Config ConfigLoader::load(const std::optional<std::string>& path) const
{
    Config config;

    if (path.has_value()) {
        std::ifstream file{*path};
        if (!file.is_open()) {
            throw std::runtime_error("failed to open config file: " + *path);
        }

        std::string section;
        std::string line;
        while (std::getline(file, line)) {
            const auto comment = line.find('#');
            if (comment != std::string::npos) {
                line = line.substr(0, comment);
            }
            if (trim(line).empty()) {
                continue;
            }

            if (line.find(':') != std::string::npos && line.find_first_not_of(" \t") == 0 && line.back() == ':') {
                section = trim(line.substr(0, line.size() - 1));
                continue;
            }

            const auto delimiter = line.find(':');
            if (delimiter == std::string::npos) {
                continue;
            }

            const auto key = trim(line.substr(0, delimiter));
            const auto value = trim(line.substr(delimiter + 1));
            const auto full_key = section + "." + key;

            if (full_key == "server.host") {
                config.server.host = value;
            } else if (full_key == "server.port") {
                config.server.port = parseUint16(value, "server.port");
            } else if (full_key == "server.threads") {
                config.server.threads = parseUint16(value, "server.threads");
            } else if (full_key == "diagnostics.tcp_timeout_ms") {
                config.diagnostics.tcp_timeout_ms = parseInt(value, "diagnostics.tcp_timeout_ms");
            } else if (full_key == "diagnostics.max_services_per_request") {
                config.diagnostics.max_services_per_request = std::stoul(value);
            } else if (full_key == "logging.level") {
                config.logging.level = value;
            } else if (full_key == "storage.sqlite_path") {
                config.storage.sqlite_path = value;
            }
        }
    }

    applyEnv(config);
    return config;
}

void ConfigLoader::applyEnv(Config& config)
{
    if (auto value = envValue("OPS_AGENT_HOST")) {
        config.server.host = *value;
    }
    if (auto value = envValue("OPS_AGENT_PORT")) {
        config.server.port = parseUint16(*value, "OPS_AGENT_PORT");
    }
    if (auto value = envValue("OPS_AGENT_THREADS")) {
        config.server.threads = parseUint16(*value, "OPS_AGENT_THREADS");
    }
    if (auto value = envValue("OPS_AGENT_TCP_TIMEOUT_MS")) {
        config.diagnostics.tcp_timeout_ms = parseInt(*value, "OPS_AGENT_TCP_TIMEOUT_MS");
    }
    if (auto value = envValue("OPS_AGENT_MAX_SERVICES_PER_REQUEST")) {
        config.diagnostics.max_services_per_request = std::stoul(*value);
    }
    if (auto value = envValue("OPS_AGENT_LOG_LEVEL")) {
        config.logging.level = *value;
    }
    if (auto value = envValue("OPS_AGENT_SQLITE_PATH")) {
        config.storage.sqlite_path = *value;
    }
}

} // namespace ops_agent::config
