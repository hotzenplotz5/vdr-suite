#include "HbbtvMediaSessionRuntime.h"

#include "FfmpegLiveStreamCommandBuilder.h"
#include "MediaSessionRepository.h"
#include "MediaSessionWorkspace.h"

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <sys/wait.h>
#include <utility>

namespace
{
constexpr auto WorkerShutdownGrace =
    std::chrono::milliseconds(500);

bool contains(
    const std::vector<MediaDeliveryProtocol>& values,
    MediaDeliveryProtocol wanted)
{
    return std::find(values.begin(), values.end(), wanted) !=
        values.end();
}

bool contains(
    const std::vector<MediaContainer>& values,
    MediaContainer wanted)
{
    return std::find(values.begin(), values.end(), wanted) !=
        values.end();
}

bool contains(
    const std::vector<MediaCodec>& values,
    MediaCodec wanted)
{
    return std::find(values.begin(), values.end(), wanted) !=
        values.end();
}

bool browserSupportsContinuousFmp4(
    const ClientMediaCapabilities& capabilities)
{
    return contains(
            capabilities.protocols,
            MediaDeliveryProtocol::Progressive) &&
        contains(
            capabilities.containers,
            MediaContainer::Fmp4) &&
        contains(
            capabilities.videoCodecs,
            MediaCodec::H264) &&
        contains(
            capabilities.audioCodecs,
            MediaCodec::Aac) &&
        capabilities.maxAudioChannels >= 2;
}

MediaPresentationProfile hbbtvPresentation()
{
    MediaPresentationProfile profile;
    profile.available = true;
    profile.profileId = "live-progressive-fmp4";
    profile.protocol = MediaDeliveryProtocol::Progressive;
    profile.container = MediaContainer::Fmp4;
    profile.adaptationClass =
        MediaAdaptationClass::Transcode;
    profile.videoAction = MediaTrackAction::Transcode;
    profile.audioAction = MediaTrackAction::Transcode;
    profile.sourceVideoStreamIndex = 0;
    profile.sourceAudioStreamIndex = 0;
    profile.targetVideoCodec = MediaCodec::H264;
    profile.targetAudioCodec = MediaCodec::Aac;
    profile.targetAudioChannels = 2;
    profile.targetVideoWidth = 0;
    profile.targetVideoHeight = 0;
    profile.deinterlaceVideo = true;
    profile.videoTranscodeWorkload =
        MediaTranscodeWorkload::Deinterlace;
    profile.reason =
        "hbbtv_progressive_fmp4_continuous_adaptation_selected";
    return profile;
}

std::string transcodeReason(
    const MediaPresentationProfile& profile)
{
    if (profile.reason.find(
            "forced VAAPI does not support") !=
        std::string::npos)
        return "forced_vaapi_transformation_unsupported";
    if (profile.reason.find(
            "forced VAAPI is unavailable") !=
        std::string::npos)
        return "forced_vaapi_unavailable";
    return "hbbtv_media_transcode_capacity_unproven";
}
}

HbbtvMediaSessionRuntime::HbbtvMediaSessionRuntime(
    MediaSessionRepository& repository,
    std::string workspaceRoot,
    WorkerSpawner workerSpawner,
    WorkerTerminator workerTerminator,
    MediaTranscodePolicy transcodePolicy)
    : repository_(repository),
      workspaceRoot_(std::move(workspaceRoot)),
      workerSpawner_(std::move(workerSpawner)),
      workerTerminator_(std::move(workerTerminator)),
      transcodePolicy_(std::move(transcodePolicy))
{
    if (!workerSpawner_)
    {
        workerSpawner_ =
            [](const std::vector<std::string>& argv,
               const std::string& workingDirectory,
               const std::string& logPath) {
                return MediaProcessRunner().spawnLogged(
                    argv,
                    workingDirectory,
                    logPath);
            };
    }
    if (!workerTerminator_)
    {
        workerTerminator_ =
            [](pid_t pid, std::chrono::milliseconds grace) {
                return MediaProcessRunner().
                    terminateAndWait(pid, grace);
            };
    }
}

HbbtvMediaSessionRuntime::~HbbtvMediaSessionRuntime()
{
    stopAll();
}

