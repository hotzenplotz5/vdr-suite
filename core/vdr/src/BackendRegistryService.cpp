#include "BackendRegistryService.h"

BackendRegistryService::BackendRegistryService(BackendRegistry& registry)
    : registry_(registry)
{
}

bool BackendRegistryService::hasBackend(const std::string& backendId) const
{
    return registry_.hasBackend(backendId);
}

std::optional<BackendNode> BackendRegistryService::getBackend(
    const std::string& backendId) const
{
    return registry_.getBackend(backendId);
}

bool BackendRegistryService::updateBackendCapabilities(
    const std::string& backendId,
    const VdrCapabilitySet& capabilities)
{
    return registry_.updateBackendCapabilities(
        backendId,
        capabilities);
}

std::vector<BackendNode> BackendRegistryService::listBackends() const
{
    return registry_.listBackends();
}

std::optional<BackendNode> BackendRegistryService::defaultBackend() const
{
    return registry_.getBackend("default");
}
