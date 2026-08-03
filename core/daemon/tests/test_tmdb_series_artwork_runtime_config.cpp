#include "TmdbSeriesArtworkRuntimeConfig.h"

#include <cassert>
#include <cstdlib>
#include <vector>

namespace
{
const std::vector<const char*> Variables = {
    "VDR_SUITE_SERIES_ARTWORK_FALLBACK_PROVIDER",
    "VDR_SUITE_TMDB_READ_ACCESS_TOKEN",
    "VDR_SUITE_TMDB_LANGUAGE",
    "VDR_SUITE_TMDB_INCLUDE_IMAGE_LANGUAGES",
    "VDR_SUITE_TMDB_CONNECT_TIMEOUT_MS",
    "VDR_SUITE_TMDB_TOTAL_TIMEOUT_MS",
    "VDR_SUITE_TMDB_MAX_RETRIES",
    "VDR_SUITE_TMDB_RETRY_BACKOFF_MS",
    "VDR_SUITE_TMDB_NEGATIVE_CACHE_TTL_SECONDS",
    "VDR_SUITE_TMDB_TRANSIENT_CACHE_TTL_SECONDS",
    "VDR_SUITE_TMDB_MAX_JSON_BYTES"
};

void clearEnvironment()
{
    for (const char* variable : Variables) unsetenv(variable);
}
}

int main()
{
    RuntimeSeriesArtworkFallbackConfig fallback;
    clearEnvironment();
    auto defaults = TmdbSeriesArtworkRuntimeConfig::fromEnvironment(fallback);
    assert(!defaults.selected);
    assert(!defaults.usable());
    assert(defaults.readAccessToken.empty());
    assert(defaults.incomingRoot == fallback.sourceRoots.front());
    assert(defaults.maximumImageBytes ==
           static_cast<std::size_t>(fallback.maximumSourceBytes));

    setenv("VDR_SUITE_SERIES_ARTWORK_FALLBACK_PROVIDER", "TMDB", 1);
    setenv("VDR_SUITE_TMDB_READ_ACCESS_TOKEN", "valid.token-123", 1);
    setenv("VDR_SUITE_TMDB_LANGUAGE", "en-GB", 1);
    setenv("VDR_SUITE_TMDB_INCLUDE_IMAGE_LANGUAGES", "en,de,null", 1);
    setenv("VDR_SUITE_TMDB_CONNECT_TIMEOUT_MS", "900", 1);
    setenv("VDR_SUITE_TMDB_TOTAL_TIMEOUT_MS", "4000", 1);
    setenv("VDR_SUITE_TMDB_MAX_RETRIES", "2", 1);
    setenv("VDR_SUITE_TMDB_RETRY_BACKOFF_MS", "100", 1);
    setenv("VDR_SUITE_TMDB_NEGATIVE_CACHE_TTL_SECONDS", "3600", 1);
    setenv("VDR_SUITE_TMDB_TRANSIENT_CACHE_TTL_SECONDS", "120", 1);
    setenv("VDR_SUITE_TMDB_MAX_JSON_BYTES", "262144", 1);
    auto configured = TmdbSeriesArtworkRuntimeConfig::fromEnvironment(fallback);
    assert(configured.selected);
    assert(configured.usable());
    assert(configured.language == "en-GB");
    assert(configured.includeImageLanguages == "en,de,null");
    assert(configured.connectTimeoutMs == 900);
    assert(configured.totalTimeoutMs == 4000);
    assert(configured.maximumRetries == 2);
    assert(configured.retryBackoffMs == 100);
    assert(configured.negativeCacheTtlSeconds == 3600);
    assert(configured.transientCacheTtlSeconds == 120);
    assert(configured.maximumJsonBytes == 262144U);

    setenv("VDR_SUITE_TMDB_READ_ACCESS_TOKEN", "bad token", 1);
    setenv("VDR_SUITE_TMDB_TOTAL_TIMEOUT_MS", "100", 1);
    setenv("VDR_SUITE_TMDB_MAX_RETRIES", "99", 1);
    setenv("VDR_SUITE_TMDB_MAX_JSON_BYTES", "1", 1);
    auto invalid = TmdbSeriesArtworkRuntimeConfig::fromEnvironment(fallback);
    assert(invalid.totalTimeoutMs == 8000);
    assert(invalid.maximumRetries == 1);
    assert(invalid.maximumJsonBytes == 512U * 1024U);
    assert(!invalid.usable());

    fallback.sourceRoots.clear();
    clearEnvironment();
    setenv("VDR_SUITE_SERIES_ARTWORK_FALLBACK_PROVIDER", "tmdb", 1);
    setenv("VDR_SUITE_TMDB_READ_ACCESS_TOKEN", "valid.token", 1);
    auto missingRoot = TmdbSeriesArtworkRuntimeConfig::fromEnvironment(fallback);
    assert(!missingRoot.usable());

    clearEnvironment();
    return 0;
}
