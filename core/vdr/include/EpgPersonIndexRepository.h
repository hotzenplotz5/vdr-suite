#pragma once

#include "EpgPersonIndex.h"
#include "EpgScraperMetadata.h"
#include "VdrEvent.h"

#include <mutex>
#include <string>
#include <vector>

class Database;

class EpgPersonIndexRepository
{
public:
    explicit EpgPersonIndexRepository(Database& database);

    bool ensureSchema();

    bool replaceEvidenceForEvent(
        const std::string& backendId,
        const VdrEvent& event,
        const EpgScraperMetadata& metadata);

    bool removeForEvent(
        const std::string& backendId,
        const std::string& channelId,
        const std::string& eventId);

    std::vector<EpgPersonIndexMatch> search(
        const EpgPersonIndexQuery& query) const;

    int count(const EpgPersonIndexQuery& query) const;

    bool deleteExpiredForBackend(
        const std::string& backendId,
        const std::string& beforeEndTime);

private:
    Database& database_;
    mutable std::recursive_mutex mutex_;

    bool ensureSchemaLocked() const;
    static std::string normalizeBackendId(const std::string& backendId);
};
