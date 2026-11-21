#pragma once

#include "RuntimeConfig.h"

#include <cstddef>
#include <string>

struct TvmazeSeriesArtworkRuntimeConfig
{
    bool selected = false;
    std::string incomingRoot;
    int connectTimeoutMs = 2000;
    int totalTimeoutMs = 8000;
    int maximumRetries = 1;
    int retryBackoffMs = 500;
    int negativeCacheTtlSeconds = 6 * 60 * 60;
    int transientCacheTtlSeconds = 5 * 60;
    std::size_t maximumJsonBytes = 2U * 1024U * 1024U;
    std::size_t maximumImageBytes = 16U * 1024U * 1024U;

    bool usable() const;

    static TvmazeSeriesArtworkRuntimeConfig fromEnvironment(
        const RuntimeSeriesArtworkFallbackConfig& fallbackConfig);
};
