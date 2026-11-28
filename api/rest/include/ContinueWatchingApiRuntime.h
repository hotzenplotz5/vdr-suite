#pragma once

#include "DashboardController.h"
#include "ContinueWatching.h"

#include <cstdint>
#include <functional>
#include <optional>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <vector>

class ContinueWatchingRepository;
class ContinueWatchingService;
class Database;
class RecentlyWatchedRepository;
class RecentlyWatchedService;
class VdrRecordingCacheRepository;
struct VdrRecording;
struct ManualRecordingMetadataAssignment;

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

    using PublicRecordingIdLookup = std::function<std::string(
        const std::string& backendId, const std::string& backendNativeId)>;

    using PublicHistoryArtworkLookup = std::function<std::string(
        const VdrRecording&, const ManualRecordingMetadataAssignment&)>;

    bool configure(Database& database, VdrRecordingCacheRepository& recordings,
                   PublicRecordingIdLookup publicRecordingIdLookup = {},
                   PublicHistoryArtworkLookup publicHistoryArtworkLookup = {});
    void reset();
    // Read-once SSE invalidation hints; persistent progress stays in the repository.
    std::vector<std::string> takePendingProgressBackends();

    static PublicRecordingProgressRequest parsePublicProgressRequest(const std::string& body);
    std::vector<ContinueWatchingItem> publicCandidates(
        const std::string& actorId, const std::string& backendId) const;
    ApiResponse publicContinueWatching(const std::string& actorId,
        const std::string& backendId,
        const std::function<std::optional<std::string>(const std::string&)>& admittedPublicId) const;
    ApiResponse publicRecentlyWatched(const std::string& actorId,
        const std::string& backendId,
        const std::function<std::optional<std::string>(const std::string&)>& admittedPublicId) const;
    ApiResponse publicProgressGet(
        const std::string& actorId,
        const VdrRecording& admittedRecording,
        const std::string& publicRecordingId) const;
    ApiResponse publicProgressUpdate(
        const std::string& actorId,
        const VdrRecording& admittedRecording,
        const std::string& publicRecordingId,
        const PublicRecordingProgressRequest& request) const;

    // Uses the already authorized Browser backend and the daemon's canonical
    // Recording resolver. Legacy callers keep their existing identity contract.
    using PublicRecordingResolver = std::function<bool(
        const std::string&, const std::string&, VdrRecording&)>;

    bool tryHandlePost(
        const std::string& requestTarget,
        const std::string& body,
        const std::string& actorRef,
        ApiResponse& response,
        const std::string& authorizedBackendId = {},
        const PublicRecordingResolver& resolvePublicRecording = {}) const;

private:
    ContinueWatchingApiRuntime() = default;

    mutable std::mutex mutex_;
    std::unique_ptr<ContinueWatchingRepository> repository_;
    std::unique_ptr<ContinueWatchingService> service_;
    std::unique_ptr<RecentlyWatchedRepository> recentlyWatchedRepository_;
    std::unique_ptr<RecentlyWatchedService> recentlyWatchedService_;
    VdrRecordingCacheRepository* recordings_ = nullptr;
    PublicRecordingIdLookup publicRecordingIdLookup_;
    PublicHistoryArtworkLookup publicHistoryArtworkLookup_;
    mutable std::set<std::string> pendingProgressBackends_;
};
