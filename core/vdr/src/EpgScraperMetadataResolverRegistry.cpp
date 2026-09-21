#include "EpgScraperMetadataResolverRegistry.h"

namespace
{

std::string normalizeBackendId(const std::string& backendId)
{
    return backendId.empty() ? "default" : backendId;
}

}

void EpgScraperMetadataResolverRegistry::registerResolver(
    const std::string& backendId,
    IEpgScraperMetadataResolver& resolver)
{
    std::lock_guard<std::mutex> lock(mutex_);
    resolvers_[normalizeBackendId(backendId)] = &resolver;
}

IEpgScraperMetadataResolver* EpgScraperMetadataResolverRegistry::findResolver(
    const std::string& backendId) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    const auto iterator = resolvers_.find(normalizeBackendId(backendId));
    return iterator == resolvers_.end() ? nullptr : iterator->second;
}
