#include "SeriesArtworkFallbackResolver.h"

#include <utility>

SeriesArtworkFallbackResolver::SeriesArtworkFallbackResolver(
    IEpgScraperMetadataResolver& delegate,
    ISeriesArtworkFallbackProvider* provider,
    SeriesArtworkFallbackResolverConfig config)
    : delegate_(delegate),
      provider_(provider),
      config_(config)
{
}

EpgScraperMetadataResolution SeriesArtworkFallbackResolver::resolve(
    const std::string& backendId,
    const VdrEvent& event)
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

    resolution.metadata.seriesArtworkFallback = std::move(fallback.artwork);
    return resolution;
}
