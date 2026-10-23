#pragma once

#include "DashboardController.h"

#include <cstdint>
#include <memory>
#include <mutex>
#include <string>

class ContinueWatchingRepository;
class ContinueWatchingService;
class Database;
class RecentlyWatchedRepository;
class RecentlyWatchedService;
class VdrRecordingCacheRepository;
struct VdrRecording;

// Parsed but not authorized; the caller must verify the live Device credential,
// exact backend play grant and public rec_ identity before any persistence access.
struct PublicRecordingProgressRequest
{
    std::string backendId;
    std::string recordingId;
    std::string operationId;
    std::string sessionId; // active owned Device Session for nonzero progress
    int positionSeconds = -1;
    std::int64_t expectedRevision = -1;
    bool valid = false;
};

class ContinueWatchingApiRuntime
{
public:
    static ContinueWatchingApiRuntime& instance();

    bool configure(Database& database, VdrRecordingCacheRepository& recordings);
    void reset();

    static PublicRecordingProgressRequest parsePublicProgressRequest(const std::string& body);
    ApiResponse publicProgressGet(
        const std::string& actorId,
        const VdrRecording& admittedRecording,
        const std::string& publicRecordingId) const;
    ApiResponse publicProgressUpdate(
        const std::string& actorId,
        const VdrRecording& admittedRecording,
        const std::string& publicRecordingId,
        const PublicRecordingProgressRequest& request) const;

    bool tryHandlePost(
        const std::string& requestTarget,
        const std::string& body,
        const std::string& actorRef,
        ApiResponse& response) const;

private:
    ContinueWatchingApiRuntime() = default;

    mutable std::mutex mutex_;
    std::unique_ptr<ContinueWatchingRepository> repository_;
    std::unique_ptr<ContinueWatchingService> service_;
    std::unique_ptr<RecentlyWatchedRepository> recentlyWatchedRepository_;
    std::unique_ptr<RecentlyWatchedService> recentlyWatchedService_;
    VdrRecordingCacheRepository* recordings_ = nullptr;
};
