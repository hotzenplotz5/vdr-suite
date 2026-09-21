#pragma once

#include "IEpgScraperMetadataResolver.h"

#include <map>
#include <mutex>
#include <string>

class EpgScraperMetadataResolverRegistry
{
public:
    void registerResolver(
        const std::string& backendId,
        IEpgScraperMetadataResolver& resolver);

    IEpgScraperMetadataResolver* findResolver(
        const std::string& backendId) const;

private:
    mutable std::mutex mutex_;
    std::map<std::string, IEpgScraperMetadataResolver*> resolvers_;
};
