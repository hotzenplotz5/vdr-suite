#pragma once

#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

class Database;

struct GenreEvidenceInput
{
    std::string backendId;
    std::string targetType;
    std::string resourceKey;
    std::string nativeId;
    std::string channelId;
    std::int64_t startTime = 0;
    std::int64_t endTime = 0;
    std::string providerId;
    std::string sourceKind;
    std::vector<std::string> originalValues;
    std::string state = "active";
    double confidence = 0.0;
    std::int64_t observedAt = 0;
};

struct GenreOverviewEntry
{
    std::string genreId;
    std::string label;
    std::string labelDe;
    std::string labelEn;
    bool known = false;
    int itemCount = 0;
    int activeCount = 0;
    int staleCount = 0;
    int conflictCount = 0;
    std::vector<std::string> sources;
};

struct GenreOverview
{
    std::string backendId;
    std::string targetType;
    std::int64_t fromTime = 0;
    std::int64_t untilTime = 0;
    int distinctItemCount = 0;
    int assignmentCount = 0;
    std::vector<GenreOverviewEntry> genres;
};

struct GenreRecordingItem
{
    std::string id;
    std::string backendId;
    std::string backendNativeId;
    std::string title;
    std::string path;
    std::string startTime;
    int durationSeconds = 0;
    std::int64_t sizeMb = 0;
    std::string metadataPayload;
    std::vector<std::string> genreIds;
};

struct GenreRecordingPage
{
    std::string backendId;
    std::string genreId;
    int limit = 0;
    int offset = 0;
    int totalCount = 0;
    std::vector<GenreRecordingItem> recordings;
};

struct GenreEpgItem
{
    std::string backendId;
    std::string channelId;
    std::string eventId;
    std::string title;
    std::string subtitle;
    std::string description;
    std::int64_t startTime = 0;
    std::int64_t endTime = 0;
    int durationSeconds = 0;
    std::vector<std::string> genreIds;
};

struct GenreEpgPage
{
    std::string backendId;
    std::string genreId;
    std::int64_t fromTime = 0;
    std::int64_t untilTime = 0;
    int limit = 0;
    int offset = 0;
    int totalCount = 0;
    std::vector<GenreEpgItem> events;
};

class GenreIndexRepository
{
public:
    explicit GenreIndexRepository(Database& database);

    bool ensureSchema();
    bool replaceEvidence(const GenreEvidenceInput& input);
    bool markProviderEvidenceStale(
        const std::string& backendId,
        const std::string& targetType,
        const std::string& resourceKey,
        const std::string& providerId);
    bool providerEvidenceNeedsRefresh(
        const std::string& backendId,
        const std::string& targetType,
        const std::string& resourceKey,
        const std::string& providerId,
        std::int64_t freshAfterEpoch) const;
    bool retireMissingRecordingTargets(const std::string& backendId);
    bool retireExpiredEpgTargets(
        const std::string& backendId,
        std::int64_t beforeEndTime);

    bool synchronizeRecordingCache(const std::string& backendId);
    bool synchronizeEpgCache(
        const std::string& backendId,
        std::int64_t fromTime,
        std::int64_t untilTime);

    GenreOverview overview(
        const std::string& backendId,
        const std::string& targetType,
        std::int64_t fromTime,
        std::int64_t untilTime,
        const std::string& locale = "de") const;

    GenreRecordingPage recordingsByGenre(
        const std::string& backendId,
        const std::string& genreId,
        int limit,
        int offset) const;

    GenreEpgPage epgByGenre(
        const std::string& backendId,
        const std::string& genreId,
        std::int64_t fromTime,
        std::int64_t untilTime,
        int limit,
        int offset) const;

    bool genreExists(const std::string& genreId) const;

    static std::string normalizeBackendId(const std::string& backendId);
    static std::string stableTargetId(
        const std::string& targetType,
        const std::string& backendId,
        const std::string& resourceKey);
    static bool isValidTargetType(const std::string& targetType);

private:
    Database& database_;
    mutable std::recursive_mutex mutex_;

    bool ensureSchemaLocked() const;
    bool replaceEvidenceLocked(const GenreEvidenceInput& input);
};
