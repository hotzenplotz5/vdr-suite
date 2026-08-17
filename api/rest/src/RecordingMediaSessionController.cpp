#include "RecordingMediaSessionController.h"

#include "FfprobeRecordingSource.h"
#include "LocalVdrRecordingSourceResolver.h"
#include "MediaAccessCredentialHttp.h"
#include "MediaPresentationSelector.h"
#include "MediaProcessRunner.h"
#include "MediaSessionIssuanceService.h"
#include "MediaSessionRepository.h"
#include "MediaSessionWorkspace.h"
#include "VdrRecordingQueryService.h"

#include <array>
#include <cerrno>
#include <chrono>
#include <cctype>
#include <cstdlib>
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

struct RecordingMediaSessionRequest
{
    bool valid = false;
    std::string backendId;
    std::string recordingId;
    ClientMediaCapabilities capabilities;
};

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

void skipWhitespace(const std::string& value, std::size_t& position)
{
    while (position < value.size() &&
        std::isspace(static_cast<unsigned char>(value[position]))) {
        ++position;
    }
}

bool locateValue(
    const std::string& object,
    const std::string& key,
    std::size_t& position)
{
    const std::string token = "\"" + key + "\"";
    std::size_t cursor = 0;
    while ((cursor = object.find(token, cursor)) != std::string::npos) {
        std::size_t colon = cursor + token.size();
        skipWhitespace(object, colon);
        if (colon < object.size() && object[colon] == ':') {
            position = colon + 1;
            skipWhitespace(object, position);
            return true;
        }
        cursor += token.size();
    }
    return false;
}

bool readStringAt(
    const std::string& object,
    std::size_t& position,
    std::string& result)
{
    if (position >= object.size() || object[position] != '"') return false;
    ++position;
    result.clear();
    while (position < object.size()) {
        const char character = object[position++];
        if (character == '"') return true;
        if (character == '\\') {
            if (position >= object.size()) return false;
            const char escaped = object[position++];
            switch (escaped) {
            case '"': result.push_back('"'); break;
            case '\\': result.push_back('\\'); break;
            case '/': result.push_back('/'); break;
            case 'b': result.push_back('\b'); break;
            case 'f': result.push_back('\f'); break;
            case 'n': result.push_back('\n'); break;
            case 'r': result.push_back('\r'); break;
            case 't': result.push_back('\t'); break;
            default: return false;
            }
            continue;
        }
        if (static_cast<unsigned char>(character) < 0x20) return false;
        result.push_back(character);
    }
    return false;
}

bool readStringField(
    const std::string& object,
    const std::string& key,
    std::string& result)
{
    std::size_t position = 0;
    return locateValue(object, key, position) &&
        readStringAt(object, position, result);
}

bool readObjectField(
    const std::string& object,
    const std::string& key,
    std::string& result)
{
    std::size_t position = 0;
    if (!locateValue(object, key, position) ||
        position >= object.size() || object[position] != '{') {
        return false;
    }

    const std::size_t start = position;
    int depth = 0;
    bool inString = false;
    bool escaped = false;
    for (; position < object.size(); ++position) {
        const char character = object[position];
        if (inString) {
            if (escaped) escaped = false;
            else if (character == '\\') escaped = true;
            else if (character == '"') inString = false;
            continue;
        }
        if (character == '"') {
            inString = true;
            continue;
        }
        if (character == '{') ++depth;
        else if (character == '}') {
            --depth;
            if (depth == 0) {
                result = object.substr(start, position - start + 1);
                return true;
            }
            if (depth < 0) return false;
        }
    }
    return false;
}

bool readStringArrayField(
    const std::string& object,
    const std::string& key,
    std::vector<std::string>& result)
{
    std::size_t position = 0;
    if (!locateValue(object, key, position) ||
        position >= object.size() || object[position] != '[') {
        return false;
    }

    ++position;
    result.clear();
    for (;;) {
        skipWhitespace(object, position);
        if (position >= object.size()) return false;
        if (object[position] == ']') {
            ++position;
            return true;
        }

        std::string value;
        if (!readStringAt(object, position, value)) return false;
        result.push_back(std::move(value));
        skipWhitespace(object, position);
        if (position >= object.size()) return false;
        if (object[position] == ']') {
            ++position;
            return true;
        }
        if (object[position] != ',') return false;
        ++position;
    }
}

bool readBoolField(
    const std::string& object,
    const std::string& key,
    bool& result)
{
    std::size_t position = 0;
    if (!locateValue(object, key, position)) return false;
    if (object.compare(position, 4, "true") == 0) {
        result = true;
        return true;
    }
    if (object.compare(position, 5, "false") == 0) {
        result = false;
        return true;
    }
    return false;
}

bool readNonNegativeIntField(
    const std::string& object,
    const std::string& key,
    int& result)
{
    std::size_t position = 0;
    if (!locateValue(object, key, position)) return false;
    const char* start = object.c_str() + position;
    char* end = nullptr;
    const long parsed = std::strtol(start, &end, 10);
    if (end == start || parsed < 0 || parsed > 16384) return false;
    result = static_cast<int>(parsed);
    return true;
}

bool safeBackendId(const std::string& value)
{
    if (value.empty() || value.size() > 128) return false;
    for (unsigned char character : value) {
        if (!std::isalnum(character) && character != '-' && character != '_' &&
            character != '.' && character != ':') return false;
    }
    return true;
}

