#include "MediaGatewayHttpServer.h"

#include "MediaAccessCredentialHttp.h"
#include "MediaAccessGrantAuthenticator.h"
#include "MediaHlsArtifactReader.h"
#include "MediaRouteLeaseRepository.h"

#include <algorithm>
#include <cctype>
#include <string>
#include <utility>

namespace
{

constexpr const char* Prefix = "/api/media/sessions/";
constexpr const char* HlsMarker = "/hls/";

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
    std::string sessionId;
    std::string artifactName;
};

MediaPath parseMediaPath(const std::string& path)
{
    MediaPath result;
    const std::string prefix(Prefix);
    if (path.rfind(prefix, 0) != 0) return result;

    const std::size_t marker = path.find(HlsMarker, prefix.size());
    if (marker == std::string::npos) return result;
    if (path.find('?', marker) != std::string::npos ||
        path.find('#', marker) != std::string::npos) return result;

    result.sessionId = path.substr(prefix.size(), marker - prefix.size());
    result.artifactName = path.substr(marker + std::string(HlsMarker).size());
    result.valid = safeIdentifier(result.sessionId) &&
        MediaHlsArtifactReader::allowedArtifactName(result.artifactName);
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

} // namespace

MediaGatewayHttpServer::MediaGatewayHttpServer(
    std::unique_ptr<IHttpServer> inner,
    const MediaAccessGrantAuthenticator& authenticator,
    const MediaRouteLeaseRepository& routeLeaseRepository,
    const MediaHlsArtifactReader& artifactReader)
    : inner_(std::move(inner)),
      authenticator_(authenticator),
      routeLeaseRepository_(routeLeaseRepository),
      artifactReader_(artifactReader)
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