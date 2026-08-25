#pragma once

#include "MediaCapabilities.h"
#include "MediaTranscodePolicy.h"
#include "RecordingDirectSourceRegistry.h"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <sys/types.h>
#include <vector>

class MediaSessionRepository;
class MediaSessionWorkspace;

struct RecordingMediaSessionProvisionResult
{
    bool ready = false;
    std::string reasonCode;
};

struct RecordingMediaSessionSeekResult
{
    bool repositioned = false;
    std::string reasonCode;
    int positionSeconds = 0;
    int durationSeconds = 0;
};

struct RecordingMediaSessionSeekCapabilityResult
{
    bool enabled = false;
    std::string reasonCode;
    int durationSeconds = 0;
};

struct RecordingMediaSessionAudioTrackSelectionResult
{
    bool selected = false;
    bool restarted = false;
    std::string reasonCode;
    int sourceAudioStreamIndex = -1;
    int positionSeconds = 0;
    int durationSeconds = 0;
};

struct RecordingMediaSessionSubtitleWebVttResult
{
    bool ready = false;
    std::string reasonCode;
    int sourceSubtitleStreamIndex = -1;
    std::string webVtt;
};

struct RecordingMediaSessionTrackState
{
    bool available = false;
    bool audioSelectionSupported = false;
    std::string reasonCode;
    std::string profileId;
    int sourceAudioStreamIndex = -1;
    int durationSeconds = 0;
};

class RecordingMediaSessionRuntime
{
public:
    using WorkerSpawner = std::function<pid_t(
        const std::vector<std::string>&,
        const std::string&,
        const std::string&)>;
    using WorkerTerminator = std::function<bool(
        pid_t,
        std::chrono::milliseconds)>;
    using ReadinessProbe = std::function<bool(
        const std::string&,
        MediaContainer)>;

    RecordingMediaSessionRuntime(
        MediaSessionRepository& repository,
        std::string workspaceRoot,
        WorkerSpawner workerSpawner = {},
        WorkerTerminator workerTerminator = {},
        ReadinessProbe readinessProbe = {});

    RecordingMediaSessionRuntime(
        MediaSessionRepository& repository,
        std::string workspaceRoot,
        RecordingDirectSourceRegistry& directSourceRegistry);

    RecordingMediaSessionRuntime(
        MediaSessionRepository& repository,
        std::string workspaceRoot,
        WorkerSpawner workerSpawner,
        WorkerTerminator workerTerminator,
        ReadinessProbe readinessProbe,
        MediaTranscodePolicy transcodePolicy);

    ~RecordingMediaSessionRuntime();

    RecordingMediaSessionRuntime(const RecordingMediaSessionRuntime&) = delete;
    RecordingMediaSessionRuntime& operator=(const RecordingMediaSessionRuntime&) = delete;

    RecordingMediaSessionProvisionResult provisionHls(
        const std::string& sessionId,
        const std::string& workspaceId,
        const std::string& grantId,
        const MediaPresentationProfile& profile,
        const std::vector<std::string>& sourceSegments);

    RecordingMediaSessionProvisionResult provisionHlsAt(
        const std::string& sessionId,
        const std::string& workspaceId,
        const std::string& grantId,
        const MediaPresentationProfile& profile,
        const std::vector<std::string>& sourceSegments,
        int startPositionSeconds,
        const std::vector<double>& segmentDurationsSeconds);

    RecordingMediaSessionProvisionResult provisionStream(
        const std::string& sessionId,
        const std::string& workspaceId,
        const std::string& grantId,
        const MediaPresentationProfile& profile,
        const std::vector<std::string>& sourceSegments,
        int durationSeconds = 0,
        const std::vector<double>& segmentDurationsSeconds = {});

    RecordingMediaSessionProvisionResult provisionDirect(
        const std::string& sessionId,
        const std::string& grantId,
        const MediaPresentationProfile& profile,
        const RecordingDirectSourceRegistration& registration);

    RecordingMediaSessionSeekCapabilityResult enableIndexedSeek(
        const std::string& sessionId,
        int durationSeconds,
        const std::vector<std::string>& sourceSegments,
        const std::vector<double>& segmentDurationsSeconds);

    RecordingMediaSessionSeekResult seekStream(
        const std::string& sessionId,
        int positionSeconds);

    RecordingMediaSessionAudioTrackSelectionResult selectAudioTrack(
        const std::string& sessionId,
        const MediaPresentationProfile& profile,
        int positionSeconds);

    RecordingMediaSessionSubtitleWebVttResult subtitleWebVtt(
        const std::string& sessionId,
        int sourceSubtitleStreamIndex,
        MediaSubtitleFormat format);

    RecordingMediaSessionTrackState trackState(
        const std::string& sessionId) const;

    bool stop(
        const std::string& sessionId,
        const std::string& reasonCode);

    std::size_t reapInactive(int idleTimeoutSeconds);

    void stopAll();

private:
    struct ActiveSession
    {
        pid_t pid = -1;
        std::string grantId;
        std::unique_ptr<MediaSessionWorkspace> workspace;
        bool direct = false;
        bool continuousStream = false;
        bool indexedSeekTimeline = false;
        int durationSeconds = 0;
        std::uint64_t streamGeneration = 0;
        MediaPresentationProfile streamProfile;
        std::map<int, std::string> subtitleWebVttCache;
    };

    static bool defaultReady(
        const std::string& workspaceDirectory,
        MediaContainer container);

    MediaSessionRepository& repository_;
    std::string workspaceRoot_;
    WorkerSpawner workerSpawner_;
    WorkerTerminator workerTerminator_;
    ReadinessProbe readinessProbe_;
    MediaTranscodePolicy transcodePolicy_;
    RecordingDirectSourceRegistry* directSourceRegistry_ = nullptr;
    mutable std::mutex mutex_;
    std::map<std::string, ActiveSession> active_;
};
