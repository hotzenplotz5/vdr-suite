#include "EpgScraperMetadataPublicJsonSerializer.h"

#include <cctype>
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

void appendPublicArtwork(
    std::ostringstream& json,
    const EpgScraperArtwork& artwork,
    const std::string& url)
{
    json << "{\"available\":" << (artwork.valid() ? "true" : "false");
    if (artwork.valid())
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

    json << ",\"preferredArtwork\":";
    appendPublicArtwork(
        json,
        metadata.preferredArtwork,
        imageUrl(metadata, "preferred", 0));

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
            imageUrl(metadata, "person", static_cast<int>(index)));
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
            imageUrl(metadata, "gallery", static_cast<int>(index)));
        json << '}';
    }
    json << "]}";
    return json.str();
}
