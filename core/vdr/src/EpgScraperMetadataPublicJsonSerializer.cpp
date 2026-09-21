#include "EpgScraperMetadataPublicJsonSerializer.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <iomanip>
#include <sstream>

namespace
{

std::string escapeJson(const std::string& value)
{
    std::ostringstream escaped;
    for (unsigned char character : value)
    {
        switch (character)
        {
            case '"': escaped << "\\\""; break;
            case '\\': escaped << "\\\\"; break;
            case '\b': escaped << "\\b"; break;
            case '\f': escaped << "\\f"; break;
            case '\n': escaped << "\\n"; break;
            case '\r': escaped << "\\r"; break;
            case '\t': escaped << "\\t"; break;
            default:
                if (character < 0x20)
                {
                    escaped << "\\u00"
                        << std::hex << std::setw(2) << std::setfill('0')
                        << static_cast<int>(character)
                        << std::dec << std::setfill(' ');
                }
                else
                {
                    escaped << static_cast<char>(character);
                }
                break;
        }
    }
    return escaped.str();
}

std::string urlEncode(const std::string& value)
{
    std::ostringstream encoded;
    encoded << std::uppercase << std::hex;
    for (unsigned char character : value)
    {
        if (std::isalnum(character) || character == '-' || character == '_' ||
            character == '.' || character == '~')
        {
            encoded << static_cast<char>(character);
        }
        else
        {
            encoded << '%' << std::setw(2) << std::setfill('0')
                << static_cast<int>(character);
        }
    }
    return encoded.str();
}

const char* mediaTypeName(EpgScraperMediaType type)
{
    switch (type)
    {
        case EpgScraperMediaType::Series: return "series";
        case EpgScraperMediaType::Movie: return "movie";
        case EpgScraperMediaType::None: return "none";
    }
    return "none";
}

const char* personRoleName(EpgScraperPersonRole role)
{
    switch (role)
    {
        case EpgScraperPersonRole::Actor: return "actor";
        case EpgScraperPersonRole::Director: return "director";
        case EpgScraperPersonRole::Writer: return "writer";
        case EpgScraperPersonRole::Producer: return "producer";
        case EpgScraperPersonRole::Moderator: return "moderator";
        case EpgScraperPersonRole::Guest: return "guest";
        case EpgScraperPersonRole::Composer: return "composer";
        case EpgScraperPersonRole::Other: return "other";
        case EpgScraperPersonRole::Unknown: return "unknown";
    }
    return "unknown";
}

const char* orientationName(EpgScraperImageOrientation orientation)
{
    switch (orientation)
    {
        case EpgScraperImageOrientation::Landscape: return "landscape";
        case EpgScraperImageOrientation::Banner: return "banner";
        case EpgScraperImageOrientation::Portrait: return "portrait";
        case EpgScraperImageOrientation::Unknown: return "unknown";
    }
    return "unknown";
}

void appendStringArray(
    std::ostringstream& json,
    const std::vector<std::string>& values)
{
    json << '[';
    for (std::size_t index = 0; index < values.size(); ++index)
    {
        if (index > 0) json << ',';
        json << '"' << escapeJson(values[index]) << '"';
    }
    json << ']';
}

bool validFallbackProvider(const std::string& provider)
{
    if (provider.empty() || provider.size() > 64U ||
        provider == "none" || provider == "tvscraper")
    {
        return false;
    }

    return std::all_of(
        provider.begin(),
        provider.end(),
        [](unsigned char character)
        {
            return (character >= 'a' && character <= 'z') ||
                (character >= '0' && character <= '9') ||
                character == '-' || character == '_' || character == '.';
        });
}

bool publicFallbackAvailable(const EpgScraperArtwork& artwork)
{
    return artwork.available && artwork.managed &&
        artwork.origin == EpgScraperArtworkOrigin::ExternalFallback &&
        validFallbackProvider(artwork.provider) &&
        std::filesystem::path(artwork.path).is_absolute() &&
        artwork.width > 0 && artwork.height > 0;
}

void appendPublicArtwork(
    std::ostringstream& json,
    const EpgScraperArtwork& artwork,
    const std::string& url,
    bool available)
{
    json << "{\"available\":" << (available ? "true" : "false");
    if (available)
    {
        json << ",\"url\":\"" << escapeJson(url) << "\""
             << ",\"width\":" << artwork.width
             << ",\"height\":" << artwork.height;
    }
    json << '}';
}

}

std::string EpgScraperMetadataPublicJsonSerializer::imageUrl(
    const EpgScraperMetadata& metadata,
    const std::string& kind,
    int index)
{
    std::ostringstream url;
    url << "/api/epg/cache/metadata/image"
        << "?backend=" << urlEncode(metadata.backendId)
        << "&channelId=" << urlEncode(metadata.channelId)
        << "&eventId=" << urlEncode(metadata.eventId)
        << "&kind=" << urlEncode(kind)
        << "&index=" << index;
    return url.str();
}

