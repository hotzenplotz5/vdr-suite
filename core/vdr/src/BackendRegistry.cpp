#include "BackendRegistry.h"

void BackendRegistry::addBackend(const BackendNode& backend)
{
    backends_.push_back(backend);
}

bool BackendRegistry::hasBackend(const std::string& backendId) const
{
    return getBackend(backendId).has_value();
}

std::optional<BackendNode> BackendRegistry::getBackend(const std::string& backendId) const
{
    for (const auto& backend : backends_)
    {
        if (backend.backendId == backendId)
        {
            return backend;
        }
    }

    return std::nullopt;
}

std::vector<BackendNode> BackendRegistry::listBackends() const
{
    return backends_;
}
