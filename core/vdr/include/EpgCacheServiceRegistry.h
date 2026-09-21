#pragma once

#include "EpgCacheService.h"

#include <map>
#include <string>

class EpgCacheServiceRegistry
{
public:
    bool registerService(
        const std::string& backendId,
        EpgCacheService& service)
    {
        services_[normalizeBackendId(backendId)] = &service;
        return true;
    }

    EpgCacheService* findService(
        const std::string& backendId) const
    {
        const auto iterator = services_.find(normalizeBackendId(backendId));

        if (iterator == services_.end())
        {
            return nullptr;
        }

        return iterator->second;
    }

    std::size_t size() const
    {
        return services_.size();
    }

private:
    std::map<std::string, EpgCacheService*> services_;

    static std::string normalizeBackendId(
        const std::string& backendId)
    {
        if (backendId.empty())
        {
            return "default";
        }

        return backendId;
    }
};