HbbtvMediaSessionProvisionResult
HbbtvMediaSessionRuntime::provisionStream(
    const std::string& mediaSessionId,
    const std::string& workspaceId,
    const std::string& grantId,
    const std::string& hbbtvSessionId,
    const std::string& backendId,
    const HbbtvMediaSource& media,
    const ClientMediaCapabilities& clientCapabilities)
{
    HbbtvMediaSessionProvisionResult result;
    if (mediaSessionId.empty() ||
        workspaceId.empty() ||
        grantId.empty() ||
        hbbtvSessionId.empty() ||
        backendId.empty() ||
        !media.available ||
        media.mediaRevision == 0 ||
        media.unixSocketPath.empty() ||
        media.consumerConnected ||
        (media.state != HbbtvMediaSourceState::Streaming &&
         media.state != HbbtvMediaSourceState::Paused))
    {
        result.reasonCode =
            "invalid_hbbtv_media_stream_provision_request";
        repository_.failBundle(
            mediaSessionId,
            result.reasonCode);
        return result;
    }

    if (!browserSupportsContinuousFmp4(
            clientCapabilities))
    {
        result.reasonCode =
            "hbbtv_progressive_fmp4_unsupported";
        repository_.failBundle(
            mediaSessionId,
            result.reasonCode);
        return result;
    }

    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (active_.find(mediaSessionId) != active_.end())
        {
            result.reasonCode =
                "media_session_already_owned";
            return result;
        }
        for (const auto& entry : active_)
        {
            if (entry.second.hbbtvSessionId ==
                hbbtvSessionId)
            {
                result.reasonCode =
                    "hbbtv_media_application_session_already_owned";
                return result;
            }
        }
    }

    auto workspace =
        std::make_unique<MediaSessionWorkspace>(
            workspaceRoot_);
    const auto workspaceResult =
        workspace->prepareLive(workspaceId);
    if (!workspaceResult.ready)
    {
        result.reasonCode =
            workspaceResult.reasonCode.empty()
                ? "media_workspace_unavailable"
                : workspaceResult.reasonCode;
        repository_.failBundle(
            mediaSessionId,
            result.reasonCode);
        return result;
    }

    result.presentation =
        transcodePolicy_.apply(hbbtvPresentation());
    if (!result.presentation.available)
    {
        result.reasonCode =
            result.presentation.reason.empty()
                ? "hbbtv_media_transcode_capacity_unproven"
                : result.presentation.reason;
        repository_.failBundle(
            mediaSessionId,
            transcodeReason(result.presentation));
        return result;
    }

    const auto command =
        FfmpegLiveStreamCommandBuilder().build(
            result.presentation,
            media.unixSocketPath,
            workspace->liveStreamPath());
    if (!command.valid)
    {
        result.reasonCode =
            command.reasonCode.empty()
                ? "hbbtv_media_worker_plan_invalid"
                : command.reasonCode;
        repository_.failBundle(
            mediaSessionId,
            result.reasonCode);
        return result;
    }

    const pid_t pid = workerSpawner_(
        command.argv,
        workspace->directory(),
        workspace->logPath());
    if (pid <= 0)
    {
        result.reasonCode =
            "hbbtv_media_worker_start_failed";
        repository_.failBundle(
            mediaSessionId,
            result.reasonCode);
        return result;
    }

    int status = 0;
    const pid_t waited =
        ::waitpid(pid, &status, WNOHANG);
    if (waited == pid)
    {
        result.reasonCode =
            "hbbtv_media_worker_exited_during_start";
        repository_.failBundle(
            mediaSessionId,
            result.reasonCode);
        return result;
    }
    if (waited < 0 && errno != EINTR)
    {
        workerTerminator_(
            pid,
            WorkerShutdownGrace);
        result.reasonCode =
            "hbbtv_media_worker_wait_failed";
        repository_.failBundle(
            mediaSessionId,
            result.reasonCode);
        return result;
    }

    ActiveSession active;
    active.pid = pid;
    active.grantId = grantId;
    active.hbbtvSessionId = hbbtvSessionId;
    active.mediaRevision = media.mediaRevision;
    active.workspace = std::move(workspace);

    {
        std::lock_guard<std::mutex> lock(mutex_);
        const auto inserted =
            active_.emplace(
                mediaSessionId,
                std::move(active));
        if (!inserted.second)
        {
            workerTerminator_(
                pid,
                WorkerShutdownGrace);
            result.reasonCode =
                "media_session_already_owned";
            repository_.failBundle(
                mediaSessionId,
                result.reasonCode);
            return result;
        }
    }

    result.ready = true;
    result.workerPid = pid;
    result.reasonCode =
        "hbbtv_media_stream_worker_active";
    return result;
}

