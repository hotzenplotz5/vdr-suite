#pragma once

#include "EpgArtworkReference.h"

#include <mutex>
#include <string>

class Database;

class EpgSeriesArtworkFallbackRepository
{
public:
    explicit EpgSeriesArtworkFallbackRepository(Database& database);

    bool ensureSchema();
    bool upsert(const EpgArtworkReference& artwork);
    EpgArtworkReference find(
        const std::string& backendId,
        const std::string& channelId,
        const std::string& eventId) const;
    bool removeForEvent(
        const std::string& backendId,
        const std::string& channelId,
        const std::string& eventId);

private:
    Database& database_;
    mutable std::mutex mutex_;

    bool ensureSchemaLocked() const;
    static std::string normalizeBackendId(const std::string& backendId);
};
