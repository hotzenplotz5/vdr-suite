#pragma once

#include "RuntimeConfig.h"

#include <cstddef>
#include <string>

struct TmdbSeriesArtworkRuntimeConfig
{
    bool selected = false;
    std::string readAccessToken;
    std::string language = "de-DE";
    std::string includeImageLanguages = "de,en,null";
    std::string incomingRoot;
    int connectTimeoutMs = 2000;
    int totalTimeoutMs = 8000;
    int maximumRetries = 1;
    int retryBackoffMs = 250;
    int negativeCacheTtlSeconds = 6 * 60 * 60;
    int transientCacheTtlSeconds = 5 * 60;
    std::size_t maximumJsonBytes = 512U * 1024U;
    std::size_t maximumImageBytes = 16U * 1024U * 1024U;

    bool usable() const;

    static TmdbSeriesArtworkRuntimeConfig fromEnvironment(
        const RuntimeSeriesArtworkFallbackConfig& fallbackConfig);
};
