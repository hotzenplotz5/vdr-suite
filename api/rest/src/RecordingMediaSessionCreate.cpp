#include "RecordingMediaSessionController.h"

#include "FfprobeRecordingSource.h"
#include "LocalVdrRecordingSourceResolver.h"
#include "MediaAccessCredentialHttp.h"
#include "MediaPresentationSelector.h"
#include "MediaProcessRunner.h"
#include "MediaSessionIssuanceService.h"
#include "MediaSessionRepository.h"
#include "MediaSessionWorkspace.h"
#include "MediaTranscodeSettingsApiRuntime.h"
#include "RecordingDirectSourceRegistry.h"
#include "RecordingSourceFingerprint.h"
#include "RecordingMediaSessionRequestParser.h"
#include "RecordingMediaSessionRuntime.h"
#include "RecordingMediaSessionStartPosition.h"
#include "VdrRecordingDuration.h"
#include "VdrRecordingIndexUpdater.h"
#include "VdrRecordingQueryService.h"

#include <algorithm>
#include <array>
#include <cerrno>
#include <chrono>
#include <iostream>
#include <string>
#include <sys/random.h>
#include <utility>
#include <vector>

namespace
{

constexpr int MediaSessionLifetimeSeconds = 21600;
constexpr std::size_t MaximumProbeOutputBytes = 1024 * 1024;
constexpr auto ProbeTimeout = std::chrono::seconds(10);
constexpr const char* LocalRecordingProviderId = "local-vdr-recording";

ApiResponse jsonError(int statusCode, const std::string& code)
{
    ApiResponse response;
    response.statusCode = statusCode;
    response.contentType = "application/json";
    response.headers["Cache-Control"] = "no-store";
    response.headers["X-Content-Type-Options"] = "nosniff";
    response.body = "{\"error\":{\"code\":\"" + code + "\"}}";
    return response;
}

std::string jsonEscape(const std::string& value)
{
    std::string result;
    result.reserve(value.size());
    for (unsigned char character : value) {
        switch (character) {
        case '"': result += "\\\""; break;
        case '\\': result += "\\\\"; break;
        case '\b': result += "\\b"; break;
        case '\f': result += "\\f"; break;
        case '\n': result += "\\n"; break;
        case '\r': result += "\\r"; break;
        case '\t': result += "\\t"; break;
        default:
            if (character < 0x20) {
                static constexpr char Hex[] = "0123456789abcdef";
                result += "\\u00";
                result.push_back(Hex[(character >> 4) & 0x0f]);
                result.push_back(Hex[character & 0x0f]);
            }
            else {
                result.push_back(static_cast<char>(character));
            }
        }
    }
    return result;
}

std::string playbackJson(
    int positionSeconds,
    int durationSeconds,
    bool seekSupported,
    bool seekPreparing,
    bool resumeSupported,
    bool resumePreparing)
{
    std::string result =
        "{\"positionSeconds\":" + std::to_string(std::max(0, positionSeconds)) +
        ",\"durationSeconds\":";
    if (durationSeconds > 0) result += std::to_string(durationSeconds);
    else result += "null";

    result += ",\"seek\":{\"supported\":";
    result += seekSupported ? "true" : "false";
    result += ",\"preparing\":";
    result += seekPreparing ? "true" : "false";
    if (seekSupported && durationSeconds > 0) {
        result +=
            ",\"window\":{\"startSeconds\":0,\"endSeconds\":" +
            std::to_string(durationSeconds) + "}";
    }
    result += "},\"resume\":{\"supported\":";
    result += resumeSupported ? "true" : "false";
    result += ",\"preparing\":";
    result += resumePreparing ? "true" : "false";
    result += "}}";
    return result;
}

bool systemEntropy(unsigned char* output, std::size_t size)
{
    std::size_t offset = 0;
    while (offset < size) {
        const ssize_t count = ::getrandom(output + offset, size - offset, 0);
        if (count < 0) {
            if (errno == EINTR) continue;
            return false;
        }
        if (count == 0) return false;
        offset += static_cast<std::size_t>(count);
    }
    return true;
}

std::string probeWorkspaceId()
{
    std::array<unsigned char, 12> bytes{};
    if (!systemEntropy(bytes.data(), bytes.size())) return {};
    static constexpr char Hex[] = "0123456789abcdef";
    std::string id = "probe_";
    id.reserve(6 + bytes.size() * 2);
    for (unsigned char byte : bytes) {
        id.push_back(Hex[(byte >> 4) & 0x0f]);
        id.push_back(Hex[byte & 0x0f]);
    }
    return id;
}

MediaSessionWorkspaceResult prepareProbeWorkspace(
    MediaSessionWorkspace& workspace,
    const std::vector<std::string>& segments)
{
    MediaSessionWorkspaceResult result;
    for (int attempt = 0; attempt < 3; ++attempt) {
        const std::string id = probeWorkspaceId();
        if (id.empty()) {
            result.reasonCode = "media_probe_entropy_unavailable";
            return result;
        }
        result = workspace.prepare(id, segments);
        if (result.ready || result.reasonCode != "workspace_create_failed") {
            return result;
        }
    }
    return result;
}

std::string descriptorCacheKey(
    const std::string& backendId,
    const std::string& recordingId)
{
    return backendId + "\n" + recordingId;
}

long long elapsedMilliseconds(
    std::chrono::steady_clock::time_point start,
    std::chrono::steady_clock::time_point end)
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
}

