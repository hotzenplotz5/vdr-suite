#include "ISeriesArtworkFallbackProvider.h"

#include <cassert>

namespace
{

SeriesArtworkFallbackResolution validResolution()
{
    SeriesArtworkFallbackResolution resolution;
    resolution.attempted = true;
    resolution.found = true;
    resolution.artwork.available = true;
    resolution.artwork.provider = "example-provider";
    resolution.artwork.origin = EpgScraperArtworkOrigin::ExternalFallback;
    resolution.artwork.path = "/var/cache/vdr-suite/epg-artwork/poster.jpg";
    resolution.artwork.width = 600;
    resolution.artwork.height = 900;
    return resolution;
}

}

int main()
{
    assert(validResolution().valid());

    {
        SeriesArtworkFallbackResolution resolution = validResolution();
        resolution.artwork.path = "relative/poster.jpg";
        assert(!resolution.valid());
    }

    {
        SeriesArtworkFallbackResolution resolution = validResolution();
        resolution.artwork.path = "https://images.example/poster.jpg";
        assert(!resolution.valid());
    }

    {
        SeriesArtworkFallbackResolution resolution = validResolution();
        resolution.artwork.provider = "tvscraper";
        assert(!resolution.valid());
    }

    {
        SeriesArtworkFallbackResolution resolution = validResolution();
        resolution.artwork.origin = EpgScraperArtworkOrigin::PrimaryMetadata;
        assert(!resolution.valid());
    }

    {
        SeriesArtworkFallbackResolution resolution = validResolution();
        resolution.found = false;
        assert(!resolution.valid());
    }

    return 0;
}
