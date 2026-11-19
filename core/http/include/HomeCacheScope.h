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
