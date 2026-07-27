#pragma once

#include "EpgArtworkReference.h"
#include "EpgScraperMetadata.h"

#include <mutex>
#include <string>

class Database;

class EpgArtworkRepository
{
public:
    explicit EpgArtworkRepository(Database& database);

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

    bool upsertMetadataJson(
        const std::string& backendId,
        const std::string& channelId,
        const std::string& eventId,
        const std::string& publicJson,
        long long resolvedAt);
    bool replaceMetadataPeople(
        const std::string& backendId,
        const std::string& channelId,
        const std::string& eventId,
        const std::vector<EpgScraperPerson>& people);
    std::string findMetadataJson(
        const std::string& backendId,
        const std::string& channelId,
        const std::string& eventId) const;

    bool upsertMetadataImage(
        const std::string& backendId,
        const std::string& channelId,
        const std::string& eventId,
        const std::string& kind,
        int imageIndex,
        const EpgArtworkReference& artwork);
    EpgArtworkReference findMetadataImage(
        const std::string& backendId,
        const std::string& channelId,
        const std::string& eventId,
        const std::string& kind,
        int imageIndex) const;

private:
    Database& database_;
    mutable std::mutex mutex_;

    bool ensureSchemaLocked() const;

    static std::string normalizeBackendId(const std::string& backendId);
};
