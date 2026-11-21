#pragma once

#include "EpgScraperMetadata.h"
#include "VdrEvent.h"

#include <filesystem>
#include <string>

struct SeriesArtworkFallbackResolution
{
    bool attempted = false;
    bool found = false;
    EpgScraperArtwork artwork;

    bool valid() const
    {
        return attempted && found && artwork.available &&
            artwork.origin == EpgScraperArtworkOrigin::ExternalFallback &&
            !artwork.provider.empty() && artwork.provider != "none" &&
            artwork.provider != "tvscraper" && !artwork.path.empty() &&
            std::filesystem::path(artwork.path).is_absolute() &&
            artwork.width > 0 && artwork.height > 0;
    }
};

class ISeriesArtworkFallbackProvider
{
public:
    virtual ~ISeriesArtworkFallbackProvider() = default;

    virtual SeriesArtworkFallbackResolution resolve(
        const std::string& backendId,
        const VdrEvent& event,
        const EpgScraperMetadata& metadata) = 0;
};