void removeProgressiveCapability(ClientMediaCapabilities& capabilities)
{
    capabilities.protocols.erase(
        std::remove(
            capabilities.protocols.begin(),
            capabilities.protocols.end(),
            MediaDeliveryProtocol::Progressive),
        capabilities.protocols.end());
}

std::string transcodePolicyReasonCode(const MediaPresentationProfile& profile)
{
    if (profile.reason.find("forced VAAPI does not support") != std::string::npos) {
        return "forced_vaapi_transformation_unsupported";
    }
    if (profile.reason.find("forced VAAPI is unavailable") != std::string::npos) {
        return "forced_vaapi_unavailable";
    }
    return "media_transcode_capacity_unproven";
}

bool hlsProfile(const std::string& profileId)
{
    return profileId == "hls-fmp4" || profileId == "hls-ts";
}

} // namespace

ApiResponse RecordingMediaSessionController::createSession(
    const std::string& body,
    const std::string& actorId) const
{
    const auto requestStartedAt = std::chrono::steady_clock::now();
    if (actorId.empty()) {
        return jsonError(401, "media_actor_required");
    }

    const RecordingMediaSessionRequest request =
        RecordingMediaSessionRequestParser().parse(body);
    if (!request.valid) {
        return jsonError(
            400,
            request.reasonCode.empty()
                ? "invalid_media_session_request"
                : request.reasonCode);
    }

    const RecordingMediaSessionStartPosition requestedStart =
        RecordingMediaSessionStartPositionParser().parse(body);
    if (!requestedStart.valid) {
        return jsonError(
            400,
            requestedStart.reasonCode.empty()
                ? "invalid_recording_start_position"
                : requestedStart.reasonCode);
    }
    const int startPositionSeconds = requestedStart.seconds;

    VdrRecording recording;
    if (!recordingQueryService_.findRecordingById(
            request.backendId,
            request.recordingId,
            recording)) {
        return jsonError(404, "recording_not_found");
    }

    LocalVdrRecordingSourceResolver trustedResolver(
        [recording](const std::string& backendId) {
            if (recording.backendId != backendId &&
                !(recording.backendId.empty() && backendId == "default")) {
                return std::vector<VdrRecording>{};
            }
            return std::vector<VdrRecording>{recording};
        });
    LocalVdrRecordingSourceResolution sourceResolution =
        trustedResolver.resolve(request.backendId, request.recordingId);
    if (!sourceResolution.resolved) {
        return jsonError(
            sourceResolution.reasonCode == "recording_not_found" ? 404 : 503,
            sourceResolution.reasonCode.empty()
                ? "recording_source_unavailable"
                : sourceResolution.reasonCode);
    }
    const auto sourceResolvedAt = std::chrono::steady_clock::now();

    const std::string cacheKey = descriptorCacheKey(
        request.backendId,
        request.recordingId);
    MediaSourceDescriptor source;
    bool probeCacheHit = false;
    bool indexMarkerReconciled = false;
    const bool suiteIndexUpdateRunning =
        sourceResolution.source.growing &&
        indexUpdater_ != nullptr &&
        indexUpdater_->status(sourceResolution.source.recordingDirectory).running();

    if (!sourceResolution.source.growing || suiteIndexUpdateRunning) {
        std::lock_guard<std::mutex> lock(descriptorCacheMutex_);
        const auto cached = descriptorCache_.find(cacheKey);
        if (cached != descriptorCache_.end()) {
            if (!sourceResolution.source.growing &&
                cached->second.sourceFingerprint ==
                    sourceResolution.source.sourceFingerprint) {
                source = cached->second.source;
                probeCacheHit = true;
            }
            else if (suiteIndexUpdateRunning &&
                     !cached->second.source.growing &&
                     sameRecordingSourceExtentIgnoringGrowthState(
                         cached->second.sourceFingerprint,
                         sourceResolution.source.sourceFingerprint)) {
                // VDR's --updindex temporarily creates .timer itself. Only
                // reconcile that marker when this controller owns a running
                // updater for exactly this directory and the previously
                // completed source extent is byte-for-byte/identity stable.
                // Any real segment change therefore remains growing/fail-closed.
                sourceResolution.source.growing = false;
                sourceResolution.source.sourceFingerprint =
                    cached->second.sourceFingerprint;
                source = cached->second.source;
                probeCacheHit = true;
                indexMarkerReconciled = true;
            }
        }
    }

    if (!probeCacheHit) {
        MediaSessionWorkspace probeWorkspace(workspaceRoot_);
        const MediaSessionWorkspaceResult workspaceResult =
            prepareProbeWorkspace(
                probeWorkspace,
                sourceResolution.source.segmentPaths);
        if (!workspaceResult.ready) {
            return jsonError(
                503,
                workspaceResult.reasonCode.empty()
                    ? "media_probe_workspace_unavailable"
                    : workspaceResult.reasonCode);
        }

        const FfprobeRecordingSource probe;
        const FfprobeRecordingPlan probePlan = probe.commandPlan();
        const MediaProcessCaptureResult processResult = MediaProcessRunner().runAndCapture(
            probePlan.argv,
            probeWorkspace.directory(),
            ProbeTimeout,
            MaximumProbeOutputBytes);
        if (!processResult.success) {
            return jsonError(
                503,
                processResult.reasonCode.empty()
                    ? "media_source_probe_failed"
                    : processResult.reasonCode);
        }

        const FfprobeRecordingResult probeResult = probe.parse(processResult.output);
        if (!probeResult.valid) {
            return jsonError(
                422,
                probeResult.reasonCode.empty()
                    ? "media_source_unsupported"
                    : probeResult.reasonCode);
        }
        source = probeResult.source;
    }

    source.resourceKind = sourceResolution.source.growing
        ? MediaResourceKind::GrowingRecording
        : MediaResourceKind::Recording;
    source.growing = sourceResolution.source.growing;
    source.seekable = !sourceResolution.source.growing;

    if (!sourceResolution.source.growing && !probeCacheHit) {
        CachedSourceDescriptor cached;
        cached.sourceFingerprint = sourceResolution.source.sourceFingerprint;
        cached.source = source;
        std::lock_guard<std::mutex> lock(descriptorCacheMutex_);
        descriptorCache_[cacheKey] = std::move(cached);
    }
    const auto descriptorReadyAt = std::chrono::steady_clock::now();

    ClientMediaCapabilities effectiveCapabilities = request.capabilities;
    if (sourceResolution.source.growing ||
        !sourceResolution.source.progressiveDirectSafe) {
        removeProgressiveCapability(effectiveCapabilities);
    }

    MediaPresentationProfile profile =
        MediaPresentationSelector().select(source, effectiveCapabilities);
    if (!profile.available || profile.profileId.empty()) {
        return jsonError(422, "media_presentation_unavailable");
    }
    if (profile.videoAction == MediaTrackAction::Transcode) {
        const MediaTranscodePolicy policy =
            MediaTranscodeSettingsApiRuntime::instance().resolvePolicy(
                request.backendId);
        profile = policy.apply(profile);
        if (!profile.available) {
            return jsonError(503, transcodePolicyReasonCode(profile));
        }
    }
    const auto presentationSelectedAt = std::chrono::steady_clock::now();

    const int truthfulDurationSeconds =
        !sourceResolution.source.growing &&
        recording.recordingDurationKnown &&
        recording.durationSeconds > 0
            ? recording.durationSeconds
            : 0;
    const std::vector<double> indexedSegmentDurations =
        truthfulDurationSeconds > 0
            ? vdrsuite::recording::segmentDurationsSecondsFromIndex(
                  recording,
                  sourceResolution.source.segmentPaths)
            : std::vector<double>{};
    const bool indexedTimelineReady =
        !indexedSegmentDurations.empty() &&
        indexedSegmentDurations.size() == sourceResolution.source.segmentPaths.size();
    const bool resumeProfile =
        profile.profileId == "progressive-fmp4" || hlsProfile(profile.profileId);
    const bool resumeSupported =
        resumeProfile && truthfulDurationSeconds > 0 && indexedTimelineReady;
    const bool timeSeekSupported =
        profile.profileId == "progressive-fmp4" && resumeSupported;

    if (startPositionSeconds > 0) {
        if (sourceResolution.source.growing) {
            return jsonError(409, "recording_resume_growing_not_supported");
        }
        if (!hlsProfile(profile.profileId)) {
            return jsonError(409, "recording_resume_profile_not_supported");
        }
        if (!resumeSupported) {
            return jsonError(409, "recording_resume_not_ready");
        }
        if (startPositionSeconds >= truthfulDurationSeconds) {
            return jsonError(422, "recording_resume_outside_window");
        }
    }

    MediaSessionIssuanceRequest issuanceRequest;
    issuanceRequest.actorId = actorId;
    issuanceRequest.backendId = request.backendId;
    issuanceRequest.resourceKind = "recording";
    issuanceRequest.resourceId = request.recordingId;
    issuanceRequest.presentationProfileId = profile.profileId;
    issuanceRequest.providerId = LocalRecordingProviderId;
    issuanceRequest.lifetimeSeconds = MediaSessionLifetimeSeconds;

    MediaSessionIssuanceResult issuance =
        mediaSessionIssuanceService_.issue(issuanceRequest);
    if (!issuance.issued) {
        return jsonError(
            503,
            issuance.reasonCode.empty()
                ? "media_session_issue_failed"
                : issuance.reasonCode);
    }

    const std::string cookie = MediaAccessCredentialHttp::sessionCookie(
        issuance.session.sessionId,
        issuance.session.accessCredential,
        MediaSessionLifetimeSeconds);
    if (cookie.empty()) {
        mediaSessionRepository_.endBundle(
            issuance.session.sessionId,
            "credential_transport_failed");
        issuance.session.clearSecret();
        return jsonError(500, "media_access_credential_transport_failed");
    }
    const auto sessionIssuedAt = std::chrono::steady_clock::now();

    RecordingMediaSessionProvisionResult provision;
    std::string mediaPath;
    if (profile.protocol == MediaDeliveryProtocol::Progressive) {
        if (profile.profileId == "progressive-direct") {
            if (directSourceRegistry_ == nullptr || sourceResolution.source.growing ||
                !sourceResolution.source.progressiveDirectSafe ||
                !request.capabilities.supportsByteRanges) {
                mediaSessionRepository_.endBundle(
                    issuance.session.sessionId,
                    "progressive_direct_source_not_safe");
                issuance.session.clearSecret();
                return jsonError(422, "media_progressive_direct_not_available");
            }

            RecordingDirectSourceRegistration registration;
            registration.recordingDirectory =
                sourceResolution.source.recordingDirectory;
            registration.segmentPaths = sourceResolution.source.segmentPaths;
            registration.sourceFingerprint =
                sourceResolution.source.sourceFingerprint;
            registration.readableBytes = sourceResolution.source.readableBytes;
            provision = mediaSessionRuntime_->provisionDirect(
                issuance.session.sessionId,
                issuance.session.grantId,
                profile,
                registration);
            mediaPath = "/api/media/sessions/" +
                issuance.session.sessionId + "/recording/stream.ts";
        }
        else if (profile.profileId == "progressive-fmp4") {
            if (sourceResolution.source.growing) {
                mediaSessionRepository_.endBundle(
                    issuance.session.sessionId,
                    "recording_progressive_stream_source_is_growing");
                issuance.session.clearSecret();
                return jsonError(422, "media_progressive_stream_not_available");
            }
            provision = mediaSessionRuntime_->provisionStream(
                issuance.session.sessionId,
                issuance.session.workspaceId,
                issuance.session.grantId,
                profile,
                sourceResolution.source.segmentPaths,
                timeSeekSupported ? truthfulDurationSeconds : 0,
                timeSeekSupported
                    ? indexedSegmentDurations
                    : std::vector<double>{});
            mediaPath = "/api/media/sessions/" +
                issuance.session.sessionId + "/recording/stream.mp4";
        }
        else {
            mediaSessionRepository_.endBundle(
                issuance.session.sessionId,
                "media_progressive_profile_not_supported");
            issuance.session.clearSecret();
            return jsonError(422, "media_presentation_unavailable");
        }
    }
    else if (profile.protocol == MediaDeliveryProtocol::Hls) {
        if (startPositionSeconds > 0) {
            provision = mediaSessionRuntime_->provisionHlsAt(
                issuance.session.sessionId,
                issuance.session.workspaceId,
                issuance.session.grantId,
                profile,
                sourceResolution.source.segmentPaths,
                startPositionSeconds,
                indexedSegmentDurations);
        }
        else {
            provision = mediaSessionRuntime_->provisionHls(
                issuance.session.sessionId,
                issuance.session.workspaceId,
                issuance.session.grantId,
                profile,
                sourceResolution.source.segmentPaths);
        }
        mediaPath = "/api/media/sessions/" +
            issuance.session.sessionId + "/hls/master.m3u8";
    }
    else {
        mediaSessionRepository_.endBundle(
            issuance.session.sessionId,
            "media_delivery_protocol_not_supported");
        issuance.session.clearSecret();
        return jsonError(422, "media_presentation_unavailable");
    }

    if (!provision.ready) {
        issuance.session.clearSecret();
        return jsonError(
            503,
            provision.reasonCode.empty()
                ? "media_provision_failed"
                : provision.reasonCode);
    }
    const auto provisionReadyAt = std::chrono::steady_clock::now();

    bool indexPreparing = false;
    if (resumeProfile &&
        !sourceResolution.source.growing &&
        !indexedTimelineReady) {
        const VdrRecordingIndexUpdateResult update = indexUpdater_->ensure(
            sourceResolution.source.recordingDirectory,
            sourceResolution.source.segmentPaths);
        if (update.running() || update.succeeded()) {
            PendingIndexContext context;
            context.backendId = request.backendId;
            context.recordingId = request.recordingId;
            context.recordingDirectory = sourceResolution.source.recordingDirectory;
            context.sourceSegments = sourceResolution.source.segmentPaths;
            {
                std::lock_guard<std::mutex> lock(pendingIndexMutex_);
                pendingIndex_[issuance.session.sessionId] = std::move(context);
            }
            indexPreparing = true;
        }
    }

    std::clog
        << "recording media startup"
        << " session=" << issuance.session.sessionId
        << " profile=" << profile.profileId
        << " growing=" << (sourceResolution.source.growing ? "true" : "false")
        << " index_marker_reconciled=" << (indexMarkerReconciled ? "true" : "false")
        << " start_position=" << startPositionSeconds
        << " index_preparing=" << (indexPreparing ? "true" : "false")
        << " probe_cache=" << (probeCacheHit ? "hit" : "miss")
        << " source_ms=" << elapsedMilliseconds(requestStartedAt, sourceResolvedAt)
        << " descriptor_ms=" << elapsedMilliseconds(sourceResolvedAt, descriptorReadyAt)
        << " selection_ms=" << elapsedMilliseconds(descriptorReadyAt, presentationSelectedAt)
        << " session_ms=" << elapsedMilliseconds(presentationSelectedAt, sessionIssuedAt)
        << " provision_ms=" << elapsedMilliseconds(sessionIssuedAt, provisionReadyAt)
        << " total_server_ms=" << elapsedMilliseconds(requestStartedAt, provisionReadyAt)
        << std::endl;

    ApiResponse response;
    response.statusCode = 201;
    response.contentType = "application/json";
    response.headers["Cache-Control"] = "no-store";
    response.headers["X-Content-Type-Options"] = "nosniff";
    response.headers["Set-Cookie"] = cookie;
    response.body =
        "{\"mediaSession\":{"
        "\"id\":\"" + jsonEscape(issuance.session.sessionId) + "\"," +
        "\"state\":\"ready\"," +
        "\"backendId\":\"" + jsonEscape(request.backendId) + "\"," +
        "\"recordingId\":\"" + jsonEscape(request.recordingId) + "\"," +
        "\"presentationProfileId\":\"" + jsonEscape(profile.profileId) + "\"," +
        "\"growing\":" + std::string(sourceResolution.source.growing ? "true" : "false") + "," +
        "\"readableBytes\":" + std::to_string(sourceResolution.source.readableBytes) + "," +
        "\"mediaPath\":\"" + jsonEscape(mediaPath) + "\"," +
        "\"playback\":" + playbackJson(
            startPositionSeconds,
            truthfulDurationSeconds,
            timeSeekSupported,
            indexPreparing && profile.profileId == "progressive-fmp4",
            resumeSupported,
            indexPreparing) + "," +
        "\"expiresAt\":\"" + jsonEscape(issuance.session.expiresAt) + "\"}}";

    issuance.session.clearSecret();
    return response;
}
