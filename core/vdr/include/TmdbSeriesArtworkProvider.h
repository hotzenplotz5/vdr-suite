#pragma once

#include "IExternalArtworkHttpTransport.h"
#include "ISeriesArtworkFallbackProvider.h"
#include "ISeriesArtworkProviderCache.h"

#include <chrono>
#include <cstddef>
#include <functional>
#include <string>

struct TmdbSeriesArtworkProviderConfig
{
    std::string readAccessToken;
    std::string language = "de-DE";
    std::string includeImageLanguages = "de,en,null";
    std::string incomingRoot =
        "/var/cache/vdr-suite/epg-artwork/incoming";
    int connectTimeoutMs = 2000;
    int totalTimeoutMs = 8000;
    int maximumRetries = 1;
    int retryBackoffMs = 250;
    int negativeCacheTtlSeconds = 6 * 60 * 60;
    int transientCacheTtlSeconds = 5 * 60;
    std::size_t maximumJsonBytes = 512U * 1024U;
    std::size_t maximumImageBytes = 16U * 1024U * 1024U;
};

class TmdbSeriesArtworkProvider final
    : public ISeriesArtworkFallbackProvider
{
public:
    using Clock = std::function<long long()>;
    using Sleeper = std::function<void(std::chrono::milliseconds)>;

    TmdbSeriesArtworkProvider(
        IExternalArtworkHttpTransport& transport,
        ISeriesArtworkProviderCache& cache,
        TmdbSeriesArtworkProviderConfig config,
        Clock clock = {},
        Sleeper sleeper = {});

    SeriesArtworkFallbackResolution resolve(
        const std::string& backendId,
        const VdrEvent& event,
        const EpgScraperMetadata& metadata) override;

    static bool configurationValid(
        const TmdbSeriesArtworkProviderConfig& config);

private:
    IExternalArtworkHttpTransport& transport_;
    ISeriesArtworkProviderCache& cache_;
    TmdbSeriesArtworkProviderConfig config_;
    Clock clock_;
    Sleeper sleeper_;
};
