#pragma once

#include "VdrChannel.h"

#include <mutex>
#include <string>
#include <vector>

class Database;

class VdrChannelCacheRepository
{
public:
    explicit VdrChannelCacheRepository(Database& database);

    bool ensureSchema();

    bool replaceChannelsForBackend(
        const std::string& backendId,
        const std::vector<VdrChannel>& channels);

    std::vector<VdrChannel> findAllForBackend(
        const std::string& backendId) const;

    int countForBackend(
        const std::string& backendId) const;

private:
    Database& database_;
    mutable std::recursive_mutex mutex_;

    static std::string normalizeBackendId(
        const std::string& backendId);
};
