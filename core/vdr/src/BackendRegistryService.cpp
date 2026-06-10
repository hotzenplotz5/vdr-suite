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

std::vector<BackendNode> BackendRegistryService::listBackends() const
{
    return registry_.listBackends();
}

std::optional<BackendNode> BackendRegistryService::defaultBackend() const
{
    return registry_.getBackend("default");
}
