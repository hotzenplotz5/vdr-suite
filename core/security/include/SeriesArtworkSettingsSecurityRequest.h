#pragma once

#include "HttpServerRequest.h"

#include <algorithm>
#include <string>

namespace SeriesArtworkSettingsSecurityRequest
{
struct RouteScope
{
    bool matched = false;
    std::string backendId;
};

inline bool validBackendId(const std::string& backendId)
{
    return !backendId.empty() && backendId.size() <= 128U &&
        std::all_of(
            backendId.begin(),
            backendId.end(),
            [](unsigned char character)
            {
                return (character >= 'a' && character <= 'z') ||
                    (character >= 'A' && character <= 'Z') ||
                    (character >= '0' && character <= '9') ||
                    character == '-' || character == '_' || character == '.';
            });
}

inline RouteScope routeScope(const std::string& target)
{
    static const std::string prefix = "/api/backends/";
    static const std::string suffix = "/settings/series-artwork";

    const std::size_t query = target.find('?');
    const std::string path = query == std::string::npos
        ? target
        : target.substr(0, query);

    RouteScope scope;
    if (path.size() < prefix.size() + suffix.size() ||
        path.compare(0, prefix.size(), prefix) != 0 ||
        path.compare(
            path.size() - suffix.size(),
            suffix.size(),
            suffix) != 0)
    {
        return scope;
    }

    scope.matched = true;
    const std::size_t backendLength =
        path.size() - prefix.size() - suffix.size();
    const std::string backendId =
        path.substr(prefix.size(), backendLength);
    if (validBackendId(backendId))
    {
        scope.backendId = backendId;
    }
    return scope;
}

inline HttpServerRequest forAuthorization(const HttpServerRequest& request)
{
    if (request.method != "POST")
    {
        return request;
    }

    const RouteScope scope = routeScope(request.path);
    if (!scope.matched)
    {
        return request;
    }

    HttpServerRequest scoped = request;
    const std::string injected =
        "\"backendId\":\"" + scope.backendId + "\"";
    const std::size_t objectStart =
        scoped.body.find_first_not_of(" \t\r\n");
    if (objectStart == std::string::npos || scoped.body[objectStart] != '{')
    {
        scoped.body = "{" + injected + "}";
        return scoped;
    }

    const std::size_t firstContent =
        scoped.body.find_first_not_of(" \t\r\n", objectStart + 1U);
    const bool emptyObject =
        firstContent != std::string::npos && scoped.body[firstContent] == '}';
    scoped.body.insert(
        objectStart + 1U,
        injected + (emptyObject ? std::string() : std::string(",")));
    return scoped;
}
}
