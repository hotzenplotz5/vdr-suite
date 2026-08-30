#pragma once

#include <functional>
#include <optional>
#include <string>
#include <vector>

class Database;

struct RecentlyWatchedState
{
    std::string actorId;
    std::string backendId;
    std::string recordingId;
    int positionSeconds = 0;
    bool positionKnown = false;
    bool completionKnown = false;
    bool completed = false;
    bool resumeRelevanceKnown = false;
    bool resumeRelevant = false;
    std::string sourceEvidence;
    std::string lastActivityAt;
    std::string lastOperationId;
};

struct RecentlyWatchedRecordingTruth
{
    std::string backendId;
    std::string recordingId;
    std::string backendNativeId;
    std::string title;
    std::string subtitle;
    std::string posterUrl;
    int durationSeconds = 0;
    bool durationKnown = false;
};

struct RecentlyWatchedItem
{
    RecentlyWatchedRecordingTruth recording;
    int positionSeconds = 0;
    bool positionKnown = false;
    bool completionKnown = false;
    bool completed = false;
    bool resumeRelevanceKnown = false;
    bool resumeRelevant = false;
    std::string sourceEvidence;
    std::string lastActivityAt;
};

class RecentlyWatchedRepository
{
public:
    static constexpr int MaxItemsPerActorBackend = 100;

    explicit RecentlyWatchedRepository(Database& database);

    bool ensureSchema();
    bool record(
        const RecentlyWatchedState& state);
    bool remove(
        const std::string& actorId,
        const std::string& backendId,
        const std::string& recordingId);
    std::vector<RecentlyWatchedState> findForActorBackend(
        const std::string& actorId,
        const std::string& backendId) const;

private:
    bool prune(
        const std::string& actorId,
        const std::string& backendId);

    Database& database_;
};

class RecentlyWatchedService
{
public:
    static constexpr const char* CanonicalPlaybackEvidence =
        "canonical-recording-playback-owner";

    using RecordingResolver = std::function<std::optional<RecentlyWatchedRecordingTruth>(
        const std::string& backendId,
        const std::string& recordingId)>;

    RecentlyWatchedService(
        RecentlyWatchedRepository& repository,
        RecordingResolver resolver);

    bool recordActivity(
        const std::string& actorId,
        const std::string& backendId,
        const std::string& recordingId,
        int positionSeconds,
        bool positionKnown,
        bool resumeSupportKnown,
        bool resumeSupported,
        bool ended,
        const std::string& operationId);
    std::vector<RecentlyWatchedItem> list(
        const std::string& actorId,
        const std::string& backendId);

private:
    RecentlyWatchedRepository& repository_;
    RecordingResolver resolver_;
};
