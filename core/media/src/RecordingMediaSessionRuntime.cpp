#include "RecordingMediaSessionRuntime.h"

#include "FfmpegHlsCommandBuilder.h"
#include "MediaProcessRunner.h"
#include "MediaSessionRepository.h"
#include "MediaSessionWorkspace.h"

#include <cerrno>
#include <chrono>
#include <filesystem>
#include <string>
#include <sys/stat.h>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>
#include <utility>

namespace
{

constexpr auto HlsReadinessTimeout = std::chrono::seconds(15);
constexpr auto HlsReadinessPollInterval = std::chrono::milliseconds(50);
constexpr auto WorkerShutdownGrace = std::chrono::milliseconds(500);
constexpr const char* RecordingStreamName = "recording.fmp4";

bool nonEmptyFile(const std::filesystem::path& path)
{
    std::error_code error;
    if (!std::filesystem::is_regular_file(path, error) || error) {
        return false;
    }
    const auto size = std::filesystem::file_size(path, error);
    return !error && size > 0;
}

bool prepareRecordingStreamPipe(
    const std::string& workspaceDirectory,
    bool replaceExisting)
{
    const std::filesystem::path streamPath =
        std::filesystem::path(workspaceDirectory) / RecordingStreamName;

    struct stat status {};
    if (::lstat(streamPath.c_str(), &status) == 0) {
        if (!replaceExisting || !S_ISFIFO(status.st_mode)) {
            return false;
        }
        if (::unlink(streamPath.c_str()) != 0) {
            return false;
        }
    }
    else if (errno != ENOENT) {
        return false;
    }

    return ::mkfifo(streamPath.c_str(), 0600) == 0;
}

struct ProgressiveStreamPlan
{
    bool valid = false;
    std::string reasonCode;
    std::vector<std::string> argv;
};

const char* x264PresetName(MediaSoftwareEncoderPreset preset)
{
    switch (preset)
    {
    case MediaSoftwareEncoderPreset::Superfast: return "superfast";
    case MediaSoftwareEncoderPreset::Veryfast: return "veryfast";
    case MediaSoftwareEncoderPreset::Faster: return "faster";
    case MediaSoftwareEncoderPreset::Fast: return "fast";
    }
    return nullptr;
}

bool validTargetVideoSize(const MediaPresentationProfile& profile)
{
    return profile.targetVideoWidth >= 2 && profile.targetVideoHeight >= 2 &&
        profile.targetVideoWidth <= 16384 && profile.targetVideoHeight <= 16384 &&
        profile.targetVideoWidth % 2 == 0 && profile.targetVideoHeight % 2 == 0;
}

bool validVaapiDevice(const std::string& value)
{
    return !value.empty() && value.size() <= 512 &&
        value.rfind("/dev/dri/", 0) == 0;
}

bool usesVaapiInput(const MediaPresentationProfile& profile)
{
    return profile.videoAction == MediaTrackAction::Transcode &&
        profile.videoEncoderBackend == MediaVideoEncoderBackend::Vaapi;
}

bool appendSelectedTrackMaps(
    std::vector<std::string>& argv,
    const MediaPresentationProfile& profile)
{
    if (profile.videoAction != MediaTrackAction::Omit) {
        if (profile.sourceVideoStreamIndex < 0) return false;
        argv.push_back("-map");
        argv.push_back("0:v:" + std::to_string(profile.sourceVideoStreamIndex) + "?");
    }
    if (profile.audioAction != MediaTrackAction::Omit) {
        if (profile.sourceAudioStreamIndex < 0) return false;
        argv.push_back("-map");
        argv.push_back("0:a:" + std::to_string(profile.sourceAudioStreamIndex) + "?");
    }
    return true;
}

bool appendProgressiveVideoPlan(
    std::vector<std::string>& argv,
    const MediaPresentationProfile& profile)
{
    switch (profile.videoAction) {
    case MediaTrackAction::Omit:
        argv.push_back("-vn");
        return true;
    case MediaTrackAction::Copy:
        if (profile.targetVideoCodec != MediaCodec::H264 &&
            profile.targetVideoCodec != MediaCodec::H265) {
            return false;
        }
        argv.insert(argv.end(), {"-c:v", "copy"});
        if (profile.targetVideoCodec == MediaCodec::H264) {
            argv.insert(argv.end(), {"-bsf:v", "h264_metadata=level=auto"});
        }
        return true;
    case MediaTrackAction::Transcode:
        break;
    }

    if (profile.targetVideoCodec != MediaCodec::H264 ||
        !validTargetVideoSize(profile)) {
        return false;
    }

    if (profile.videoEncoderBackend == MediaVideoEncoderBackend::SoftwareX264) {
        const char* preset = x264PresetName(profile.videoEncoderPreset);
        if (preset == nullptr) return false;
        argv.insert(argv.end(), {
            "-c:v", "libx264",
            "-preset", preset,
            "-tune", "zerolatency",
            "-crf", "20",
            "-vf"
        });
        if (profile.deinterlaceVideo) {
            argv.push_back(
                "bwdif=mode=send_frame:parity=auto:deint=all,scale=" +
                std::to_string(profile.targetVideoWidth) + ":" +
                std::to_string(profile.targetVideoHeight));
        }
        else {
            argv.push_back(
                "scale=" + std::to_string(profile.targetVideoWidth) + ":" +
                std::to_string(profile.targetVideoHeight));
        }
        argv.insert(argv.end(), {"-pix_fmt", "yuv420p"});
        return true;
    }

    if (profile.videoEncoderBackend == MediaVideoEncoderBackend::Vaapi) {
        if (profile.deinterlaceVideo || !validVaapiDevice(profile.videoHardwareDevice)) {
            return false;
        }
        argv.insert(argv.end(), {
            "-c:v", "h264_vaapi",
            "-qp", "22",
            "-vf",
            "scale_vaapi=w=" + std::to_string(profile.targetVideoWidth) +
                ":h=" + std::to_string(profile.targetVideoHeight) + ":format=nv12"
        });
        return true;
    }

    return false;
}

bool appendProgressiveAudioPlan(
    std::vector<std::string>& argv,
    const MediaPresentationProfile& profile)
{
    switch (profile.audioAction) {
    case MediaTrackAction::Omit:
        argv.push_back("-an");
        return true;
    case MediaTrackAction::Copy:
        if (profile.targetAudioCodec != MediaCodec::Aac &&
            profile.targetAudioCodec != MediaCodec::Ac3 &&
            profile.targetAudioCodec != MediaCodec::Eac3) {
            return false;
        }
        argv.insert(argv.end(), {"-c:a", "copy"});
        if (profile.targetAudioCodec == MediaCodec::Aac) {
            argv.insert(argv.end(), {"-bsf:a", "aac_adtstoasc"});
        }
        return true;
    case MediaTrackAction::Transcode:
        if (profile.targetAudioCodec != MediaCodec::Aac ||
            profile.targetAudioChannels < 0 || profile.targetAudioChannels > 32) {
            return false;
        }
        argv.insert(argv.end(), {"-c:a", "aac", "-b:a", "192k"});
        if (profile.targetAudioChannels > 0) {
            argv.push_back("-ac");
            argv.push_back(std::to_string(profile.targetAudioChannels));
        }
        return true;
    }
    return false;
}

ProgressiveStreamPlan buildProgressiveStreamPlan(
    const MediaPresentationProfile& profile,
    int startPositionSeconds = 0)
{
    ProgressiveStreamPlan plan;
    if (startPositionSeconds < 0 || !profile.available ||
        profile.profileId != "progressive-fmp4" ||
        profile.protocol != MediaDeliveryProtocol::Progressive ||
        profile.container != MediaContainer::Fmp4 ||
        profile.adaptationClass == MediaAdaptationClass::PassThrough) {
        plan.reasonCode = "profile_is_not_recording_progressive_fmp4";
        return plan;
    }

    plan.argv = {
        "/usr/bin/ffmpeg",
        "-nostdin",
        "-hide_banner",
        "-loglevel", "warning",
        "-y"
    };

    if (usesVaapiInput(profile)) {
        if (!validVaapiDevice(profile.videoHardwareDevice)) {
            plan.reasonCode = "unsupported_recording_video_transformation";
            return plan;
        }
        plan.argv.insert(plan.argv.end(), {
            "-init_hw_device", "vaapi=va:" + profile.videoHardwareDevice,
            "-filter_hw_device", "va",
            "-hwaccel", "vaapi",
            "-hwaccel_device", "va",
            "-hwaccel_output_format", "vaapi"
        });
    }

    // Non-zero seeks are allowed only for a workspace whose ffconcat carries
    // VDR-index-derived duration directives for every source segment. Place
    // -ss before the concat input so FFmpeg can seek the indexed timeline
    // instead of decoding/discarding the recording from time zero.
    if (startPositionSeconds > 0) {
        plan.argv.push_back("-ss");
        plan.argv.push_back(std::to_string(startPositionSeconds));
    }

    // Deliberately no -re here. The browser-facing FIFO supplies natural
    // backpressure, so a completed recording can produce the first fMP4 bytes
    // as quickly as storage/demux/remux allows without running away from the
    // consumer or creating an HLS deletion window.
    plan.argv.insert(plan.argv.end(), {
        "-f", "concat",
        "-safe", "1",
        "-i", "input.ffconcat"
    });

    if (!appendSelectedTrackMaps(plan.argv, profile)) {
        plan.reasonCode = "selected_source_track_missing";
        return plan;
    }
    plan.argv.push_back("-sn");

    if (!appendProgressiveVideoPlan(plan.argv, profile)) {
        plan.reasonCode = "unsupported_recording_video_transformation";
        return plan;
    }
    if (!appendProgressiveAudioPlan(plan.argv, profile)) {
        plan.reasonCode = "unsupported_recording_audio_transformation";
        return plan;
    }

    plan.argv.insert(plan.argv.end(), {
        "-f", "mp4",
        "-movflags", "+empty_moov+default_base_moof+frag_keyframe+omit_tfhd_offset",
        "-frag_duration", "250000",
        "-min_frag_duration", "100000",
        "-flush_packets", "1",
        RecordingStreamName
    });
    plan.valid = true;
    return plan;
}

} // namespace

