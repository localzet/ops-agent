#include "ops_agent/infrastructure/http/HttpResponse.hpp"

namespace ops_agent::infrastructure::http {

crow::response jsonResponse(const nlohmann::json& body, int status)
{
    crow::response response{status, body.dump()};
    response.add_header("Content-Type", "application/json");
    return response;
}

crow::response errorResponse(const std::string& message, int status)
{
    return jsonResponse({{"error", message}}, status);
}

nlohmann::json toJson(const features::health::HealthDto& dto)
{
    return {
        {"status", dto.status},
        {"hostname", dto.hostname},
        {"version", dto.version},
        {"uptime_sec", dto.uptime_sec},
        {"timestamp", dto.timestamp},
    };
}

nlohmann::json toJson(const features::system::SystemDto& dto)
{
    return {
        {"hostname", dto.hostname},
        {"uptime_sec", dto.uptime_sec},
        {"cpu_load", {
            {"one_minute", dto.cpu_load.one_minute},
            {"five_minutes", dto.cpu_load.five_minutes},
            {"fifteen_minutes", dto.cpu_load.fifteen_minutes},
        }},
        {"memory", {
            {"total_bytes", dto.memory.total_bytes},
            {"available_bytes", dto.memory.available_bytes},
            {"used_bytes", dto.memory.used_bytes},
            {"used_percent", dto.memory.used_percent},
        }},
        {"disk", {
            {"path", dto.disk.path},
            {"total_bytes", dto.disk.total_bytes},
            {"available_bytes", dto.disk.available_bytes},
            {"used_bytes", dto.disk.used_bytes},
            {"used_percent", dto.disk.used_percent},
        }},
    };
}

nlohmann::json toJson(const features::services::ServicesResponseDto& dto)
{
    nlohmann::json services = nlohmann::json::array();
    for (const auto& service : dto.services) {
        services.push_back({
            {"name", service.name},
            {"load_state", service.load_state},
            {"active_state", service.active_state},
            {"sub_state", service.sub_state},
            {"description", service.description},
            {"error", service.error},
        });
    }
    return {{"services", services}};
}

nlohmann::json toJson(const features::tcp_check::TcpCheckDto& dto)
{
    return {
        {"host", dto.host},
        {"port", dto.port},
        {"reachable", dto.reachable},
        {"latency_ms", dto.latency_ms},
        {"error", dto.error},
    };
}

} // namespace ops_agent::infrastructure::http
