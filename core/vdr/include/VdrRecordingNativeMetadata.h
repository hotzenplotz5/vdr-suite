#pragma once

#include <cstddef>
#include <string>
#include <vector>

enum class VdrRecordingNativeMetadataAvailability
{
    Found,
    NotFound,
    ProviderUnavailable,
    TransportError,
    InvalidPayload
};

struct VdrRecordingNativeArtwork
{
    bool available = false;
    std::string provider;
    std::string path;
    int width = 0;
    int height = 0;
    std::string orientation;
};

struct VdrRecordingNativePerson
{
    std::string role;
    std::string name;
    std::string characterName;
    VdrRecordingNativeArtwork image;
};

struct VdrRecordingNativeMetadata
{
    static constexpr int SupportedSchema = 1;
    static constexpr int SupportedIdentitySchema = 1;
    static constexpr std::size_t MaximumGenres = 12;
    static constexpr std::size_t MaximumCountries = 8;
    static constexpr std::size_t MaximumNetworks = 8;
    static constexpr std::size_t MaximumPeople = 128;
    static constexpr std::size_t MaximumImages = 8;
    static constexpr std::size_t MaximumPayloadBytes = 65535;

    VdrRecordingNativeMetadataAvailability availability =
        VdrRecordingNativeMetadataAvailability::InvalidPayload;
    int schema = 0;
    bool found = false;
    std::string reason;
    std::string provider;
    int recordingIdentitySchema = 0;
    std::string recordingKey;
    std::string mediaType;
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
    std::string imdbId;
    std::string status;
    std::string collectionName;

    std::vector<std::string> genres;
    std::vector<std::string> productionCountries;
    std::vector<std::string> networks;
    VdrRecordingNativeArtwork preferredArtwork;
    std::vector<VdrRecordingNativePerson> people;
    std::vector<VdrRecordingNativeArtwork> images;

    std::string diagnostic;
};
