#pragma once

#include "EpgScraperMetadata.h"

#include <filesystem>
#include <string>

struct SeriesArtworkFallbackMaterializationRequest
{
    std::string backendId;
    std::string channelId;
    std::string eventId;
    EpgScraperArtwork candidate;

    bool valid() const
    {
        return !backendId.empty() &&
            !channelId.empty() &&
            !eventId.empty() &&
            candidate.available &&
            candidate.origin == EpgScraperArtworkOrigin::ExternalFallback &&
            !candidate.provider.empty() &&
            candidate.provider != "none" &&
            candidate.provider != "tvscraper" &&
            std::filesystem::path(candidate.path).is_absolute() &&
            candidate.width > 0 &&
            candidate.height > 0;
    }
};

struct SeriesArtworkFallbackMaterializationResult
{
    bool attempted = false;
    bool stored = false;
    EpgScraperArtwork artwork;

    bool valid() const
    {
        return attempted &&
            stored &&
            artwork.available &&
            artwork.origin == EpgScraperArtworkOrigin::ExternalFallback &&
            !artwork.provider.empty() &&
            artwork.provider != "none" &&
            artwork.provider != "tvscraper" &&
            std::filesystem::path(artwork.path).is_absolute() &&
            artwork.width > 0 &&
            artwork.height > 0;
    }
};

class ISeriesArtworkFallbackMaterializer
{
public:
    virtual ~ISeriesArtworkFallbackMaterializer() = default;

    virtual SeriesArtworkFallbackMaterializationResult materialize(
        const SeriesArtworkFallbackMaterializationRequest& request) = 0;
};
