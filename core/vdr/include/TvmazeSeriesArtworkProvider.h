#pragma once

#include "IExternalArtworkHttpTransport.h"
#include "ISeriesArtworkFallbackProvider.h"
#include "ISeriesArtworkProviderCache.h"

#include <chrono>
#include <cstddef>
#include <functional>
#include <string>

struct TvmazeSeriesArtworkProviderConfig
{
    std::string incomingRoot =
        "/var/cache/vdr-suite/epg-artwork/incoming";
    int connectTimeoutMs = 2000;
    int totalTimeoutMs = 8000;
    int maximumRetries = 1;
    int retryBackoffMs = 500;
    int negativeCacheTtlSeconds = 6 * 60 * 60;
    int transientCacheTtlSeconds = 5 * 60;
    std::size_t maximumJsonBytes = 2U * 1024U * 1024U;
    std::size_t maximumImageBytes = 16U * 1024U * 1024U;
};

class TvmazeSeriesArtworkProvider final
    : public ISeriesArtworkFallbackProvider
{
public:
    using Clock = std::function<long long()>;
    using Sleeper = std::function<void(std::chrono::milliseconds)>;

    TvmazeSeriesArtworkProvider(
        IExternalArtworkHttpTransport& transport,
        ISeriesArtworkProviderCache& cache,
        TvmazeSeriesArtworkProviderConfig config,
        Clock clock = {},
        Sleeper sleeper = {});

    SeriesArtworkFallbackResolution resolve(
        const std::string& backendId,
        const VdrEvent& event,
        const EpgScraperMetadata& metadata) override;

    static bool configurationValid(
        const TvmazeSeriesArtworkProviderConfig& config);

private:
    IExternalArtworkHttpTransport& transport_;
    ISeriesArtworkProviderCache& cache_;
    TvmazeSeriesArtworkProviderConfig config_;
    Clock clock_;
    Sleeper sleeper_;
};