bool HbbtvMediaSessionRuntime::finishTaken(
    const std::string& mediaSessionId,
    ActiveSession&& active,
    const std::string& reasonCode,
    bool workerAlreadyExited)
{
    bool workerStopped = true;
    if (!workerAlreadyExited && active.pid > 0)
        workerStopped = workerTerminator_(
            active.pid,
            WorkerShutdownGrace);
    const bool bundleEnded =
        repository_.endBundle(
            mediaSessionId,
            reasonCode);
    return workerStopped && bundleEnded;
}

bool HbbtvMediaSessionRuntime::stop(
    const std::string& mediaSessionId,
    const std::string& reasonCode)
{
    if (mediaSessionId.empty() ||
        reasonCode.empty())
        return false;

    ActiveSession active;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        const auto found =
            active_.find(mediaSessionId);
        if (found == active_.end())
            return false;
        active = std::move(found->second);
        active_.erase(found);
    }
    return finishTaken(
        mediaSessionId,
        std::move(active),
        reasonCode,
        false);
}

bool HbbtvMediaSessionRuntime::stopForApplicationSession(
    const std::string& hbbtvSessionId,
    const std::string& reasonCode)
{
    const auto active =
        activeForApplicationSession(hbbtvSessionId);
    if (!active.has_value())
        return true;
    return stop(
        active->mediaSessionId,
        reasonCode);
}

std::size_t HbbtvMediaSessionRuntime::reapInactive(
    int idleTimeoutSeconds)
{
    if (idleTimeoutSeconds < 0 ||
        idleTimeoutSeconds > 86400)
        return 0;

    struct Candidate
    {
        std::string mediaSessionId;
        std::string grantId;
        pid_t pid = -1;
    };

    std::vector<Candidate> candidates;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        candidates.reserve(active_.size());
        for (const auto& entry : active_)
        {
            candidates.push_back(
                {entry.first,
                 entry.second.grantId,
                 entry.second.pid});
        }
    }

    std::size_t reaped = 0;
    for (const Candidate& candidate : candidates)
    {
        std::string reasonCode;
        bool workerAlreadyExited = false;

        int status = 0;
        const pid_t waited =
            ::waitpid(
                candidate.pid,
                &status,
                WNOHANG);
        if (waited == candidate.pid)
        {
            reasonCode =
                "hbbtv_media_worker_exited";
            workerAlreadyExited = true;
        }
        else if (waited < 0 && errno != EINTR)
        {
            reasonCode =
                "hbbtv_media_worker_wait_failed";
        }

        if (reasonCode.empty())
        {
            const auto grant =
                repository_.findResolvedGrant(
                    candidate.grantId,
                    idleTimeoutSeconds);
            if (!grant.has_value() ||
                grant->sessionId !=
                    candidate.mediaSessionId)
            {
                reasonCode =
                    "media_access_grant_missing";
            }
            else if (!grant->active ||
                     grant->revoked)
            {
                reasonCode =
                    "media_access_revoked";
            }
            else if (grant->expired)
            {
                reasonCode =
                    "media_access_expired";
            }
            else if (grant->idleExpired)
            {
                reasonCode =
                    "media_access_idle_expired";
            }
        }

        if (reasonCode.empty())
            continue;

        ActiveSession active;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            const auto found =
                active_.find(
                    candidate.mediaSessionId);
            if (found == active_.end())
                continue;
            active =
                std::move(found->second);
            active_.erase(found);
        }

        if (finishTaken(
                candidate.mediaSessionId,
                std::move(active),
                reasonCode,
                workerAlreadyExited))
        {
            ++reaped;
        }
    }
    return reaped;
}

void HbbtvMediaSessionRuntime::stopAll()
{
    std::map<std::string, ActiveSession> active;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        active.swap(active_);
    }
    for (auto& entry : active)
    {
        finishTaken(
            entry.first,
            std::move(entry.second),
            "daemon_shutdown",
            false);
    }
}

std::optional<HbbtvMediaSessionActive>
HbbtvMediaSessionRuntime::activeForApplicationSession(
    const std::string& hbbtvSessionId) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& entry : active_)
    {
        if (entry.second.hbbtvSessionId ==
            hbbtvSessionId)
        {
            return HbbtvMediaSessionActive{
                entry.first,
                entry.second.mediaRevision};
        }
    }
    return std::nullopt;
}

std::size_t HbbtvMediaSessionRuntime::activeCount() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return active_.size();
}
