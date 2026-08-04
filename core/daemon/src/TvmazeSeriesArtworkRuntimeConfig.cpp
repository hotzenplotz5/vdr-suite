#include "TvmazeSeriesArtworkRuntimeConfig.h"

#include "TvmazeSeriesArtworkProvider.h"

#include <algorithm>
#include <cerrno>
#include <cctype>
#include <cstdlib>
#include <string>

namespace
{
std::string environmentOrEmpty(const char* name)
{
    const char* value = std::getenv(name);
    return value == nullptr ? std::string() : std::string(value);
}

std::string environmentOrDefault(
    const char* name,
    const std::string& fallback)
{
    const std::string value = environmentOrEmpty(name);
    return value.empty() ? fallback : value;
}

std::string lowercase(std::string value)
{
    std::transform(
        value.begin(),
        value.end(),
        value.begin(),
        [](const unsigned char character) {
            return static_cast<char>(std::tolower(character));
        });
    return value;
}

int environmentInteger(
    const char* name,
    int fallback,
    int minimum,
    int maximum)
{
    const char* value = std::getenv(name);
    if (value == nullptr || *value == '\0') return fallback;

    errno = 0;
    char* end = nullptr;
    const long parsed = std::strtol(value, &end, 10);
    if (errno != 0 || end == value || end == nullptr || *end != '\0' ||
        parsed < minimum || parsed > maximum)
    {
        return fallback;
    }
    return static_cast<int>(parsed);
}

std::size_t environmentSize(
    const char* name,
    std::size_t fallback,
    std::size_t minimum,
    std::size_t maximum)
{
    const char* value = std::getenv(name);
    if (value == nullptr || *value == '\0') return fallback;

    errno = 0;
    char* end = nullptr;
    const unsigned long long parsed = std::strtoull(value, &end, 10);
    if (errno != 0 || end == value || end == nullptr || *end != '\0' ||
        parsed < minimum || parsed > maximum)
    {
        return fallback;
    }
    return static_cast<std::size_t>(parsed);
}
}

bool TvmazeSeriesArtworkRuntimeConfig::usable() const
{
    TvmazeSeriesArtworkProviderConfig provider;
    provider.incomingRoot = incomingRoot;
    provider.connectTimeoutMs = connectTimeoutMs;
    provider.totalTimeoutMs = totalTimeoutMs;
    provider.maximumRetries = maximumRetries;
    provider.retryBackoffMs = retryBackoffMs;
    provider.negativeCacheTtlSeconds = negativeCacheTtlSeconds;
    provider.transientCacheTtlSeconds = transientCacheTtlSeconds;
    provider.maximumJsonBytes = maximumJsonBytes;
    provider.maximumImageBytes = maximumImageBytes;
    return selected &&
        TvmazeSeriesArtworkProvider::configurationValid(provider);
}

TvmazeSeriesArtworkRuntimeConfig
TvmazeSeriesArtworkRuntimeConfig::fromEnvironment(
    const RuntimeSeriesArtworkFallbackConfig& fallbackConfig)
{
    TvmazeSeriesArtworkRuntimeConfig value;
    value.selected = lowercase(environmentOrDefault(
        "VDR_SUITE_SERIES_ARTWORK_FALLBACK_PROVIDER",
        "tvmaze")) == "tvmaze";
    value.incomingRoot = fallbackConfig.sourceRoots.empty()
        ? std::string()
        : fallbackConfig.sourceRoots.front();
    value.connectTimeoutMs = environmentInteger(
        "VDR_SUITE_TVMAZE_CONNECT_TIMEOUT_MS",
        value.connectTimeoutMs,
        100,
        10000);
    value.totalTimeoutMs = environmentInteger(
        "VDR_SUITE_TVMAZE_TOTAL_TIMEOUT_MS",
        value.totalTimeoutMs,
        100,
        30000);
    if (value.totalTimeoutMs < value.connectTimeoutMs)
        value.totalTimeoutMs = std::max(8000, value.connectTimeoutMs);

    value.maximumRetries = environmentInteger(
        "VDR_SUITE_TVMAZE_MAX_RETRIES",
        value.maximumRetries,
        0,
        2);
    value.retryBackoffMs = environmentInteger(
        "VDR_SUITE_TVMAZE_RETRY_BACKOFF_MS",
        value.retryBackoffMs,
        50,
        5000);
    value.negativeCacheTtlSeconds = environmentInteger(
        "VDR_SUITE_TVMAZE_NEGATIVE_CACHE_TTL_SECONDS",
        value.negativeCacheTtlSeconds,
        60,
        7 * 24 * 60 * 60);
    value.transientCacheTtlSeconds = environmentInteger(
        "VDR_SUITE_TVMAZE_TRANSIENT_CACHE_TTL_SECONDS",
        value.transientCacheTtlSeconds,
        10,
        3600);
    value.maximumJsonBytes = environmentSize(
        "VDR_SUITE_TVMAZE_MAX_JSON_BYTES",
        value.maximumJsonBytes,
        1024U,
        4U * 1024U * 1024U);
    value.maximumImageBytes = std::min(
        value.maximumImageBytes,
        static_cast<std::size_t>(
            std::max(0, fallbackConfig.maximumSourceBytes)));
    return value;
}
