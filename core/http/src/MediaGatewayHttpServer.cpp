#include "MediaGatewayHttpServer.h"

#include "MediaAccessCredentialHttp.h"
#include "MediaAccessGrantAuthenticator.h"
#include "MediaHlsArtifactReader.h"
#include "MediaRouteLeaseRepository.h"
#include "RecordingDirectSourceRegistry.h"
#include "SegmentedRecordingByteSource.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <filesystem>
#include <limits>
#include <string>
#include <sys/stat.h>
#include <utility>

namespace
{

constexpr const char* Prefix = "/api/media/sessions/";
constexpr const char* HlsMarker = "/hls/";
constexpr const char* LiveSuffix = "/live/stream.mp4";
constexpr const char* RecordingDirectSuffix = "/recording/stream.ts";

bool safeIdentifier(const std::string& value)
{
    if (value.empty() || value.size() > 128) return false;
    return std::all_of(
        value.begin(), value.end(),
        [](unsigned char character) {
            return std::isalnum(character) || character == '-' ||
                character == '_' || character == '.' || character == ':';
        });
}

struct MediaPath
{
    bool valid = false;
    bool live = false;
    bool recordingDirect = false;
    std::string sessionId;
    std::string artifactName;
};

MediaPath parseMediaPath(const std::string& path)
{
    MediaPath result;
    const std::string prefix(Prefix);
    if (path.rfind(prefix, 0) != 0 || path.find('?') != std::string::npos ||
        path.find('#') != std::string::npos) {
        return result;
    }

    const std::size_t hlsMarker = path.find(HlsMarker, prefix.size());
    if (hlsMarker != std::string::npos) {
        result.sessionId = path.substr(prefix.size(), hlsMarker - prefix.size());
        result.artifactName = path.substr(hlsMarker + std::string(HlsMarker).size());
        result.valid = safeIdentifier(result.sessionId) &&
            MediaHlsArtifactReader::allowedArtifactName(result.artifactName);
        return result;
    }

    const std::string directSuffix(RecordingDirectSuffix);
    if (path.size() > prefix.size() + directSuffix.size() &&
        path.compare(
            path.size() - directSuffix.size(),
            directSuffix.size(),
            directSuffix) == 0) {
        result.sessionId = path.substr(
            prefix.size(), path.size() - prefix.size() - directSuffix.size());
        result.recordingDirect = true;
        result.valid = safeIdentifier(result.sessionId);
        return result;
    }

    const std::string liveSuffix(LiveSuffix);
    if (path.size() <= prefix.size() + liveSuffix.size() ||
        path.compare(
            path.size() - liveSuffix.size(),
            liveSuffix.size(),
            liveSuffix) != 0) {
        return result;
    }
    result.sessionId = path.substr(
        prefix.size(), path.size() - prefix.size() - liveSuffix.size());
    result.live = true;
    result.valid = safeIdentifier(result.sessionId);
    return result;
}

std::string headerValue(
    const HttpServerRequest& request,
    const std::string& wanted)
{
    for (const auto& entry : request.headers) {
        if (entry.first.size() != wanted.size()) continue;
        bool equal = true;
        for (std::size_t index = 0; index < wanted.size(); ++index) {
            if (std::tolower(static_cast<unsigned char>(entry.first[index])) !=
                std::tolower(static_cast<unsigned char>(wanted[index]))) {
                equal = false;
                break;
            }
        }
        if (equal) return entry.second;
    }
    return {};
}

std::string mediaCredentialFromCookie(const std::string& cookie)
{
    std::size_t cursor = 0;
    while (cursor < cookie.size()) {
        while (cursor < cookie.size() &&
            (cookie[cursor] == ' ' || cookie[cursor] == ';')) ++cursor;
        const std::size_t end = cookie.find(';', cursor);
        const std::string item = cookie.substr(
            cursor,
            end == std::string::npos ? std::string::npos : end - cursor);
        const std::size_t separator = item.find('=');
        if (separator != std::string::npos &&
            item.substr(0, separator) == MediaAccessCredentialHttp::CookieName) {
            return item.substr(separator + 1);
        }
        if (end == std::string::npos) break;
        cursor = end + 1;
    }
    return {};
}

std::string mediaCredential(const HttpServerRequest& request)
{
    const std::string authorization =
        headerValue(request, MediaAccessCredentialHttp::AuthorizationHeader);
    const std::string bearer = "Bearer ";
    if (authorization.rfind(bearer, 0) == 0 &&
        authorization.size() > bearer.size()) {
        return authorization.substr(bearer.size());
    }

    return mediaCredentialFromCookie(headerValue(request, "Cookie"));
}

HttpServerResponse jsonError(int statusCode, const std::string& code)
{
    HttpServerResponse response;
    response.statusCode = statusCode;
    response.headers["Content-Type"] = "application/json";
    response.headers["Cache-Control"] = "no-store";
    response.headers["X-Content-Type-Options"] = "nosniff";
    response.body =
        "{\"error\":{\"code\":\"" + code + "\"}}";
    return response;
}

std::string liveStreamPath(
    const std::string& workspaceRoot,
    const std::string& workspaceId)
{
    if (!safeIdentifier(workspaceId)) return {};
    const std::filesystem::path root(workspaceRoot);
    if (!root.is_absolute()) return {};
    const std::filesystem::path workspace = root / workspaceId;
    const std::filesystem::path stream = workspace / "live.fmp4";
    if (workspace.parent_path() != root || stream.parent_path() != workspace)
        return {};

    struct stat status {};
    if (::lstat(stream.c_str(), &status) != 0 || !S_ISFIFO(status.st_mode))
        return {};
    return stream.string();
}

bool decimalValue(const std::string& text, std::uint64_t& value)
{
    if (text.empty()) return false;
    value = 0;
    for (unsigned char character : text) {
        if (!std::isdigit(character)) return false;
        const unsigned digit = character - '0';
        if (value > (std::numeric_limits<std::uint64_t>::max() - digit) / 10U)
            return false;
        value = value * 10U + digit;
    }
    return true;
}

enum class RangeState
{
    Valid,
    Malformed,
    Unsatisfiable
};

struct ByteRange
{
    RangeState state = RangeState::Malformed;
    std::uint64_t first = 0;
    std::uint64_t last = 0;
};

ByteRange parseSingleByteRange(
    const std::string& header,
    std::uint64_t length)
{
    ByteRange result;
    constexpr const char* BytesPrefix = "bytes=";
    if (header.rfind(BytesPrefix, 0) != 0 ||
        header.find(',') != std::string::npos || length == 0) {
        return result;
    }

    const std::string spec = header.substr(6);
    const std::size_t dash = spec.find('-');
    if (dash == std::string::npos || spec.find('-', dash + 1) != std::string::npos)
        return result;

    const std::string firstText = spec.substr(0, dash);
    const std::string lastText = spec.substr(dash + 1);
    if (firstText.empty()) {
        std::uint64_t suffixLength = 0;
        if (!decimalValue(lastText, suffixLength) || suffixLength == 0)
            return result;
        if (suffixLength >= length) {
            result.first = 0;
        }
        else {
            result.first = length - suffixLength;
        }
        result.last = length - 1;
        result.state = RangeState::Valid;
        return result;
    }

    std::uint64_t first = 0;
    if (!decimalValue(firstText, first)) return result;
    if (first >= length) {
        result.state = RangeState::Unsatisfiable;
        return result;
    }

    std::uint64_t last = length - 1;
    if (!lastText.empty()) {
        if (!decimalValue(lastText, last)) return result;
        if (last < first) return result;
        if (last >= length) last = length - 1;
    }

    result.first = first;
    result.last = last;
    result.state = RangeState::Valid;
    return result;
}

HttpServerResponse rangeError(
    int statusCode,
    const std::string& code,
    std::uint64_t readableBytes)
{
    HttpServerResponse response = jsonError(statusCode, code);
    response.headers["Accept-Ranges"] = "bytes";
    if (statusCode == 416) {
        response.headers["Content-Range"] =
            "bytes */" + std::to_string(readableBytes);
    }
    return response;
}

} // namespace

