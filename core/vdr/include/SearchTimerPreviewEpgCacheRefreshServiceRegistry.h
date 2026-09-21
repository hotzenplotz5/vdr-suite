#pragma once

#include "SearchTimerPreviewEpgCacheRefreshService.h"

#include <map>
#include <string>

class SearchTimerPreviewEpgCacheRefreshServiceRegistry
{
public:
    void registerService(
        const std::string& backendId,
        SearchTimerPreviewEpgCacheRefreshService& service)
    {
        servicesByBackend_[normalizeBackendId(backendId)] = &service;
    }

    SearchTimerPreviewEpgCacheRefreshService* findService(
        const std::string& backendId) const
    {
        const auto iterator =
            servicesByBackend_.find(normalizeBackendId(backendId));

        if (iterator == servicesByBackend_.end()) {
            return nullptr;
        }

        return iterator->second;
    }

private:
    static std::string normalizeBackendId(const std::string& backendId)
    {
        if (backendId.empty()) {
            return "default";
        }

        return backendId;
    }

    std::map<std::string, SearchTimerPreviewEpgCacheRefreshService*> servicesByBackend_;
};
