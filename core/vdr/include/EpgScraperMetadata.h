#pragma once

#include <string>
#include <vector>

enum class EpgScraperMediaType
{
    None,
    Series,
    Movie
};

enum class EpgScraperPersonRole
{
    Unknown,
    Actor,
    Director,
    Writer,
    Producer,
    Moderator,
    Guest,
    Composer,
    Other
};

enum class EpgScraperImageOrientation
{
    Unknown,
    Landscape,
    Banner,
    Portrait
};

enum class EpgScraperArtworkOrigin
{
    Unknown,
    PrimaryMetadata,
    ExternalFallback
};

enum class EpgScraperExternalIdProvider
{
    Unknown,
    Imdb,
    Tmdb,
    Tvdb
};

enum class EpgScraperExternalIdScope
{
    Unknown,
    Series,
    Season,
    Episode,
    Movie
};

struct EpgScraperExternalId
{
    EpgScraperExternalIdProvider provider =
        EpgScraperExternalIdProvider::Unknown;
    EpgScraperExternalIdScope scope = EpgScraperExternalIdScope::Unknown;
    std::string value;

    bool valid() const
    {
        return provider != EpgScraperExternalIdProvider::Unknown &&
            scope != EpgScraperExternalIdScope::Unknown && !value.empty();
    }
};

struct EpgScraperArtwork
{
    bool available = false;
    std::string provider;
    EpgScraperArtworkOrigin origin = EpgScraperArtworkOrigin::Unknown;
    // Internal delivery eligibility. External providers and the materializer
    // cannot set this boundary; it is asserted only after managed persistence
    // succeeds or a persisted managed record is rehydrated.
    bool managed = false;
    std::string path;
    int width = 0;
    int height = 0;

    bool valid() const
    {
        return available && provider == "tvscraper" && !path.empty() &&
            width > 0 && height > 0;
    }
};

struct EpgScraperPerson
{
    EpgScraperPersonRole role = EpgScraperPersonRole::Unknown;
    std::string name;
    std::string characterName;
    EpgScraperArtwork image;

    bool valid() const
    {
        return !name.empty();
    }
};

struct EpgScraperImage
{
    EpgScraperImageOrientation orientation =
        EpgScraperImageOrientation::Unknown;
    EpgScraperArtwork artwork;

    bool valid() const
    {
        return orientation != EpgScraperImageOrientation::Unknown &&
            artwork.valid();
    }
};

struct EpgScraperMetadata
{
    std::string backendId;
    std::string channelId;
    std::string eventId;
    std::string provider;
    EpgScraperMediaType mediaType = EpgScraperMediaType::None;

    int providerId = 0;
    int seasonNumber = 0;
    int episodeNumber = 0;
    int absoluteEpisodeNumber = 0;
    int runtimeMinutes = 0;
    int durationDeviationMinutes = 0;
    int scraperHd = 0;
    int scraperLanguage = 0;
    double popularity = 0.0;
    double voteAverage = 0.0;
    int voteCount = 0;
    bool adult = false;
    int collectionId = 0;
    int lastSeason = 0;

    std::string title;
    std::string originalTitle;
    std::string episodeName;
    std::string tagline;
    std::string overview;
    std::string releaseDate;
    std::string firstAired;
    // Transitional compatibility field. Consumers that need stable matching
    // must prefer the provider- and scope-qualified externalIds entries.
    std::string imdbId;
    std::string status;
    std::string collectionName;

    std::vector<std::string> genres;
    std::vector<std::string> productionCountries;
    std::vector<std::string> networks;
    std::vector<EpgScraperExternalId> externalIds;
    EpgScraperArtwork preferredArtwork;
    // A provider-neutral, daemon-side candidate. It remains separate from the
    // primary TVScraper artwork until secure materialization, persistence, and
    // deliberate public selection have all succeeded.
    EpgScraperArtwork seriesArtworkFallback;
    std::vector<EpgScraperPerson> people;
    std::vector<EpgScraperImage> images;

    bool valid() const
    {
        return !backendId.empty() && !channelId.empty() && !eventId.empty() &&
            provider == "tvscraper" && mediaType != EpgScraperMediaType::None;
    }
};

struct EpgScraperMetadataResolution
{
    bool attempted = false;
    bool found = false;
    EpgScraperMetadata metadata;
};