RecordingMediaSessionRuntime::RecordingMediaSessionRuntime(
    MediaSessionRepository& repository,
    std::string workspaceRoot,
    WorkerSpawner workerSpawner,
    WorkerTerminator workerTerminator,
    ReadinessProbe readinessProbe)
    : RecordingMediaSessionRuntime(
          repository,
          std::move(workspaceRoot),
          std::move(workerSpawner),
          std::move(workerTerminator),
          std::move(readinessProbe),
          MediaTranscodePolicy::fromEnvironment())
{
}

RecordingMediaSessionRuntime::RecordingMediaSessionRuntime(
    MediaSessionRepository& repository,
    std::string workspaceRoot,
    RecordingDirectSourceRegistry& directSourceRegistry)
    : RecordingMediaSessionRuntime(
          repository,
          std::move(workspaceRoot))
{
    directSourceRegistry_ = &directSourceRegistry;
}

RecordingMediaSessionRuntime::RecordingMediaSessionRuntime(
    MediaSessionRepository& repository,
    std::string workspaceRoot,
    WorkerSpawner workerSpawner,
    WorkerTerminator workerTerminator,
    ReadinessProbe readinessProbe,
    MediaTranscodePolicy transcodePolicy)
    : repository_(repository),
      workspaceRoot_(std::move(workspaceRoot)),
      workerSpawner_(std::move(workerSpawner)),
      workerTerminator_(std::move(workerTerminator)),
      readinessProbe_(std::move(readinessProbe)),
      transcodePolicy_(std::move(transcodePolicy))
{
    if (!workerSpawner_) {
        workerSpawner_ = [](const std::vector<std::string>& argv,
                            const std::string& workingDirectory,
                            const std::string& logPath) {
            return MediaProcessRunner().spawnLogged(argv, workingDirectory, logPath);
        };
    }
    if (!workerTerminator_) {
        workerTerminator_ = [](pid_t pid, std::chrono::milliseconds gracePeriod) {
            return MediaProcessRunner().terminateAndWait(pid, gracePeriod);
        };
    }
    if (!readinessProbe_) {
        readinessProbe_ = &RecordingMediaSessionRuntime::defaultReady;
    }
}

