#pragma once

#include "IEpgScraperMetadataResolver.h"
#include "ISeriesArtworkFallbackMaterializer.h"

struct SeriesArtworkFallbackMaterializingResolverConfig
{
    bool enabled = false;
};

class SeriesArtworkFallbackMaterializingResolver final
    : public IEpgScraperMetadataResolver
{
public:
    SeriesArtworkFallbackMaterializingResolver(
        IEpgScraperMetadataResolver& delegate,
        ISeriesArtworkFallbackMaterializer* materializer,
        SeriesArtworkFallbackMaterializingResolverConfig config = {})
        : delegate_(delegate),
          materializer_(materializer),
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
        EpgScraperArtwork& candidate =
            resolution.metadata.seriesArtworkFallback;

        if (!candidate.available)
        {
            return resolution;
        }

        if (!resolution.attempted ||
            !resolution.found ||
            !resolution.metadata.valid() ||
            resolution.metadata.mediaType != EpgScraperMediaType::Series)
        {
            candidate = EpgScraperArtwork{};
            return resolution;
        }

        if (!config_.enabled || materializer_ == nullptr)
        {
            candidate = EpgScraperArtwork{};
            return resolution;
        }

        SeriesArtworkFallbackMaterializationRequest request;
        request.backendId = resolution.metadata.backendId.empty()
            ? backendId
            : resolution.metadata.backendId;
        request.channelId = resolution.metadata.channelId.empty()
            ? event.channelId
            : resolution.metadata.channelId;
        request.eventId = resolution.metadata.eventId.empty()
            ? event.id
            : resolution.metadata.eventId;
        request.candidate = candidate;

        const SeriesArtworkFallbackMaterializationResult materialized =
            materializer_->materialize(request);
        candidate = materialized.valid()
            ? materialized.artwork
            : EpgScraperArtwork{};
        return resolution;
    }

private:
    IEpgScraperMetadataResolver& delegate_;
    ISeriesArtworkFallbackMaterializer* materializer_ = nullptr;
    SeriesArtworkFallbackMaterializingResolverConfig config_;
};
