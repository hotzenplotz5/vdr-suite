#pragma once

#include <string>
#include <vector>

enum class EpgMetadataMediaType
{
    None = 0,
    Series,
    Movie
};

enum class EpgMetadataImageOrientation
{
    None = 0,
    Landscape,
    Banner,
    Portrait
};

struct EpgMetadataImage
{
    EpgMetadataImageOrientation orientation =
        EpgMetadataImageOrientation::None;
    std::string path;
    int width = 0;
    int height = 0;

    bool valid() const noexcept
    {
        return orientation != EpgMetadataImageOrientation::None &&
            !path.empty() &&
            width > 0 &&
            height > 0;
    }
};

struct EpgMetadataPerson
{
    std::string role;
    std::string name;
    std::string characterName;
    EpgMetadataImage image;

    bool valid() const noexcept
    {
        return !role.empty() && !name.empty();
    }
};

struct EpgMetadataRecord
{
    std::string backendId;
    std::string channelId;
    std::string eventId;
    std::string provider;
    EpgMetadataMediaType mediaType = EpgMetadataMediaType::None;
    int providerDatabaseId = 0;

    std::string title;
    std::string originalTitle;
    std::string episodeTitle;
    std::string tagline;
    std::string overview;
    std::string episodeOverview;
    std::string releaseDate;
    std::string firstAired;
    std::string imdbId;
    int collectionId = 0;
    std::string collectionName;
    std::string status;

    int runtimeMinutes = 0;
    int seasonNumber = 0;
    int episodeNumber = 0;
    int absoluteEpisodeNumber = 0;
    int lastSeason = 0;
    bool adult = false;
    double voteAverage = 0.0;
    int voteCount = 0;

    std::vector<std::string> genres;
    std::vector<std::string> productionCountries;
    std::vector<std::string> networks;
    std::vector<EpgMetadataPerson> persons;
    std::vector<EpgMetadataImage> images;

    std::string sourcePayload;
    long long resolvedAt = 0;

    bool valid() const noexcept
    {
        return !backendId.empty() &&
            !channelId.empty() &&
            !eventId.empty() &&
            provider == "tvscraper" &&
            mediaType != EpgMetadataMediaType::None &&
            resolvedAt > 0;
    }
};