RecordingMediaSessionRuntime::~RecordingMediaSessionRuntime()
{
    stopAll();
}

bool RecordingMediaSessionRuntime::defaultReady(
    const std::string& workspaceDirectory,
    MediaContainer container)
{
    const std::filesystem::path root(workspaceDirectory);
    if (!root.is_absolute() || !nonEmptyFile(root / "master.m3u8")) {
        return false;
    }

    if (container == MediaContainer::Fmp4) {
        return nonEmptyFile(root / "init.mp4") &&
            nonEmptyFile(root / "segment-000000.m4s");
    }
    if (container == MediaContainer::MpegTs) {
        return nonEmptyFile(root / "segment-000000.ts");
    }
    return false;
}

RecordingMediaSessionProvisionResult RecordingMediaSessionRuntime::provisionHls(
    const std::string& sessionId,
    const std::string& workspaceId,
    const std::string& grantId,
    const MediaPresentationProfile& profile,
    const std::vector<std::string>& sourceSegments)
{
    RecordingMediaSessionProvisionResult result;
    if (sessionId.empty() || workspaceId.empty() || grantId.empty() ||
        !profile.available || profile.protocol != MediaDeliveryProtocol::Hls ||
        (profile.container != MediaContainer::Fmp4 &&
         profile.container != MediaContainer::MpegTs)) {
        result.reasonCode = "invalid_hls_provision_request";
        return result;
    }

    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (active_.find(sessionId) != active_.end()) {
            result.reasonCode = "media_session_already_owned";
            return result;
        }
    }

    auto workspace = std::make_unique<MediaSessionWorkspace>(workspaceRoot_);
    const MediaSessionWorkspaceResult workspaceResult =
        workspace->prepare(workspaceId, sourceSegments);
    if (!workspaceResult.ready) {
        result.reasonCode = workspaceResult.reasonCode.empty()
            ? "media_workspace_unavailable"
            : workspaceResult.reasonCode;
        repository_.failBundle(sessionId, result.reasonCode);
        return result;
    }

    const MediaPresentationProfile resolvedProfile = transcodePolicy_.apply(profile);
    const FfmpegHlsCommandPlan command =
        FfmpegHlsCommandBuilder().build(resolvedProfile);
    if (!command.valid) {
        result.reasonCode = command.reasonCode.empty()
            ? "media_worker_plan_invalid"
            : command.reasonCode;
        repository_.failBundle(sessionId, result.reasonCode);
        return result;
    }

    const pid_t pid = workerSpawner_(
        command.argv,
        workspace->directory(),
        workspace->logPath());
    if (pid <= 0) {
        result.reasonCode = "media_worker_start_failed";
        repository_.failBundle(sessionId, result.reasonCode);
        return result;
    }

    const auto deadline = std::chrono::steady_clock::now() + HlsReadinessTimeout;
    while (std::chrono::steady_clock::now() < deadline) {
        if (readinessProbe_(workspace->directory(), resolvedProfile.container)) {
            ActiveSession active;
            active.pid = pid;
            active.grantId = grantId;
            active.workspace = std::move(workspace);
            {
                std::lock_guard<std::mutex> lock(mutex_);
                const auto inserted = active_.emplace(sessionId, std::move(active));
                if (!inserted.second) {
                    workerTerminator_(pid, WorkerShutdownGrace);
                    repository_.failBundle(sessionId, "media_session_already_owned");
                    result.reasonCode = "media_session_already_owned";
                    return result;
                }
            }

            if (!repository_.activateBundle(sessionId)) {
                ActiveSession failed;
                {
                    std::lock_guard<std::mutex> lock(mutex_);
                    auto found = active_.find(sessionId);
                    if (found != active_.end()) {
                        failed = std::move(found->second);
                        active_.erase(found);
                    }
                }
                if (failed.pid > 0) {
                    workerTerminator_(failed.pid, WorkerShutdownGrace);
                }
                repository_.failBundle(sessionId, "media_session_activation_failed");
                result.reasonCode = "media_session_activation_failed";
                return result;
            }

            result.ready = true;
            return result;
        }

        int status = 0;
        const pid_t waited = ::waitpid(pid, &status, WNOHANG);
        if (waited == pid) {
            result.reasonCode = "media_worker_exited_before_ready";
            repository_.failBundle(sessionId, result.reasonCode);
            return result;
        }
        if (waited < 0 && errno != EINTR) {
            result.reasonCode = "media_worker_wait_failed";
            workerTerminator_(pid, WorkerShutdownGrace);
            repository_.failBundle(sessionId, result.reasonCode);
            return result;
        }

        std::this_thread::sleep_for(HlsReadinessPollInterval);
    }

    workerTerminator_(pid, WorkerShutdownGrace);
    result.reasonCode = "media_hls_not_ready";
    repository_.failBundle(sessionId, result.reasonCode);
    return result;
}

