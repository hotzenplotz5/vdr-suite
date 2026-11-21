#pragma once

#include "ManualRecordingMetadataAssignmentRepository.h"

#include <filesystem>
#include <sstream>
#include <string>

namespace vdrsuite
{
namespace rest
{
namespace manual_recording_metadata_json
{

inline void appendJsonString(
    std::ostringstream& json,
    const std::string& value)
{
    json << '"';

    for (const unsigned char character : value)
    {
        switch (character)
        {
        case '"': json << "\\\""; break;
        case '\\': json << "\\\\"; break;
        case '\b': json << "\\b"; break;
        case '\f': json << "\\f"; break;
        case '\n': json << "\\n"; break;
        case '\r': json << "\\r"; break;
        case '\t': json << "\\t"; break;
        default:
            if (character < 0x20)
            {
                const char* digits = "0123456789abcdef";
                json << "\\u00"
                     << digits[(character >> 4) & 0x0f]
                     << digits[character & 0x0f];
            }
            else
            {
                json << static_cast<char>(character);
            }
            break;
        }
    }

    json << '"';
}

inline bool localManualImage(
    const std::string& path)
{
    if (path.empty())
    {
        return false;
    }

    const std::filesystem::path normalized =
        std::filesystem::path(path).lexically_normal();

    static const std::string root =
        "/var/cache/vdr-suite/recording-metadata/posters/";

    return normalized.is_absolute() &&
           normalized != normalized.root_path() &&
           normalized.string().compare(
               0,
               root.size(),
               root) == 0;
}

inline std::string percentEncode(
    const std::string& value)
{
    static const char hex[] =
        "0123456789ABCDEF";

    std::string output;

    for (const unsigned char character : value)
    {
        if ((character >= 'A' && character <= 'Z') ||
            (character >= 'a' && character <= 'z') ||
            (character >= '0' && character <= '9') ||
            character == '-' ||
            character == '_' ||
            character == '.' ||
            character == '~')
        {
            output.push_back(
                static_cast<char>(character));
        }
        else
        {
            output.push_back('%');
            output.push_back(hex[character >> 4U]);
            output.push_back(hex[character & 0x0fU]);
        }
    }

    return output;
}

inline std::string manualImageUrl(
    const ManualRecordingMetadataAssignment& assignment,
    const std::string& backendNativeId)
{
    return
        "/api/vdr/recordings/metadata/image?backend=" +
        percentEncode(assignment.backendId) +
        "&backendNativeId=" +
        percentEncode(backendNativeId) +
        "&kind=preferred&index=0&assignmentRevision=" +
        std::to_string(assignment.revision);
}

inline std::string contentKind(
    const ManualRecordingMetadataAssignment& assignment)
{
    if (assignment.mediaType == "movie")
    {
        return "movie";
    }

    if (assignment.mediaType == "series")
    {
        return "series";
    }

    if (assignment.mediaType == "episode")
    {
        return "series-episode";
    }

    return "unknown";
}

inline std::string seasonEpisodeLabel(
    const ManualRecordingMetadataAssignment& assignment)
{
    std::ostringstream label;

    if (assignment.seasonNumber > 0)
    {
        label << 'S';

        if (assignment.seasonNumber < 10)
        {
            label << '0';
        }

        label << assignment.seasonNumber;
    }

    if (assignment.episodeNumber > 0)
    {
        label << 'E';

        if (assignment.episodeNumber < 10)
        {
            label << '0';
        }

        label << assignment.episodeNumber;
    }

    return label.str();
}

inline unsigned int placeholderVariant(
    const std::string& value)
{
    unsigned int hash = 2166136261u;

    for (const unsigned char character : value)
    {
        hash ^= character;
        hash *= 16777619u;
    }

    return hash % 6u;
}

inline std::string serialize(
    const ManualRecordingMetadataAssignment& assignment,
    const std::string& backendNativeId)
{
    const bool posterAvailable =
        localManualImage(assignment.posterReference);

    const std::string posterUrl =
        posterAvailable
            ? manualImageUrl(
                  assignment,
                  backendNativeId)
            : std::string{};

    const std::string kind =
        contentKind(assignment);

    const std::string seasonEpisode =
        seasonEpisodeLabel(assignment);

    std::ostringstream json;

    json
        << "{\"native\":{"
        << "\"eventTitle\":\"\","
        << "\"shortText\":\"\","
        << "\"description\":\"\"},"
        << "\"provider\":{"
        << "\"available\":true,"
        << "\"source\":\"manual\","
        << "\"contentKind\":";

    appendJsonString(json, kind);

    json << ",\"movieId\":";
    appendJsonString(
        json,
        assignment.mediaType == "movie"
            ? assignment.externalId
            : std::string{});

    json << ",\"seriesId\":";
    appendJsonString(
        json,
        assignment.mediaType == "series"
            ? assignment.externalId
            : std::string{});

    json << ",\"episodeId\":";
    appendJsonString(
        json,
        assignment.mediaType == "episode"
            ? assignment.externalId
            : std::string{});

    json << ",\"title\":";
    appendJsonString(json, assignment.title);

    json << ",\"originalTitle\":";
    appendJsonString(json, assignment.originalTitle);

    json << ",\"tagline\":\"\""
         << ",\"overview\":";
    appendJsonString(json, assignment.overview);

    json << ",\"genreText\":\"\""
         << ",\"releaseDate\":";
    appendJsonString(json, assignment.releaseDate);

    json << ",\"seriesTitle\":";
    appendJsonString(
        json,
        assignment.mediaType == "series"
            ? assignment.title
            : std::string{});

    json << ",\"episodeTitle\":";
    appendJsonString(
        json,
        assignment.mediaType == "episode"
            ? assignment.title
            : std::string{});

    json
        << ",\"seasonNumber\":"
        << assignment.seasonNumber
        << ",\"episodeNumber\":"
        << assignment.episodeNumber
        << ",\"runtimeMinutes\":0"
        << ",\"rating\":0}";

    json
        << ",\"artwork\":{"
        << "\"available\":"
        << (posterAvailable ? "true" : "false")
        << ",\"count\":"
        << (posterAvailable ? 1 : 0)
        << ",\"posterAvailable\":"
        << (posterAvailable ? "true" : "false")
        << ",\"fanartAvailable\":false"
        << ",\"bannerAvailable\":false"
        << ",\"stillAvailable\":false"
        << ",\"preferredAssetId\":\"\""
        << ",\"preferredUrl\":";

    appendJsonString(json, posterUrl);

    json
        << "}"
        << ",\"presentation\":{"
        << "\"title\":";

    appendJsonString(json, assignment.title);

    json << ",\"subtitle\":";
    appendJsonString(json, seasonEpisode);

    json << ",\"summary\":";
    appendJsonString(json, assignment.overview);

    json << ",\"contentKind\":";
    appendJsonString(json, kind);

    json << ",\"seasonEpisode\":";
    appendJsonString(json, seasonEpisode);

    json
        << ",\"posterAssetId\":\"\""
        << ",\"posterUrl\":";

    appendJsonString(json, posterUrl);

    json
        << ",\"providerAvailable\":true"
        << ",\"artworkPrepared\":"
        << (posterAvailable ? "true" : "false")
        << ",\"placeholderVariant\":"
        << placeholderVariant(assignment.title)
        << "}"
        << ",\"manualAssignment\":{"
        << "\"active\":true"
        << ",\"revision\":"
        << assignment.revision
        << ",\"relationshipLocked\":"
        << (assignment.relationshipLocked
                ? "true"
                : "false")
        << ",\"providerId\":";

    appendJsonString(json, assignment.providerId);

    json << ",\"externalNamespace\":";
    appendJsonString(
        json,
        assignment.externalNamespace);

    json << ",\"externalId\":";
    appendJsonString(json, assignment.externalId);

    json << "}}";

    return json.str();
}

}
}
}