MediaGatewayHttpServer::MediaGatewayHttpServer(
    std::unique_ptr<IHttpServer> inner,
    const MediaAccessGrantAuthenticator& authenticator,
    const MediaRouteLeaseRepository& routeLeaseRepository,
    const MediaHlsArtifactReader& artifactReader,
    std::string workspaceRoot,
    const RecordingDirectSourceRegistry* directSourceRegistry)
    : inner_(std::move(inner)),
      authenticator_(authenticator),
      routeLeaseRepository_(routeLeaseRepository),
      artifactReader_(artifactReader),
      workspaceRoot_(std::move(workspaceRoot)),
      directSourceRegistry_(directSourceRegistry)
{
}

HttpServerResponse MediaGatewayHttpServer::handleRequest(
    const HttpServerRequest& request) const
{
    const MediaPath mediaPath = parseMediaPath(request.path);
    if (!mediaPath.valid) {
        return inner_->handleRequest(request);
    }

    if (request.method != "GET") {
        return jsonError(405, "media_method_not_allowed");
    }

    const std::string credential = mediaCredential(request);
    if (credential.empty()) {
        return jsonError(401, "media_access_credential_required");
    }

    const MediaAccessGrantAuthentication authentication =
        authenticator_.authenticate(credential, mediaPath.sessionId);
    if (!authentication.authenticated) {
        return jsonError(401, authentication.reasonCode.empty()
            ? "media_access_denied"
            : authentication.reasonCode);
    }

    const auto lease = routeLeaseRepository_.findActive(
        authentication.sessionId,
        authentication.routeId,
        authentication.routeEpoch);
    if (!lease.has_value()) {
        return jsonError(409, "media_route_not_active");
    }

    if (mediaPath.live) {
        if (lease->presentationProfileId != "live-progressive-fmp4") {
            return jsonError(409, "media_presentation_not_live_stream");
        }
        const std::string streamPath = liveStreamPath(
            workspaceRoot_, lease->workspaceId);
        if (streamPath.empty()) {
            HttpServerResponse response = jsonError(404, "live_stream_not_ready");
            response.headers["Retry-After"] = "1";
            return response;
        }

        HttpServerResponse response;
        response.statusCode = 200;
        response.headers["Content-Type"] = "video/mp4";
        response.headers["Cache-Control"] = "no-store";
        response.headers["X-Content-Type-Options"] = "nosniff";
        response.headers["Cross-Origin-Resource-Policy"] = "same-origin";
        response.headers["X-Accel-Buffering"] = "no";
        response.streamBodyPath = streamPath;
        return response;
    }

    if (mediaPath.recordingDirect) {
        if (lease->presentationProfileId != "progressive-direct") {
            return jsonError(409, "media_presentation_not_progressive_direct");
        }
        if (directSourceRegistry_ == nullptr) {
            return jsonError(503, "recording_direct_source_unavailable");
        }

        const RecordingDirectSourceLookup direct =
            directSourceRegistry_->lookup(mediaPath.sessionId);
        if (!direct.available) {
            return jsonError(
                409,
                direct.reasonCode.empty()
                    ? "recording_direct_source_unavailable"
                    : direct.reasonCode);
        }

        const std::string rangeHeader = headerValue(request, "Range");
        if (rangeHeader.empty()) {
            return rangeError(
                400,
                "media_byte_range_required",
                direct.readableBytes);
        }

        const ByteRange range = parseSingleByteRange(
            rangeHeader,
            direct.readableBytes);
        if (range.state == RangeState::Malformed) {
            return rangeError(
                400,
                "media_byte_range_invalid",
                direct.readableBytes);
        }
        if (range.state == RangeState::Unsatisfiable) {
            return rangeError(
                416,
                "media_byte_range_not_satisfiable",
                direct.readableBytes);
        }

        const std::uint64_t requested64 = range.last - range.first + 1;
        const std::size_t requested = requested64 >
                static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max())
            ? std::numeric_limits<std::size_t>::max()
            : static_cast<std::size_t>(requested64);
        const RecordingByteReadResult read = directSourceRegistry_->read(
            mediaPath.sessionId,
            range.first,
            requested);
        if (!read.success || read.bytes.empty()) {
            if (read.reasonCode == "recording_range_not_satisfiable") {
                return rangeError(
                    416,
                    "media_byte_range_not_satisfiable",
                    direct.readableBytes);
            }
            if (read.reasonCode == "recording_source_changed" ||
                read.reasonCode == "recording_source_is_growing" ||
                read.reasonCode == "recording_segment_changed") {
                return jsonError(409, read.reasonCode);
            }
            return jsonError(
                503,
                read.reasonCode.empty()
                    ? "recording_direct_read_failed"
                    : read.reasonCode);
        }

        const std::uint64_t actualLast =
            range.first + static_cast<std::uint64_t>(read.bytes.size()) - 1;
        HttpServerResponse response;
        response.statusCode = 206;
        response.headers["Content-Type"] = "video/mp2t";
        response.headers["Cache-Control"] = "no-store";
        response.headers["X-Content-Type-Options"] = "nosniff";
        response.headers["Cross-Origin-Resource-Policy"] = "same-origin";
        response.headers["Accept-Ranges"] = "bytes";
        response.headers["Content-Range"] =
            "bytes " + std::to_string(range.first) + "-" +
            std::to_string(actualLast) + "/" +
            std::to_string(direct.readableBytes);
        response.body.assign(
            reinterpret_cast<const char*>(read.bytes.data()),
            read.bytes.size());
        return response;
    }

    if (lease->presentationProfileId != "hls-fmp4" &&
        lease->presentationProfileId != "hls-ts") {
        return jsonError(409, "media_presentation_not_hls");
    }

    const MediaHlsArtifact artifact = artifactReader_.read(
        lease->workspaceId,
        mediaPath.artifactName);
    if (!artifact.found) {
        const int statusCode = artifact.reasonCode == "media_artifact_not_ready"
            ? 404
            : 500;
        HttpServerResponse response = jsonError(
            statusCode,
            artifact.reasonCode.empty()
                ? "media_artifact_unavailable"
                : artifact.reasonCode);
        if (statusCode == 404) response.headers["Retry-After"] = "1";
        return response;
    }

    HttpServerResponse response;
    response.statusCode = 200;
    response.headers["Content-Type"] = artifact.contentType;
    response.headers["Cache-Control"] = "no-store";
    response.headers["X-Content-Type-Options"] = "nosniff";
    response.headers["Cross-Origin-Resource-Policy"] = "same-origin";
    response.body.assign(
        reinterpret_cast<const char*>(artifact.bytes.data()),
        artifact.bytes.size());
    return response;
}