std::string EpgScraperMetadataPublicJsonSerializer::serialize(
    const EpgScraperMetadataResolution& resolution) const
{
    if (!resolution.attempted)
    {
        return "{\"available\":false,\"status\":\"unavailable\"}";
    }
    if (!resolution.found)
    {
        return "{\"available\":false,\"status\":\"not-found\"}";
    }

    const EpgScraperMetadata& metadata = resolution.metadata;
    std::ostringstream json;
    json << "{\"available\":true"
         << ",\"status\":\"ready\""
         << ",\"provider\":\"tvscraper\""
         << ",\"mediaType\":\"" << mediaTypeName(metadata.mediaType) << "\""
         << ",\"providerId\":" << metadata.providerId
         << ",\"seasonNumber\":" << metadata.seasonNumber
         << ",\"episodeNumber\":" << metadata.episodeNumber
         << ",\"absoluteEpisodeNumber\":" << metadata.absoluteEpisodeNumber
         << ",\"runtimeMinutes\":" << metadata.runtimeMinutes
         << ",\"durationDeviationMinutes\":" << metadata.durationDeviationMinutes
         << ",\"popularity\":" << metadata.popularity
         << ",\"voteAverage\":" << metadata.voteAverage
         << ",\"voteCount\":" << metadata.voteCount
         << ",\"adult\":" << (metadata.adult ? "true" : "false")
         << ",\"collectionId\":" << metadata.collectionId
         << ",\"lastSeason\":" << metadata.lastSeason
         << ",\"title\":\"" << escapeJson(metadata.title) << "\""
         << ",\"originalTitle\":\"" << escapeJson(metadata.originalTitle) << "\""
         << ",\"episodeName\":\"" << escapeJson(metadata.episodeName) << "\""
         << ",\"tagline\":\"" << escapeJson(metadata.tagline) << "\""
         << ",\"overview\":\"" << escapeJson(metadata.overview) << "\""
         << ",\"releaseDate\":\"" << escapeJson(metadata.releaseDate) << "\""
         << ",\"firstAired\":\"" << escapeJson(metadata.firstAired) << "\""
         << ",\"imdbId\":\"" << escapeJson(metadata.imdbId) << "\""
         << ",\"statusText\":\"" << escapeJson(metadata.status) << "\""
         << ",\"collectionName\":\"" << escapeJson(metadata.collectionName) << "\""
         << ",\"genres\":";

    appendStringArray(json, metadata.genres);
    json << ",\"productionCountries\":";
    appendStringArray(json, metadata.productionCountries);
    json << ",\"networks\":";
    appendStringArray(json, metadata.networks);

    json << ",\"providerHints\":{"
         << "\"hd\":" << metadata.scraperHd << ','
         << "\"language\":" << metadata.scraperLanguage
         << "}";

    const bool primaryArtworkAvailable = metadata.preferredArtwork.valid();
    const bool fallbackArtworkAvailable =
        !primaryArtworkAvailable &&
        publicFallbackAvailable(metadata.seriesArtworkFallback);
    const EpgScraperArtwork& selectedPreferredArtwork =
        primaryArtworkAvailable
            ? metadata.preferredArtwork
            : metadata.seriesArtworkFallback;

    json << ",\"preferredArtwork\":";
    appendPublicArtwork(
        json,
        selectedPreferredArtwork,
        imageUrl(metadata, "preferred", 0),
        primaryArtworkAvailable || fallbackArtworkAvailable);

    json << ",\"people\":[";
    for (std::size_t index = 0; index < metadata.people.size(); ++index)
    {
        if (index > 0) json << ',';
        const EpgScraperPerson& person = metadata.people[index];
        json << "{\"role\":\"" << personRoleName(person.role) << "\""
             << ",\"name\":\"" << escapeJson(person.name) << "\""
             << ",\"characterName\":\"" << escapeJson(person.characterName) << "\""
             << ",\"image\":";
        appendPublicArtwork(
            json,
            person.image,
            imageUrl(metadata, "person", static_cast<int>(index)),
            person.image.valid());
        json << '}';
    }
    json << ']';

    json << ",\"images\":[";
    for (std::size_t index = 0; index < metadata.images.size(); ++index)
    {
        if (index > 0) json << ',';
        const EpgScraperImage& image = metadata.images[index];
        json << "{\"orientation\":\""
             << orientationName(image.orientation) << "\""
             << ",\"image\":";
        appendPublicArtwork(
            json,
            image.artwork,
            imageUrl(metadata, "gallery", static_cast<int>(index)),
            image.artwork.valid());
        json << '}';
    }
    json << "]}";
    return json.str();
}