RecordingMediaSessionProvisionResult RecordingMediaSessionRuntime::provisionStream(
    const std::string& sessionId,
    const std::string& workspaceId,
    const std::string& grantId,
    const MediaPresentationProfile& profile,
    const std::vector<std::string>& sourceSegments,
    int durationSeconds,
    const std::vector<double>& segmentDurationsSeconds)
{
    RecordingMediaSessionProvisionResult result;
    if (sessionId.empty() || workspaceId.empty() || grantId.empty() ||
        sourceSegments.empty() || durationSeconds < 0 || !profile.available ||
        profile.profileId != "progressive-fmp4" ||
        profile.protocol != MediaDeliveryProtocol::Progressive ||
        profile.container != MediaContainer::Fmp4 ||
        profile.adaptationClass == MediaAdaptationClass::PassThrough) {
        result.reasonCode = "invalid_recording_stream_provision_request";
        return result;
    }

    const bool indexedSeekTimeline =
        durationSeconds > 0 &&
        segmentDurationsSeconds.size() == sourceSegments.size();

    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (active_.find(sessionId) != active_.end()) {
            result.reasonCode = "media_session_already_owned";
            return result;
        }
    }

    auto workspace = std::make_unique<MediaSessionWorkspace>(workspaceRoot_);
    const MediaSessionWorkspaceResult workspaceResult =
        workspace->prepare(
            workspaceId,
            sourceSegments,
            indexedSeekTimeline
                ? segmentDurationsSeconds
                : std::vector<double>{});
    if (!workspaceResult.ready) {
        result.reasonCode = workspaceResult.reasonCode.empty()
            ? "media_workspace_unavailable"
            : workspaceResult.reasonCode;
        repository_.failBundle(sessionId, result.reasonCode);
        return result;
    }

    if (!prepareRecordingStreamPipe(workspace->directory(), false)) {
        result.reasonCode = "recording_stream_pipe_create_failed";
        repository_.failBundle(sessionId, result.reasonCode);
        return result;
    }

    const MediaPresentationProfile resolvedProfile = transcodePolicy_.apply(profile);
    if (!resolvedProfile.available) {
        result.reasonCode = resolvedProfile.reason.empty()
            ? "media_transcode_capacity_unproven"
            : resolvedProfile.reason;
        repository_.failBundle(sessionId, result.reasonCode);
        return result;
    }

    const ProgressiveStreamPlan command =
        buildProgressiveStreamPlan(resolvedProfile);
    if (!command.valid) {
        result.reasonCode = command.reasonCode.empty()
            ? "media_worker_plan_invalid"
            : command.reasonCode;
        repository_.failBundle(sessionId, result.reasonCode);
        return result;
    }

    const pid_t pid = workerSpawner_(
        command.argv,
        workspace->directory(),
        workspace->logPath());
    if (pid <= 0) {
        result.reasonCode = "media_worker_start_failed";
        repository_.failBundle(sessionId, result.reasonCode);
        return result;
    }

    ActiveSession active;
    active.pid = pid;
    active.grantId = grantId;
    active.workspace = std::move(workspace);
    active.continuousStream = true;
    active.indexedSeekTimeline = indexedSeekTimeline;
    active.durationSeconds = indexedSeekTimeline ? durationSeconds : 0;
    active.streamGeneration = 1;
    active.streamProfile = resolvedProfile;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        const auto inserted = active_.emplace(sessionId, std::move(active));
        if (!inserted.second) {
            workerTerminator_(pid, WorkerShutdownGrace);
            repository_.failBundle(sessionId, "media_session_already_owned");
            result.reasonCode = "media_session_already_owned";
            return result;
        }
    }

    if (!repository_.activateBundle(sessionId)) {
        ActiveSession failed;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            auto found = active_.find(sessionId);
            if (found != active_.end()) {
                failed = std::move(found->second);
                active_.erase(found);
            }
        }
        if (failed.pid > 0) {
            workerTerminator_(failed.pid, WorkerShutdownGrace);
        }
        repository_.failBundle(sessionId, "media_session_activation_failed");
        result.reasonCode = "media_session_activation_failed";
        return result;
    }

    result.ready = true;
    return result;
}

