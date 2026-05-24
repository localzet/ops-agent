#pragma once

#include "ops_agent/features/health/HealthDto.hpp"
#include "ops_agent/features/services/ServiceStatusDto.hpp"
#include "ops_agent/features/system/SystemDto.hpp"
#include "ops_agent/features/tcp_check/TcpCheckDto.hpp"

#include <crow.h>
#include <nlohmann/json.hpp>

#include <string>

namespace ops_agent::infrastructure::http {

crow::response jsonResponse(const nlohmann::json& body, int status = 200);
crow::response errorResponse(const std::string& message, int status);

nlohmann::json toJson(const features::health::HealthDto& dto);
nlohmann::json toJson(const features::system::SystemDto& dto);
nlohmann::json toJson(const features::services::ServicesResponseDto& dto);
nlohmann::json toJson(const features::tcp_check::TcpCheckDto& dto);

} // namespace ops_agent::infrastructure::http
