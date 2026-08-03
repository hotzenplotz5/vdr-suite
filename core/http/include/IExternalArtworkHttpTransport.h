#pragma once

#include <cstddef>
#include <string>

struct ExternalArtworkHttpRequest
{
    std::string url;
    std::string bearerToken;
    std::string accept;
    int connectTimeoutMs = 2000;
    int totalTimeoutMs = 8000;
    std::size_t maximumResponseBytes = 512U * 1024U;
};

struct ExternalArtworkHttpResponse
{
    bool attempted = false;
    bool transportError = false;
    long statusCode = 0;
    long retryAfterSeconds = 0;
    std::string contentType;
    std::string body;
};

class IExternalArtworkHttpTransport
{
public:
    virtual ~IExternalArtworkHttpTransport() = default;

    virtual ExternalArtworkHttpResponse perform(
        const ExternalArtworkHttpRequest& request) = 0;
};
