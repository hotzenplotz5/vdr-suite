#pragma once

#include <functional>
#include <optional>
#include <string>
#include <vector>

class Database;

struct ContinueWatchingState
{
    std::string actorId;
    std::string backendId;
    std::string recordingId;
    int positionSeconds = 0;
    std::string lastActivityAt;
    std::string lastOperationId;
};

struct ContinueWatchingRecordingTruth
{
    std::string backendId;
    std::string recordingId;
    std::string title;
    std::string subtitle;
    std::string posterUrl;
    int durationSeconds = 0;
    bool durationKnown = false;
};

struct ContinueWatchingItem
{
    ContinueWatchingRecordingTruth recording;
    int resumePositionSeconds = 0;
    std::string lastActivityAt;
};

class ContinueWatchingRepository
{
public:
    explicit ContinueWatchingRepository(Database& database);

    bool ensureSchema();
    bool upsert(
        const std::string& actorId,
        const std::string& backendId,
        const std::string& recordingId,
        int positionSeconds,
        const std::string& operationId);
    bool clear(
        const std::string& actorId,
        const std::string& backendId,
        const std::string& recordingId);
    std::vector<ContinueWatchingState> findForActorBackend(
        const std::string& actorId,
        const std::string& backendId) const;

private:
    Database& database_;
};

class ContinueWatchingService
{
public:
    using RecordingResolver = std::function<std::optional<ContinueWatchingRecordingTruth>(
        const std::string& backendId,
        const std::string& recordingId)>;

    ContinueWatchingService(
        ContinueWatchingRepository& repository,
        RecordingResolver resolver);

    bool recordProgress(
        const std::string& actorId,
        const std::string& backendId,
        const std::string& recordingId,
        int positionSeconds,
        bool resumeSupported,
        const std::string& operationId);
    bool clear(
        const std::string& actorId,
        const std::string& backendId,
        const std::string& recordingId,
        const std::string& operationId = std::string());
    std::vector<ContinueWatchingItem> list(
        const std::string& actorId,
        const std::string& backendId);

private:
    ContinueWatchingRepository& repository_;
    RecordingResolver resolver_;
};
