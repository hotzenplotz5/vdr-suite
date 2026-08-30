#pragma once

#include "HttpServerRequest.h"

#include <string>

namespace ContinueWatchingSecurityRequest
{

inline bool matches(const HttpServerRequest& request)
{
    if (request.method != "POST") return false;
    const std::size_t query = request.path.find('?');
    const std::string path = query == std::string::npos
        ? request.path
        : request.path.substr(0, query);
    return path == "/api/media/continue-watching" ||
        path == "/api/media/recently-watched";
}

inline HttpServerRequest forAuthorization(const HttpServerRequest& request)
{
    if (!matches(request)) return request;

    // Continue Watching and Recently Watched are bounded first-party Recording
    // playback state/evidence. Authorize both through the already accepted
    // Recording MediaSession permission/CSRF contract while preserving the
    // original request for API dispatch.
    HttpServerRequest scoped = request;
    scoped.path = "/api/media/sessions";
    return scoped;
}

} // namespace ContinueWatchingSecurityRequest
