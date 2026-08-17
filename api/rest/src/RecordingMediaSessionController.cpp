#include "RecordingMediaSessionController.h"

#include "FfprobeRecordingSource.h"
#include "LocalVdrRecordingSourceResolver.h"
#include "MediaAccessCredentialHttp.h"
#include "MediaPresentationSelector.h"
#include "MediaProcessRunner.h"
#include "MediaSessionIssuanceService.h"
#include "MediaSessionRepository.h"
#include "MediaSessionWorkspace.h"
#include "RecordingMediaSessionRequestParser.h"
#include "RecordingMediaSessionRuntime.h"
#include "VdrRecordingQueryService.h"

#include <array>
#include <cerrno>
#include <chrono>
#include <memory>
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

ApiResponse stoppedResponse(const std::string& sessionId)
{
    ApiResponse response;
    response.statusCode = 200;
    response.contentType = "application/json";
    response.headers["Cache-Control"] = "no-store";
    response.headers["X-Content-Type-Options"] = "nosniff";
    const std::string expiredCookie =
        MediaAccessCredentialHttp::expiredSessionCookie(sessionId);
    if (!expiredCookie.empty()) {
        response.headers["Set-Cookie"] = expiredCookie;
    }
    response.body =
        "{\"mediaSession\":{\"id\":\"" +
        jsonEscape(sessionId) +
        "\",\"state\":\"ended\"}}";
    return response;
}

ApiResponse stopFailureResponse(
    const std::string& sessionId,
    const std::string& code)
{
    ApiResponse response = jsonError(503, code);
    const std::string expiredCookie =
        MediaAccessCredentialHttp::expiredSessionCookie(sessionId);
    if (!expiredCookie.empty()) {
        response.headers["Set-Cookie"] = expiredCookie;
    }
    return response;
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

} // namespace

RecordingMediaSessionController::RecordingMediaSessionController(
    VdrRecordingQueryService& recordingQueryService,
    MediaSessionRepository& mediaSessionRepository,
    MediaSessionIssuanceService& mediaSessionIssuanceService,
    std::string workspaceRoot)
    : recordingQueryService_(recordingQueryService),
      mediaSessionRepository_(mediaSessionRepository),
      mediaSessionIssuanceService_(mediaSessionIssuanceService),
      mediaSessionRuntime_(std::make_unique<RecordingMediaSessionRuntime>(
          mediaSessionRepository,
          workspaceRoot)),
      workspaceRoot_(std::move(workspaceRoot))
{
}

RecordingMediaSessionController::~RecordingMediaSessionController() = default;

ApiResponse RecordingMediaSessionController::handleRequest(
    const std::string& body,
    const std::string& actorId) const
{
    if (actorId.empty()) {
        return jsonError(401, "media_actor_required");
    }

    const RecordingMediaSessionStopRequest stopRequest =
        RecordingMediaSessionRequestParser().parseStop(body);
    if (stopRequest.valid) {
        return stopSession(body, actorId);
    }
    if (stopRequest.reasonCode != "media_session_stop_not_requested") {
        return jsonError(
            400,
            stopRequest.reasonCode.empty()
                ? "invalid_media_session_operation"
                : stopRequest.reasonCode);
    }

    return createSession(body, actorId);
}

ApiResponse RecordingMediaSessionController::stopSession(
    const std::string& body,
    const std::string& actorId) const
{
    const RecordingMediaSessionStopRequest request =
        RecordingMediaSessionRequestParser().parseStop(body);
    if (!request.valid) {
        return jsonError(
            400,
            request.reasonCode.empty()
                ? "invalid_media_session_stop_request"
                : request.reasonCode);
    }

    const auto stored = mediaSessionRepository_.findSession(request.sessionId);
    if (!stored.has_value() || stored->backendId != request.backendId) {
        return jsonError(404, "media_session_not_found");
    }
    if (stored->actorId != actorId) {
        return jsonError(403, "media_session_not_owned");
    }

    if (stored->state == "ended" || stored->state == "failed") {
        return stoppedResponse(request.sessionId);
    }

    if (!mediaSessionRuntime_->stop(request.sessionId, "client_closed")) {
        mediaSessionRepository_.endBundle(
            request.sessionId,
            "client_stop_runtime_unavailable");
        return stopFailureResponse(
            request.sessionId,
            "media_session_stop_failed");
    }

    return stoppedResponse(request.sessionId);
}

ApiResponse RecordingMediaSessionController::createSession(
    const std::string& body,
    const std::string& actorId) const
{
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
    const LocalVdrRecordingSourceResolution sourceResolution =
        trustedResolver.resolve(request.backendId, request.recordingId);
    if (!sourceResolution.resolved) {
        return jsonError(
            sourceResolution.reasonCode == "recording_not_found" ? 404 : 503,
            sourceResolution.reasonCode.empty()
                ? "recording_source_unavailable"
                : sourceResolution.reasonCode);
    }

    MediaSessionWorkspace probeWorkspace(workspaceRoot_);
    const MediaSessionWorkspaceResult workspaceResult =
        prepareProbeWorkspace(probeWorkspace, sourceResolution.source.segmentPaths);
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

    const MediaPresentationProfile profile =
        MediaPresentationSelector().select(
            probeResult.source,
            request.capabilities);
    if (!profile.available || profile.profileId.empty()) {
        return jsonError(422, "media_presentation_unavailable");
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

    if (profile.protocol != MediaDeliveryProtocol::Hls) {
        mediaSessionRepository_.endBundle(
            issuance.session.sessionId,
            "progressive_direct_not_yet_provisioned");
        issuance.session.clearSecret();
        return jsonError(422, "media_progressive_direct_not_available");
    }

    const RecordingMediaSessionProvisionResult provision =
        mediaSessionRuntime_->provisionHls(
            issuance.session.sessionId,
            issuance.session.workspaceId,
            profile,
            sourceResolution.source.segmentPaths);
    if (!provision.ready) {
        issuance.session.clearSecret();
        return jsonError(
            503,
            provision.reasonCode.empty()
                ? "media_hls_provision_failed"
                : provision.reasonCode);
    }

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
        "\"mediaPath\":\"/api/media/sessions/" +
            jsonEscape(issuance.session.sessionId) + "/hls/master.m3u8\"," +
        "\"expiresAt\":\"" + jsonEscape(issuance.session.expiresAt) + "\"}}";

    issuance.session.clearSecret();
    return response;
}