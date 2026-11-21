#include "TvmazeSeriesArtworkRuntimeConfig.h"

#include <cassert>
#include <cstdlib>
#include <vector>

namespace
{
const std::vector<const char*> Variables = {
    "VDR_SUITE_SERIES_ARTWORK_FALLBACK_PROVIDER",
    "VDR_SUITE_TVMAZE_CONNECT_TIMEOUT_MS",
    "VDR_SUITE_TVMAZE_TOTAL_TIMEOUT_MS",
    "VDR_SUITE_TVMAZE_MAX_RETRIES",
    "VDR_SUITE_TVMAZE_RETRY_BACKOFF_MS",
    "VDR_SUITE_TVMAZE_NEGATIVE_CACHE_TTL_SECONDS",
    "VDR_SUITE_TVMAZE_TRANSIENT_CACHE_TTL_SECONDS",
    "VDR_SUITE_TVMAZE_MAX_JSON_BYTES"
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

    auto defaults =
        TvmazeSeriesArtworkRuntimeConfig::fromEnvironment(fallback);
    assert(defaults.selected);
    assert(defaults.usable());
    assert(defaults.incomingRoot == fallback.sourceRoots.front());
    assert(defaults.connectTimeoutMs == 2000);
    assert(defaults.totalTimeoutMs == 8000);
    assert(defaults.maximumRetries == 1);
    assert(defaults.retryBackoffMs == 500);
    assert(defaults.maximumJsonBytes == 2U * 1024U * 1024U);
    assert(defaults.maximumImageBytes ==
           static_cast<std::size_t>(fallback.maximumSourceBytes));

    setenv("VDR_SUITE_SERIES_ARTWORK_FALLBACK_PROVIDER", "TMDB", 1);
    auto notSelected =
        TvmazeSeriesArtworkRuntimeConfig::fromEnvironment(fallback);
    assert(!notSelected.selected);
    assert(!notSelected.usable());

    setenv("VDR_SUITE_SERIES_ARTWORK_FALLBACK_PROVIDER", "TVMAZE", 1);
    setenv("VDR_SUITE_TVMAZE_CONNECT_TIMEOUT_MS", "900", 1);
    setenv("VDR_SUITE_TVMAZE_TOTAL_TIMEOUT_MS", "4000", 1);
    setenv("VDR_SUITE_TVMAZE_MAX_RETRIES", "2", 1);
    setenv("VDR_SUITE_TVMAZE_RETRY_BACKOFF_MS", "1000", 1);
    setenv("VDR_SUITE_TVMAZE_NEGATIVE_CACHE_TTL_SECONDS", "3600", 1);
    setenv("VDR_SUITE_TVMAZE_TRANSIENT_CACHE_TTL_SECONDS", "120", 1);
    setenv("VDR_SUITE_TVMAZE_MAX_JSON_BYTES", "1048576", 1);

    auto configured =
        TvmazeSeriesArtworkRuntimeConfig::fromEnvironment(fallback);
    assert(configured.selected);
    assert(configured.usable());
    assert(configured.connectTimeoutMs == 900);
    assert(configured.totalTimeoutMs == 4000);
    assert(configured.maximumRetries == 2);
    assert(configured.retryBackoffMs == 1000);
    assert(configured.negativeCacheTtlSeconds == 3600);
    assert(configured.transientCacheTtlSeconds == 120);
    assert(configured.maximumJsonBytes == 1048576U);

    setenv("VDR_SUITE_TVMAZE_TOTAL_TIMEOUT_MS", "100", 1);
    setenv("VDR_SUITE_TVMAZE_MAX_RETRIES", "99", 1);
    setenv("VDR_SUITE_TVMAZE_MAX_JSON_BYTES", "1", 1);
    auto invalid =
        TvmazeSeriesArtworkRuntimeConfig::fromEnvironment(fallback);
    assert(invalid.totalTimeoutMs == 8000);
    assert(invalid.maximumRetries == 1);
    assert(invalid.maximumJsonBytes == 2U * 1024U * 1024U);
    assert(invalid.usable());

    fallback.sourceRoots.clear();
    clearEnvironment();
    auto missingRoot =
        TvmazeSeriesArtworkRuntimeConfig::fromEnvironment(fallback);
    assert(!missingRoot.usable());

    clearEnvironment();
    return 0;
}
