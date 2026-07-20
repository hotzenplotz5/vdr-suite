#pragma once

#include "EpgMetadataJsonParser.h"
#include "EpgMetadataRecord.h"

#include <mutex>
#include <string>

class Database;

class EpgMetadataRepository
{
public:
    explicit EpgMetadataRepository(Database& database);

    bool ensureSchema();
    bool upsert(const EpgMetadataRecord& metadata);

    EpgMetadataRecord find(
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
    EpgMetadataJsonParser parser_;

    bool ensureSchemaLocked() const;
    static std::string normalizeBackendId(const std::string& backendId);
};