bool safeRecordingId(const std::string& value)
{
    if (value.empty() || value.size() > 512) return false;
    for (unsigned char character : value) {
        if (character < 0x20 || character == 0x7f) return false;
    }
    return true;
}

bool parseProtocols(
    const std::vector<std::string>& values,
    std::vector<MediaDeliveryProtocol>& result)
{
    result.clear();
    for (const auto& value : values) {
        if (value == "progressive") result.push_back(MediaDeliveryProtocol::Progressive);
        else if (value == "hls") result.push_back(MediaDeliveryProtocol::Hls);
        else return false;
    }
    return !result.empty();
}

bool parseContainers(
    const std::vector<std::string>& values,
    std::vector<MediaContainer>& result)
{
    result.clear();
    for (const auto& value : values) {
        if (value == "mpeg-ts") result.push_back(MediaContainer::MpegTs);
        else if (value == "mp4") result.push_back(MediaContainer::Mp4);
        else if (value == "fmp4") result.push_back(MediaContainer::Fmp4);
        else return false;
    }
    return !result.empty();
}

bool parseCodec(const std::string& value, MediaCodec& codec)
{
    if (value == "h264") codec = MediaCodec::H264;
    else if (value == "h265" || value == "hevc") codec = MediaCodec::H265;
    else if (value == "mpeg2video") codec = MediaCodec::Mpeg2Video;
    else if (value == "aac") codec = MediaCodec::Aac;
    else if (value == "ac3") codec = MediaCodec::Ac3;
    else if (value == "eac3") codec = MediaCodec::Eac3;
    else if (value == "mpeg-audio" || value == "mp2" || value == "mp3") codec = MediaCodec::MpegAudio;
    else return false;
    return true;
}

bool parseCodecs(
    const std::vector<std::string>& values,
    std::vector<MediaCodec>& result)
{
    result.clear();
    for (const auto& value : values) {
        MediaCodec codec = MediaCodec::Unknown;
        if (!parseCodec(value, codec)) return false;
        result.push_back(codec);
    }
    return true;
}

RecordingMediaSessionRequest parseRequest(const std::string& body)
{
    RecordingMediaSessionRequest request;
    if (!readStringField(body, "backendId", request.backendId) ||
        !readStringField(body, "recordingId", request.recordingId) ||
        !safeBackendId(request.backendId) ||
        !safeRecordingId(request.recordingId)) {
        return request;
    }

    std::string capabilitiesObject;
    if (!readObjectField(body, "capabilities", capabilitiesObject)) return request;

    std::vector<std::string> protocols;
    std::vector<std::string> containers;
    std::vector<std::string> videoCodecs;
    std::vector<std::string> audioCodecs;
    if (!readStringArrayField(capabilitiesObject, "protocols", protocols) ||
        !readStringArrayField(capabilitiesObject, "containers", containers) ||
        !readStringArrayField(capabilitiesObject, "videoCodecs", videoCodecs) ||
        !readStringArrayField(capabilitiesObject, "audioCodecs", audioCodecs) ||
        !readBoolField(
            capabilitiesObject,
            "supportsByteRanges",
            request.capabilities.supportsByteRanges) ||
        !readNonNegativeIntField(
            capabilitiesObject,
            "maxVideoWidth",
            request.capabilities.maxVideoWidth) ||
        !readNonNegativeIntField(
            capabilitiesObject,
            "maxVideoHeight",
            request.capabilities.maxVideoHeight) ||
        !parseProtocols(protocols, request.capabilities.protocols) ||
        !parseContainers(containers, request.capabilities.containers) ||
        !parseCodecs(videoCodecs, request.capabilities.videoCodecs) ||
        !parseCodecs(audioCodecs, request.capabilities.audioCodecs)) {
        return request;
    }

    request.valid = true;
    return request;
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
      workspaceRoot_(std::move(workspaceRoot))
{
}

ApiResponse RecordingMediaSessionController::createSession(
    const std::string& body,
    const std::string& actorId) const
{
    if (actorId.empty()) {
        return jsonError(401, "media_actor_required");
    }

    const RecordingMediaSessionRequest request = parseRequest(body);
    if (!request.valid) {
        return jsonError(400, "invalid_media_session_request");
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

    ApiResponse response;
    response.statusCode = 201;
    response.contentType = "application/json";
    response.headers["Cache-Control"] = "no-store";
    response.headers["X-Content-Type-Options"] = "nosniff";
    response.headers["Set-Cookie"] = cookie;
    response.body =
        "{\"mediaSession\":{"
        "\"id\":\"" + jsonEscape(issuance.session.sessionId) + "\"," +
        "\"state\":\"provisioning\"," +
        "\"backendId\":\"" + jsonEscape(request.backendId) + "\"," +
        "\"recordingId\":\"" + jsonEscape(request.recordingId) + "\"," +
        "\"presentationProfileId\":\"" + jsonEscape(profile.profileId) + "\"," +
        "\"mediaPath\":\"/api/media/sessions/" +
            jsonEscape(issuance.session.sessionId) + "/\"," +
        "\"expiresAt\":\"" + jsonEscape(issuance.session.expiresAt) + "\"}}";

    issuance.session.clearSecret();
    return response;
}
