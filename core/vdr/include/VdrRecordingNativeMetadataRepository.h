#pragma once

#include "VdrRecordingNativeMetadata.h"

#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

class Database;

struct VdrRecordingNativeMetadataRecord
{
    std::string backendId;
    std::string backendNativeId;
    std::string recordingKey;
    std::string contentState = "empty";
    std::string lastAttemptState = "never";
    std::int64_t resolvedAt = 0;
    std::int64_t expiresAt = 0;
    std::int64_t negativeExpiresAt = 0;
    int retryCount = 0;
    std::int64_t nextRetryAt = 0;
    std::string lastError;
    VdrRecordingNativeMetadata metadata;

    bool exists() const noexcept
    {
        return !recordingKey.empty();
    }
};

struct VdrRecordingNativePersonSearchQuery
{
    std::string name;
    std::string normalizedName;
    std::string characterName;
    std::string role;
    std::string source;
    std::string providerReference;
    int limit = 0;
    int offset = 0;
};

struct VdrRecordingNativePersonIndexEntry
{
    std::string backendId;
    std::string backendNativeId;
    std::string recordingKey;
    int ordinal = 0;
    std::string role;
    std::string name;
    std::string normalizedName;
    std::string characterName;
    std::string source = "tvscraper";
    std::string providerReference;
    VdrRecordingNativeArtwork image;
};

struct VdrRecordingNativePersonSearchResult
{
    int totalCount = 0;
    int limit = 0;
    int offset = 0;
    std::vector<VdrRecordingNativePersonIndexEntry> entries;
};

class VdrRecordingNativeMetadataRepository
{
public:
    explicit VdrRecordingNativeMetadataRepository(Database& database);

    bool ensureSchema();

    bool storeResolution(
        const std::string& backendId,
        const std::string& backendNativeId,
        const VdrRecordingNativeMetadata& metadata,
        std::int64_t resolvedAt,
        std::int64_t expiresAt,
        std::int64_t negativeExpiresAt);

    bool recordFailure(
        const std::string& backendId,
        const std::string& backendNativeId,
        const std::string& recordingKey,
        VdrRecordingNativeMetadataAvailability failure,
        const std::string& diagnostic,
        int retryCount,
        std::int64_t nextRetryAt);

    VdrRecordingNativeMetadataRecord find(
        const std::string& backendId,
        const std::string& recordingKey) const;

    VdrRecordingNativeMetadataRecord findByBackendNativeId(
        const std::string& backendId,
        const std::string& backendNativeId) const;

    VdrRecordingNativePersonSearchResult searchPeople(
        const std::string& backendId,
        const VdrRecordingNativePersonSearchQuery& query) const;

    bool removeMissingRecordings(
        const std::string& backendId,
        const std::vector<std::string>& activeRecordingKeys);

    std::vector<std::string> findDueRecordingKeys(
        const std::string& backendId,
        std::int64_t now,
        int limit) const;

private:
    Database& database_;
    mutable std::recursive_mutex mutex_;

    bool ensureSchemaLocked() const;
    static std::string normalizeBackendId(const std::string& backendId);
};