RecordingMediaSessionProvisionResult RecordingMediaSessionRuntime::provisionDirect(
    const std::string& sessionId,
    const std::string& grantId,
    const MediaPresentationProfile& profile,
    const RecordingDirectSourceRegistration& registration)
{
    RecordingMediaSessionProvisionResult result;
    if (directSourceRegistry_ == nullptr || sessionId.empty() || grantId.empty() ||
        !profile.available || profile.protocol != MediaDeliveryProtocol::Progressive ||
        profile.container != MediaContainer::MpegTs ||
        profile.adaptationClass != MediaAdaptationClass::PassThrough ||
        registration.readableBytes == 0) {
        result.reasonCode = "invalid_direct_provision_request";
        return result;
    }

    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (active_.find(sessionId) != active_.end()) {
            result.reasonCode = "media_session_already_owned";
            return result;
        }
    }

    std::string registrationFailure;
    if (!directSourceRegistry_->registerCompleted(
            sessionId,
            registration,
            registrationFailure)) {
        result.reasonCode = registrationFailure.empty()
            ? "recording_direct_source_unavailable"
            : registrationFailure;
        repository_.failBundle(sessionId, result.reasonCode);
        return result;
    }

    ActiveSession active;
    active.grantId = grantId;
    active.direct = true;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        const auto inserted = active_.emplace(sessionId, std::move(active));
        if (!inserted.second) {
            directSourceRegistry_->remove(sessionId);
            repository_.failBundle(sessionId, "media_session_already_owned");
            result.reasonCode = "media_session_already_owned";
            return result;
        }
    }

    if (!repository_.activateBundle(sessionId)) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            active_.erase(sessionId);
        }
        directSourceRegistry_->remove(sessionId);
        repository_.failBundle(sessionId, "media_session_activation_failed");
        result.reasonCode = "media_session_activation_failed";
        return result;
    }

    result.ready = true;
    return result;
}

