#pragma once

#include "ArtworkCacheValidation.h"
#include "AuthorizationService.h"
#include "RestQueryParameters.h"

namespace vdrsuite::http
{
// An opaque cache partition, not a credential or new permission. Binding it
// to the actual authenticated session prevents reusing a previous login's
// cached projection even if the actor and effective grant set are identical.
inline std::string browserHomeCacheScope(const RequestSecurityContext& context,
                                         const std::string& backend)
{
    if (!context.authenticated() || !context.session || context.session->sessionId.empty() ||
        context.actor.type != ActorType::User ||
        context.permissionGrantResolution != PermissionGrantResolutionState::Resolved) return {};
    std::string identity = "browser-session-v2";
    appendArtworkField(identity, context.actor.actorId);
    appendArtworkField(identity, context.session->sessionId);
    std::vector<std::string> grants;
    for (const auto& grant : context.grants) {
        std::string tuple;
        appendArtworkField(tuple, grant.permission);
        appendArtworkField(tuple, grant.backendId);
        grants.push_back(std::move(tuple));
    }
    return artworkCacheScope(identity, backend, std::move(grants));
}

// Cache-only canonical query scope. Ambiguous query identities stay online.
inline std::string canonicalHomeCacheBackend(const std::string& raw)
{
    std::size_t backends = 0, backendIds = 0;
    for (std::size_t start = 0; start < raw.size();) {
        const auto end = raw.find('&', start);
        const auto item = raw.substr(start, end == std::string::npos ? end : end - start);
        const auto key = item.substr(0, item.find('='));
        if (key.find('%') != std::string::npos) return {};
        if (key == "backend") ++backends;
        if (key == "backendId") ++backendIds;
        if (end == std::string::npos) break;
        start = end + 1;
    }
    const auto query = RestQueryParameters::parse(raw);
    const auto backend = query.get("backend");
    if (backends != 1 || backendIds > 1 || (backendIds && query.get("backendId") != backend)) return {};
    return backend;
}

// The direct artwork service owns the bytes after the HTTP router's
// 404 fallback. Only canonical Home previews may carry a cache partition
// across that existing boundary.
inline std::string directHomeArtworkBackend(const std::string& target)
{
    const std::string prefix = "/recording-artwork/";
    const std::string suffix = "?variant=home";
    if (target.compare(0, prefix.size(), prefix) != 0 ||
        target.size() <= prefix.size() + suffix.size() ||
        target.compare(target.size() - suffix.size(), suffix.size(), suffix) != 0) return {};
    const auto end = target.find('/', prefix.size());
    const auto query = target.find('?');
    if (end == std::string::npos || query == std::string::npos || end >= query ||
        query - end - 1 != 32U || target.substr(query) != suffix) return {};
    const auto backend = target.substr(prefix.size(), end - prefix.size());
    const auto asset = target.substr(end + 1, query - end - 1);
    if (backend.empty() || backend.size() > 128U || backend == "." || backend == ".." ||
        !std::all_of(backend.begin(), backend.end(), [](unsigned char c) {
            return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
                (c >= '0' && c <= '9') || c == '_' || c == '-' || c == '.';
        }) ||
        !std::all_of(asset.begin(), asset.end(), [](unsigned char c) {
            return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') ||
                (c >= 'A' && c <= 'F');
        })) return {};
    return backend;
}

inline void addDirectHomeArtworkFallbackScope(const HttpServerRequest& request,
    HttpServerResponse& response, const RequestSecurityContext& context, bool browserAuthenticated)
{
    if (request.method != "GET" || response.statusCode != 404 || !browserAuthenticated) return;
    const auto backend = directHomeArtworkBackend(request.path);
    if (backend.empty() || !AuthorizationService{}.authorize(context,
        {"recordings.view", backend, "recordings.view"}).allowed) return;
    const auto partition = browserHomeCacheScope(context, backend);
    if (partition.empty()) return;
    // Opaque cache eligibility only. The decorator must resolve the actual
    // asset and generate a validator for the final representation.
    response.headers["X-VDR-Suite-Artwork-Scope"] = partition;
    response.headers["Cache-Control"] = "private, no-store";
    response.headers["Vary"] = "Cookie, Authorization";
}

// Cache-only grammar matching the selected cover URL emitted by the series
// service and the Home preview helper. Never change legacy route admission.
inline std::string seriesHomeArtworkBackend(const std::string& target,
                                          const std::string& revision)
{
    if (revision.empty() || revision.size() > 19U || revision.front() == '0' ||
        !std::all_of(revision.begin(), revision.end(), [](unsigned char c) {
            return c >= '0' && c <= '9';
        })) return {};
    const std::string prefix = "/api/backends/";
    const std::string suffix = "/settings/series-artwork/image";
    const auto at = target.find('?');
    if (at == std::string::npos || target.size() > 4096U) return {};
    const auto path = target.substr(0, at);
    if (path.size() <= prefix.size() + suffix.size() ||
        path.compare(0, prefix.size(), prefix) != 0 ||
        path.compare(path.size() - suffix.size(), suffix.size(), suffix) != 0) return {};
    const auto backend = path.substr(prefix.size(), path.size() - prefix.size() - suffix.size());
    if (backend.empty() || backend.size() > 128U || backend == "." || backend == ".." ||
        !std::all_of(backend.begin(), backend.end(), [](unsigned char c) {
            return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
                (c >= '0' && c <= '9') || c == '_' || c == '-' || c == '.';
        })) return {};
    const auto raw = target.substr(at + 1);
    const std::string key = "seriesKey=";
    const std::string tail = "&revision=" + revision + "&variant=home";
    if (raw.size() <= key.size() + tail.size() || raw.compare(0, key.size(), key) != 0 ||
        raw.compare(raw.size() - tail.size(), tail.size(), tail) != 0 ||
        raw.find('&') != raw.size() - tail.size()) return {};
    return backend;
}

inline void addSeriesHomeArtworkScope(const HttpServerRequest& request,
    HttpServerResponse& response, const RequestSecurityContext& context, bool browserAuthenticated)
{
    const auto marker = response.headers.find("X-VDR-Suite-Series-Cover-Revision");
    if (marker == response.headers.end()) return;
    const auto revision = marker->second;
    response.headers.erase(marker); // Internal evidence, not a public credential/header.
    if (request.method != "GET" || response.statusCode != 200 || !browserAuthenticated) return;
    const auto backend = seriesHomeArtworkBackend(request.path, revision);
    if (backend.empty() || !AuthorizationService{}.authorize(context,
        {"recordings.view", backend, "recordings.view"}).allowed) return;
    const auto partition = browserHomeCacheScope(context, backend);
    if (partition.empty()) return;
    response.headers["X-VDR-Suite-Artwork-Scope"] = partition;
    response.headers["Cache-Control"] = "private, no-cache";
    response.headers["Vary"] = "Cookie, Authorization";
}

// Cache eligibility only: the original HTTP status/body and authorization
// decision remain untouched. The opaque scope is not an access credential.
inline void addHomeCacheScope(const HttpServerRequest& request, HttpServerResponse& response,
                              const RequestSecurityContext& context, bool browserAuthenticated,
                              const std::string& authorizedBackend)
{
    if (!browserAuthenticated || !context.authenticated() || !context.session ||
        context.actor.type != ActorType::User ||
        context.permissionGrantResolution != PermissionGrantResolutionState::Resolved ||
        response.statusCode != 200) return;
    const auto at = request.path.find('?');
    const auto path = request.path.substr(0, at);
    std::string backend;
    bool recording = false, history = false;
    const std::string prefix = "/api/backends/", suffix = "/snapshot";
    if (request.method == "GET" && at == std::string::npos &&
        path.size() > prefix.size() + suffix.size() &&
        path.compare(0, prefix.size(), prefix) == 0 &&
        path.compare(path.size() - suffix.size(), suffix.size(), suffix) == 0) {
        backend = path.substr(prefix.size(), path.size() - prefix.size() - suffix.size());
        recording = history = true;
    } else if (request.method == "GET" && at != std::string::npos &&
        (path == "/api/vdr/recordings/query" || path == "/api/vdr/recordings/folder" ||
         path == "/api/metadata/genres" || path == "/api/metadata/genres/recordings")) {
        const auto raw = request.path.substr(at + 1);
        const auto query = RestQueryParameters::parse(raw);
        if (path == "/api/metadata/genres" && query.get("scope") != "recordings") return;
        backend = canonicalHomeCacheBackend(raw);
        if (backend.empty()) return;
        recording = true;
    } else if (request.method == "POST" && path == "/api/media/continue-watching" &&
               at == std::string::npos) {
        backend = authorizedBackend;
        // Only the existing first-party list read, never a mutation response.
        if (request.body != "{\"operation\":\"list\",\"backendId\":\"" + backend + "\"}") return;
        history = true;
    } else return;

    if (backend.empty() || backend.size() > 128U || backend == "." || backend == ".." ||
        !std::all_of(backend.begin(), backend.end(), [](unsigned char c) {
            return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
                (c >= '0' && c <= '9') || c == '_' || c == '-' || c == '.';
        })) return;
    const AuthorizationService authorization;
    recording = recording && authorization.authorize(context,
        {"recordings.view", backend, "recordings.view"}).allowed;
    history = history && authorization.authorize(context,
        {"media.recording.play", backend, "media.recording.play"}).allowed;
    if (!recording && !history) return;
    const auto scope = browserHomeCacheScope(context, backend);
    if (scope.empty()) return;
    response.headers["X-VDR-Suite-Home-Backend"] = backend;
    if (recording) response.headers["X-VDR-Suite-Home-Recording-Scope"] = scope;
    if (history) response.headers["X-VDR-Suite-Home-History-Scope"] = scope;
    response.headers["Cache-Control"] = "private, no-store";
}
}
