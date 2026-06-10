#include "BackendRegistry.h"

void BackendRegistry::addBackend(const BackendNode& backend)
{
    backends_.push_back(backend);
}

bool BackendRegistry::hasBackend(const std::string& backendId) const
{
    for (const auto& backend : backends_)
    {
        if (backend.backendId == backendId)
        {
            return true;
        }
    }

    return false;
}
