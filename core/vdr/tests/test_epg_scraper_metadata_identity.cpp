#include "EpgScraperMetadata.h"

#include <cassert>

int main()
{
    EpgScraperExternalId seriesImdb;
    seriesImdb.provider = EpgScraperExternalIdProvider::Imdb;
    seriesImdb.scope = EpgScraperExternalIdScope::Series;
    seriesImdb.value = "tt1234567";
    assert(seriesImdb.valid());

    EpgScraperExternalId unqualified;
    unqualified.value = "tt1234567";
    assert(!unqualified.valid());

    EpgScraperArtwork primary;
    primary.available = true;
    primary.provider = "tvscraper";
    primary.origin = EpgScraperArtworkOrigin::PrimaryMetadata;
    primary.path = "/cache/primary.jpg";
    primary.width = 1280;
    primary.height = 720;
    assert(primary.valid());

    EpgScraperArtwork fallback = primary;
    fallback.provider = "tmdb";
    fallback.origin = EpgScraperArtworkOrigin::ExternalFallback;
    assert(!fallback.valid());

    return 0;
}
