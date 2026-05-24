#include "ops_agent/features/services/ServicesService.hpp"

#include <sstream>
#include <stdexcept>

namespace ops_agent::features::services {
namespace {

std::vector<std::string> splitNames(const std::string& names_query)
{
    std::vector<std::string> names;
    std::stringstream stream{names_query};
    std::string item;

    while (std::getline(stream, item, ',')) {
        if (!item.empty()) {
            names.push_back(item);
        }
    }

    return names;
}

} // namespace

ServicesService::ServicesService(std::shared_ptr<ServiceManager> manager,
                                 config::DiagnosticsConfig diagnostics_config)
    : manager_(std::move(manager)),
      diagnostics_config_(diagnostics_config)
{
}

ServicesResponseDto ServicesService::getStatuses(const std::string& names_query) const
{
    const auto names = splitNames(names_query);
    if (names.empty()) {
        throw std::invalid_argument("query parameter 'names' is required");
    }
    if (names.size() > diagnostics_config_.max_services_per_request) {
        throw std::invalid_argument("too many services requested");
    }

    ServicesResponseDto response;
    response.services.reserve(names.size());
    for (const auto& name : names) {
        response.services.push_back(manager_->status(name));
    }
    return response;
}

} // namespace ops_agent::features::services
