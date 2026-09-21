#include "VdrRecordingNativeMetadataPublicJsonSerializer.h"

#include <cctype>
#include <iomanip>
#include <sstream>

namespace
{
std::string escapeJson(const std::string& value)
{
    std::ostringstream escaped;

    for (const unsigned char character : value)
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
                    << std::hex
                    << std::setw(2)
                    << std::setfill('0')
                    << static_cast<int>(character)
                    << std::dec
                    << std::setfill(' ');
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

    for (const unsigned char character : value)
    {
        if (std::isalnum(character) || character == '-' || character == '_' ||
            character == '.' || character == '~')
        {
            encoded << static_cast<char>(character);
        }
        else
        {
            encoded << '%'
                << std::setw(2)
                << std::setfill('0')
                << static_cast<int>(character);
        }
    }

    return encoded.str();
}

void appendStringArray(
    std::ostringstream& json,
    const std::vector<std::string>& values)
{
    json << '[';

    for (std::size_t index = 0; index < values.size(); ++index)
    {
        if (index > 0)
        {
            json << ',';
        }

        json << '"' << escapeJson(values[index]) << '"';
    }

    json << ']';
}

void appendArtwork(
    std::ostringstream& json,
    const VdrRecordingNativeArtwork& artwork,
    const std::string& url)
{
    json << "{\"available\":"
         << (artwork.available && !artwork.path.empty() ? "true" : "false");

    if (artwork.available && !artwork.path.empty())
    {
        json << ",\"url\":\"" << escapeJson(url) << "\""
             << ",\"width\":" << artwork.width
             << ",\"height\":" << artwork.height;
    }

    json << '}';
}

std::string unavailableJson(const bool settled)
{
    return std::string("{\"available\":false,\"status\":\"not-found\",\"settled\":") +
        (settled ? "true}" : "false}");
}
}

std::string VdrRecordingNativeMetadataPublicJsonSerializer::imageUrl(
    const VdrRecordingNativeMetadataRecord& record,
    const std::string& kind,
    int index)
{
    std::ostringstream url;
    url << "/api/vdr/recordings/metadata/image"
        << "?backend=" << urlEncode(record.backendId)
        << "&backendNativeId=" << urlEncode(record.backendNativeId)
        << "&kind=" << urlEncode(kind)
        << "&index=" << index;
    return url.str();
}

std::string VdrRecordingNativeMetadataPublicJsonSerializer::serialize(
    const VdrRecordingNativeMetadataRecord& record) const
{
    if (!record.exists())
    {
        return unavailableJson(false);
    }

    if (!record.metadata.found)
    {
        return unavailableJson(
            record.metadata.availability ==
            VdrRecordingNativeMetadataAvailability::NotFound);
    }

    const VdrRecordingNativeMetadata& metadata = record.metadata;
    std::ostringstream json;

    json << "{\"available\":true"
         << ",\"status\":\"ready\""
         << ",\"provider\":\"" << escapeJson(metadata.provider) << "\""
         << ",\"mediaType\":\"" << escapeJson(metadata.mediaType) << "\""
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
         << '}';

    json << ",\"preferredArtwork\":";
    appendArtwork(
        json,
        metadata.preferredArtwork,
        imageUrl(record, "preferred", 0));

    json << ",\"people\":[";
    for (std::size_t index = 0; index < metadata.people.size(); ++index)
    {
        if (index > 0)
        {
            json << ',';
        }

        const VdrRecordingNativePerson& person = metadata.people[index];
        json << "{\"role\":\"" << escapeJson(person.role) << "\""
             << ",\"name\":\"" << escapeJson(person.name) << "\""
             << ",\"characterName\":\"" << escapeJson(person.characterName) << "\""
             << ",\"image\":";
        appendArtwork(
            json,
            person.image,
            imageUrl(record, "person", static_cast<int>(index)));
        json << '}';
    }
    json << ']';

    json << ",\"images\":[";
    for (std::size_t index = 0; index < metadata.images.size(); ++index)
    {
        if (index > 0)
        {
            json << ',';
        }

        const VdrRecordingNativeArtwork& image = metadata.images[index];
        json << "{\"orientation\":\"" << escapeJson(image.orientation) << "\""
             << ",\"image\":";
        appendArtwork(
            json,
            image,
            imageUrl(record, "gallery", static_cast<int>(index)));
        json << '}';
    }
    json << "]}";

    return json.str();
}
