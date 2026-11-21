#include "PublicApiRuntime.h"

#include <mutex>
#include <utility>

// The existing backend-scoped callback lifecycle is kept on the original
// PublicApiRuntime instance. The response/paging helper dependencies remain
// in PublicApiRuntime.cpp until a separate, contract-tested source move.
void PublicApiRuntime::registerBackendCollectionLookup(
    BackendCollectionLookup lookup)
{
    std::lock_guard<std::mutex> lock(
        backendCollectionLookupMutex_);
    backendCollectionLookup_ = std::move(lookup);
}

void PublicApiRuntime::resetBackendCollectionLookup()
{
    std::lock_guard<std::mutex> lock(
        backendCollectionLookupMutex_);
    backendCollectionLookup_ = {};
}

bool PublicApiRuntime::backendCollectionLookupConfigured() const
{
    std::lock_guard<std::mutex> lock(
        backendCollectionLookupMutex_);
    return static_cast<bool>(backendCollectionLookup_);
}

PublicBackendCollectionResult
PublicApiRuntime::lookupBackendCollection(
    const PublicBackendCollectionRequest& request) const
{
    BackendCollectionLookup lookup;
    {
        std::lock_guard<std::mutex> lock(
            backendCollectionLookupMutex_);
        lookup = backendCollectionLookup_;
    }
    if (!lookup) return {};
    return lookup(request);
}

