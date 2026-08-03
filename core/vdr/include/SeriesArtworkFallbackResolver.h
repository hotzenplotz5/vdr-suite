#pragma once

#include "IEpgScraperMetadataResolver.h"
#include "ISeriesArtworkFallbackProvider.h"

#include <utility>

struct SeriesArtworkFallbackResolverConfig
{
    bool enabled = false;
};

class SeriesArtworkFallbackResolver final : public IEpgScraperMetadataResolver
{
public:
    SeriesArtworkFallbackResolver(
        IEpgScraperMetadataResolver& delegate,
        ISeriesArtworkFallbackProvider* provider,
        SeriesArtworkFallbackResolverConfig config = {})
        : delegate_(delegate),
          provider_(provider),
          config_(config)
    {
    }

    EpgScraperMetadataResolution resolve(
        const std::string& backendId,
        const VdrEvent& event) override
    {
        EpgScraperMetadataResolution resolution = delegate_.resolve(
            backendId,
            event);

        if (!config_.enabled ||
            provider_ == nullptr ||
            !resolution.attempted ||
            !resolution.found ||
            !resolution.metadata.valid() ||
            resolution.metadata.mediaType != EpgScraperMediaType::Series ||
            resolution.metadata.preferredArtwork.valid() ||
            resolution.metadata.seriesArtworkFallback.available)
        {
            return resolution;
        }

        SeriesArtworkFallbackResolution fallback = provider_->resolve(
            backendId,
            event,
            resolution.metadata);
        if (!fallback.valid())
        {
            return resolution;
        }

        resolution.metadata.seriesArtworkFallback =
            std::move(fallback.artwork);
        return resolution;
    }

private:
    IEpgScraperMetadataResolver& delegate_;
    ISeriesArtworkFallbackProvider* provider_ = nullptr;
    SeriesArtworkFallbackResolverConfig config_;
};
