#pragma once

#include "IEpgScraperMetadataResolver.h"
#include "ISeriesArtworkFallbackProvider.h"

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
        SeriesArtworkFallbackResolverConfig config = {});

    EpgScraperMetadataResolution resolve(
        const std::string& backendId,
        const VdrEvent& event) override;

private:
    IEpgScraperMetadataResolver& delegate_;
    ISeriesArtworkFallbackProvider* provider_ = nullptr;
    SeriesArtworkFallbackResolverConfig config_;
};
