#include "EpgScraperMetadataPublicJsonSerializer.h"

#include <cassert>
#include <string>

namespace
{

bool contains(const std::string& text, const std::string& value)
{
    return text.find(value) != std::string::npos;
}

EpgScraperArtwork artwork(
    const std::string& path,
    int width,
    int height)
{
    EpgScraperArtwork value;
    value.available = true;
    value.provider = "tvscraper";
    value.path = path;
    value.width = width;
    value.height = height;
    return value;
}

EpgScraperArtwork fallbackArtwork(
    const std::string& path,
    int width,
    int height)
{
    EpgScraperArtwork value;
    value.available = true;
    value.provider = "example-provider";
    value.origin = EpgScraperArtworkOrigin::ExternalFallback;
    value.path = path;
    value.width = width;
    value.height = height;
    return value;
}

}

int main()
{
    EpgScraperMetadataPublicJsonSerializer serializer;

    {
        EpgScraperMetadataResolution resolution;
        const std::string json = serializer.serialize(resolution);
        assert(json ==
            "{\"available\":false,\"status\":\"unavailable\"}");
    }

    {
        EpgScraperMetadataResolution resolution;
        resolution.attempted = true;
        const std::string json = serializer.serialize(resolution);
        assert(json ==
            "{\"available\":false,\"status\":\"not-found\"}");
    }

    {
        EpgScraperMetadataResolution resolution;
        resolution.attempted = true;
        resolution.found = true;

        EpgScraperMetadata& metadata = resolution.metadata;
        metadata.backendId = "parents vdr";
        metadata.channelId = "S19.2E-1-1011-11100";
        metadata.eventId = "12345";
        metadata.provider = "tvscraper";
        metadata.mediaType = EpgScraperMediaType::Series;
        metadata.providerId = 77;
        metadata.seasonNumber = 3;
        metadata.episodeNumber = 12;
        metadata.absoluteEpisodeNumber = 44;
        metadata.runtimeMinutes = 48;
        metadata.durationDeviationMinutes = -2;
        metadata.scraperHd = 1;
        metadata.scraperLanguage = 2;
        metadata.popularity = 12.5;
        metadata.voteAverage = 8.4;
        metadata.voteCount = 123;
        metadata.collectionId = 9;
        metadata.lastSeason = 5;
        metadata.title = "Testserie";
        metadata.originalTitle = "Test Series";
        metadata.episodeName = "Die Folge";
        metadata.tagline = "Eine Zeile\nmit Umbruch";
        metadata.overview = "Beschreibung mit \"Zitat\"";
        metadata.releaseDate = "2026-07-20";
        metadata.firstAired = "2026-07-19";
        metadata.imdbId = "tt1234567";
        metadata.status = "Returning Series";
        metadata.collectionName = "Test Collection";
        metadata.genres = {"Drama", "Mystery"};
        metadata.productionCountries = {"Deutschland"};
        metadata.networks = {"ZDF"};
        metadata.preferredArtwork = artwork(
            "/var/cache/vdr/plugins/tvscraper/private/preferred.jpg",
            1280,
            720);
        metadata.seriesArtworkFallback = fallbackArtwork(
            "/var/cache/vdr-suite/epg-artwork/private/fallback.jpg",
            600,
            900);

        EpgScraperPerson person;
        person.role = EpgScraperPersonRole::Actor;
        person.name = "Erika Mustermann";
        person.characterName = "Kommissarin Nord";
        person.image = artwork(
            "/var/cache/vdr/plugins/tvscraper/private/person.jpg",
            300,
            450);
        metadata.people.push_back(person);

        EpgScraperImage image;
        image.orientation = EpgScraperImageOrientation::Portrait;
        image.artwork = artwork(
            "/var/cache/vdr/plugins/tvscraper/private/poster.jpg",
            600,
            900);
        metadata.images.push_back(image);

        assert(metadata.valid());
        const std::string json = serializer.serialize(resolution);

        assert(contains(json, "\"available\":true"));
        assert(contains(json, "\"status\":\"ready\""));
        assert(contains(json, "\"mediaType\":\"series\""));
        assert(contains(json, "\"durationDeviationMinutes\":-2"));
        assert(contains(json, "\"providerHints\":{\"hd\":1,\"language\":2}"));
        assert(contains(json, "\"role\":\"actor\""));
        assert(contains(json, "Kommissarin Nord"));
        assert(contains(json, "\"orientation\":\"portrait\""));
        assert(contains(json,
            "/api/epg/cache/metadata/image?backend=parents%20vdr&amp;") == false);
        assert(contains(json,
            "/api/epg/cache/metadata/image?backend=parents%20vdr&channelId="));
        assert(contains(json, "kind=preferred&index=0"));
        assert(contains(json, "kind=person&index=0"));
        assert(contains(json, "kind=gallery&index=0"));

        assert(!contains(json, "/var/cache/"));
        assert(!contains(json, "preferred.jpg"));
        assert(!contains(json, "person.jpg"));
        assert(!contains(json, "poster.jpg"));
        assert(!contains(json, "fallback.jpg"));
        assert(!contains(json, "example-provider"));
        assert(!contains(json, "seriesArtworkFallback"));
        assert(!contains(json, "scraperHd"));
        assert(!contains(json, "scraperLanguage"));
    }

    return 0;
}