RecordingMediaSessionSeekResult RecordingMediaSessionRuntime::seekStream(
    const std::string& sessionId,
    int positionSeconds)
{
    RecordingMediaSessionSeekResult result;
    if (sessionId.empty() || positionSeconds < 0) {
        result.reasonCode = "invalid_recording_seek_request";
        return result;
    }

    ActiveSession failed;
    std::string failureReason;
    {
        std::unique_lock<std::mutex> lock(mutex_);
        const auto found = active_.find(sessionId);
        if (found == active_.end()) {
            result.reasonCode = "recording_seek_runtime_not_found";
            return result;
        }

        ActiveSession& active = found->second;
        if (!active.continuousStream || !active.workspace ||
            !active.indexedSeekTimeline ||
            active.streamProfile.profileId != "progressive-fmp4" ||
            active.durationSeconds <= 0) {
            result.reasonCode = "recording_seek_not_supported";
            return result;
        }
        if (positionSeconds >= active.durationSeconds) {
            result.reasonCode = "recording_seek_outside_window";
            result.durationSeconds = active.durationSeconds;
            return result;
        }

        const ProgressiveStreamPlan command =
            buildProgressiveStreamPlan(active.streamProfile, positionSeconds);
        if (!command.valid) {
            result.reasonCode = command.reasonCode.empty()
                ? "recording_seek_worker_plan_invalid"
                : command.reasonCode;
            return result;
        }

        if (active.pid > 0 &&
            !workerTerminator_(active.pid, WorkerShutdownGrace)) {
            result.reasonCode = "recording_seek_worker_stop_failed";
            return result;
        }
        active.pid = -1;

        if (!prepareRecordingStreamPipe(active.workspace->directory(), true)) {
            failureReason = "recording_seek_pipe_reset_failed";
            failed = std::move(active);
            active_.erase(found);
        }
        else {
            const pid_t pid = workerSpawner_(
                command.argv,
                active.workspace->directory(),
                active.workspace->logPath());
            if (pid <= 0) {
                failureReason = "recording_seek_worker_start_failed";
                failed = std::move(active);
                active_.erase(found);
            }
            else {
                active.pid = pid;
                ++active.streamGeneration;
                result.repositioned = true;
                result.positionSeconds = positionSeconds;
                result.durationSeconds = active.durationSeconds;
                return result;
            }
        }
    }

    if (!failureReason.empty()) {
        repository_.failBundle(sessionId, failureReason);
        result.reasonCode = failureReason;
    }
    return result;
}

bool RecordingMediaSessionRuntime::stop(
    const std::string& sessionId,
    const std::string& reasonCode)
{
    if (sessionId.empty() || reasonCode.empty()) return false;

    ActiveSession active;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        const auto found = active_.find(sessionId);
        if (found == active_.end()) return false;
        active = std::move(found->second);
        active_.erase(found);
    }

    bool workerStopped = true;
    if (active.pid > 0) {
        workerStopped = workerTerminator_(active.pid, WorkerShutdownGrace);
    }
    if (active.direct && directSourceRegistry_ != nullptr) {
        directSourceRegistry_->remove(sessionId);
    }

    const bool bundleEnded = repository_.endBundle(sessionId, reasonCode);
    return workerStopped && bundleEnded;
}

std::size_t RecordingMediaSessionRuntime::reapInactive(int idleTimeoutSeconds)
{
    if (idleTimeoutSeconds < 0 || idleTimeoutSeconds > 86400) {
        return 0;
    }

    struct Candidate
    {
        std::string sessionId;
        std::string grantId;
        pid_t pid = -1;
        bool continuousStream = false;
        std::uint64_t streamGeneration = 0;
    };

    std::vector<Candidate> candidates;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        candidates.reserve(active_.size());
        for (const auto& entry : active_) {
            Candidate candidate;
            candidate.sessionId = entry.first;
            candidate.grantId = entry.second.grantId;
            candidate.pid = entry.second.pid;
            candidate.continuousStream = entry.second.continuousStream;
            candidate.streamGeneration = entry.second.streamGeneration;
            candidates.push_back(std::move(candidate));
        }
    }

    std::size_t reaped = 0;
    for (const auto& candidate : candidates) {
        if (candidate.continuousStream && candidate.pid > 0) {
            int status = 0;
            errno = 0;
            const pid_t waited = ::waitpid(candidate.pid, &status, WNOHANG);
            if (waited == candidate.pid || (waited < 0 && errno == ECHILD)) {
                ActiveSession exited;
                {
                    std::lock_guard<std::mutex> lock(mutex_);
                    const auto found = active_.find(candidate.sessionId);
                    if (found == active_.end() ||
                        found->second.pid != candidate.pid ||
                        found->second.streamGeneration != candidate.streamGeneration) {
                        continue;
                    }
                    exited = std::move(found->second);
                    active_.erase(found);
                }
                if (exited.direct && directSourceRegistry_ != nullptr) {
                    directSourceRegistry_->remove(candidate.sessionId);
                }
                if (repository_.endBundle(
                        candidate.sessionId,
                        "recording_stream_worker_exited")) {
                    ++reaped;
                }
                continue;
            }
            if (waited < 0 && errno != EINTR) {
                bool stillCurrent = false;
                {
                    std::lock_guard<std::mutex> lock(mutex_);
                    const auto found = active_.find(candidate.sessionId);
                    stillCurrent = found != active_.end() &&
                        found->second.pid == candidate.pid &&
                        found->second.streamGeneration == candidate.streamGeneration;
                }
                if (stillCurrent &&
                    stop(candidate.sessionId, "recording_stream_worker_wait_failed")) {
                    ++reaped;
                }
                continue;
            }
        }

        // A continuous progressive response is one authenticated GET, just like
        // Live-TV. It has no HLS polling cadence with which to refresh
        // last_seen_at. Disable only idle expiry for that transport while still
        // enforcing explicit revocation and the absolute grant expiry. Worker
        // exit above is the transport-liveness fence for disconnect/EOF.
        const int grantIdleTimeout = candidate.continuousStream
            ? 0
            : idleTimeoutSeconds;
        const auto grant = repository_.findResolvedGrant(
            candidate.grantId,
            grantIdleTimeout);
        if (!grant.has_value() || grant->sessionId != candidate.sessionId) {
            if (candidate.continuousStream &&
                stop(candidate.sessionId, "media_access_grant_missing")) {
                ++reaped;
            }
            continue;
        }

        std::string reasonCode;
        if (!grant->active || grant->revoked) {
            reasonCode = "media_access_revoked";
        }
        else if (grant->expired) {
            reasonCode = "media_access_expired";
        }
        else if (grant->idleExpired) {
            reasonCode = "media_access_idle_expired";
        }

        if (!reasonCode.empty() && stop(candidate.sessionId, reasonCode)) {
            ++reaped;
        }
    }

    return reaped;
}

void RecordingMediaSessionRuntime::stopAll()
{
    std::map<std::string, ActiveSession> active;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        active.swap(active_);
    }

    for (auto& entry : active) {
        if (entry.second.pid > 0) {
            workerTerminator_(entry.second.pid, WorkerShutdownGrace);
        }
        if (entry.second.direct && directSourceRegistry_ != nullptr) {
            directSourceRegistry_->remove(entry.first);
        }
        repository_.endBundle(entry.first, "daemon_shutdown");
    }
}
