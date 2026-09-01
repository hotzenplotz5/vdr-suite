#pragma once

#include "VdrRecordingArtworkIdentity.h"

#include <cstdint>
#include <iomanip>
#include <initializer_list>
#include <sstream>
#include <string>

namespace vdr_recording_metadata_json_detail
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
        case '"':
            json << "\\\"";
            break;
        case '\\':
            json << "\\\\";
            break;
        case '\b':
            json << "\\b";
            break;
        case '\f':
            json << "\\f";
            break;
        case '\n':
            json << "\\n";
            break;
        case '\r':
            json << "\\r";
            break;
        case '\t':
            json << "\\t";
            break;
        default:
            if (character < 0x20)
            {
                const char* digits = "0123456789abcdef";
                json << "\\u00";
                json << digits[(character >> 4) & 0x0f];
                json << digits[character & 0x0f];
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

inline std::string firstNonEmpty(
    const std::initializer_list<std::string>& values)
{
    for (const std::string& value : values)
    {
        if (!value.empty())
        {
            return value;
        }
    }

    return {};
}

inline std::string percentEncode(
    const std::string& value)
{
    static const char hex[] = "0123456789ABCDEF";
    std::string output;
    output.reserve(value.size());

    for (const unsigned char character : value)
    {
        if ((character >= 'A' && character <= 'Z') ||
            (character >= 'a' && character <= 'z') ||
            (character >= '0' && character <= '9') ||
            character == '-' || character == '_' ||
            character == '.' || character == '~')
        {
            output.push_back(static_cast<char>(character));
            continue;
        }

        output.push_back('%');
        output.push_back(hex[character >> 4U]);
        output.push_back(hex[character & 0x0fU]);
    }

    return output;
}

inline std::string nativeMetadataPreferredArtworkUrl(
    const VdrRecording& recording)
{
    if (recording.backendNativeId.empty())
    {
        return {};
    }

    const std::string backendId =
        recording.backendId.empty() ? "default" : recording.backendId;

    return "/api/vdr/recordings/metadata/image?backend=" +
        percentEncode(backendId) +
        "&backendNativeId=" + percentEncode(recording.backendNativeId) +
        "&kind=preferred&index=0";
}

inline std::string presentationTitle(
    const VdrRecording& recording)
{
    const VdrRecordingProviderMetadata& provider =
        recording.metadata.provider;

    if (provider.contentKind ==
            VdrRecordingContentKind::SeriesEpisode &&
        !provider.seriesTitle.empty())
    {
        return provider.seriesTitle;
    }

    return firstNonEmpty({
        provider.title,
        recording.metadata.native.eventTitle,
        recording.title,
        "Aufnahme"
    });
}

inline std::string seasonEpisodeLabel(
    const VdrRecordingProviderMetadata& provider)
{
    if (provider.seasonNumber <= 0 &&
        provider.episodeNumber <= 0)
    {
        return {};
    }

    std::ostringstream label;

    if (provider.seasonNumber > 0)
    {
        label << "S" << std::setw(2) << std::setfill('0')
              << provider.seasonNumber;
    }

    if (provider.episodeNumber > 0)
    {
        label << "E" << std::setw(2) << std::setfill('0')
              << provider.episodeNumber;
    }

    return label.str();
}

inline std::string presentationSubtitle(
    const VdrRecording& recording)
{
    const VdrRecordingProviderMetadata& provider =
        recording.metadata.provider;

    if (provider.contentKind ==
        VdrRecordingContentKind::SeriesEpisode)
    {
        const std::string number = seasonEpisodeLabel(provider);

        if (!number.empty() && !provider.episodeTitle.empty())
        {
            return number + " · " + provider.episodeTitle;
        }

        return firstNonEmpty({
            provider.episodeTitle,
            number,
            recording.metadata.native.shortText
        });
    }

    return firstNonEmpty({
        provider.tagline,
        recording.metadata.native.shortText,
        provider.genreText
    });
}

inline std::string presentationSummary(
    const VdrRecording& recording)
{
    return firstNonEmpty({
        recording.metadata.provider.overview,
        recording.metadata.native.description,
        recording.metadata.native.shortText
    });
}

inline unsigned int placeholderVariant(
    const std::string& value)
{
    std::uint32_t hash = 2166136261u;

    for (const unsigned char character : value)
    {
        hash ^= character;
        hash *= 16777619u;
    }

    return hash % 6u;
}

struct ArtworkSummary
{
    int count = 0;
    bool poster = false;
    bool fanart = false;
    bool banner = false;
    bool still = false;
};

inline ArtworkSummary summarizeArtwork(
    const VdrRecordingMetadata& metadata)
{
    ArtworkSummary summary;

    for (const VdrRecordingArtworkRef& artwork : metadata.artwork)
    {
        if (!artwork.isValid())
        {
            continue;
        }

        ++summary.count;

        switch (artwork.kind)
        {
        case VdrRecordingArtworkKind::Poster:
            summary.poster = true;
            break;
        case VdrRecordingArtworkKind::Fanart:
            summary.fanart = true;
            break;
        case VdrRecordingArtworkKind::Banner:
            summary.banner = true;
            break;
        case VdrRecordingArtworkKind::Still:
            summary.still = true;
            break;
        }
    }

    return summary;
}

inline void appendStringProperty(
    std::ostringstream& json,
    const char* name,
    const std::string& value,
    const bool first = false)
{
    if (!first)
    {
        json << ',';
    }

    appendJsonString(json, name);
    json << ':';
    appendJsonString(json, value);
}

}

inline std::string VdrRecordingMetadataJsonSerializer::serialize(
    const VdrRecording& recording)
{
    using namespace vdr_recording_metadata_json_detail;

    const VdrRecordingMetadata& metadata = recording.metadata;
    const VdrRecordingProviderMetadata& provider = metadata.provider;
    const ArtworkSummary artwork = summarizeArtwork(metadata);
    const VdrRecordingArtworkRef* preferredArtwork =
        VdrRecordingArtworkIdentity::preferredArtwork(recording);
    const std::string preferredAssetId =
        preferredArtwork == nullptr
            ? std::string()
            : VdrRecordingArtworkIdentity::assetId(
                recording,
                *preferredArtwork);
    const std::string preferredArtworkUrl =
        preferredArtwork == nullptr
            ? nativeMetadataPreferredArtworkUrl(recording)
            : VdrRecordingArtworkIdentity::publicUrl(
                recording,
                *preferredArtwork);
    const std::string title = presentationTitle(recording);
    const std::string subtitle = presentationSubtitle(recording);
    const std::string summary = presentationSummary(recording);

    std::ostringstream json;
    json << '{';

    json << "\"native\":{";
    appendStringProperty(
        json,
        "eventTitle",
        metadata.native.eventTitle,
        true);
    appendStringProperty(json, "shortText", metadata.native.shortText);
    appendStringProperty(json, "description", metadata.native.description);
    json << '}';

    json << ",\"provider\":{";
    json << "\"available\":"
         << (provider.hasData() ? "true" : "false");
    appendStringProperty(
        json,
        "source",
        vdrRecordingMetadataSourceName(provider.source));
    appendStringProperty(
        json,
        "contentKind",
        vdrRecordingContentKindName(provider.contentKind));
    appendStringProperty(json, "movieId", provider.movieId);
    appendStringProperty(json, "seriesId", provider.seriesId);
    appendStringProperty(json, "episodeId", provider.episodeId);
    appendStringProperty(json, "title", provider.title);
    appendStringProperty(json, "originalTitle", provider.originalTitle);
    appendStringProperty(json, "tagline", provider.tagline);
    appendStringProperty(json, "overview", provider.overview);
    appendStringProperty(json, "genreText", provider.genreText);
    appendStringProperty(json, "releaseDate", provider.releaseDate);
    appendStringProperty(json, "seriesTitle", provider.seriesTitle);
    appendStringProperty(json, "episodeTitle", provider.episodeTitle);
    json << ",\"seasonNumber\":" << provider.seasonNumber;
    json << ",\"episodeNumber\":" << provider.episodeNumber;
    json << ",\"runtimeMinutes\":" << provider.runtimeMinutes;
    json << ",\"rating\":" << std::setprecision(17)
         << provider.rating;
    json << '}';

    json << ",\"artwork\":{";
    json << "\"available\":"
         << (artwork.count > 0 ? "true" : "false");
    json << ",\"count\":" << artwork.count;
    json << ",\"posterAvailable\":"
         << (artwork.poster ? "true" : "false");
    json << ",\"fanartAvailable\":"
         << (artwork.fanart ? "true" : "false");
    json << ",\"bannerAvailable\":"
         << (artwork.banner ? "true" : "false");
    json << ",\"stillAvailable\":"
         << (artwork.still ? "true" : "false");
    appendStringProperty(json, "preferredAssetId", preferredAssetId);
    appendStringProperty(json, "preferredUrl", preferredArtworkUrl);
    json << '}';

    json << ",\"presentation\":{";
    appendStringProperty(json, "title", title, true);
    appendStringProperty(json, "subtitle", subtitle);
    appendStringProperty(json, "summary", summary);
    appendStringProperty(
        json,
        "contentKind",
        vdrRecordingContentKindName(provider.contentKind));
    appendStringProperty(
        json,
        "seasonEpisode",
        seasonEpisodeLabel(provider));
    appendStringProperty(json, "posterAssetId", preferredAssetId);
    appendStringProperty(json, "posterUrl", preferredArtworkUrl);
    json << ",\"providerAvailable\":"
         << (provider.hasData() ? "true" : "false");
    json << ",\"artworkPrepared\":"
         << (artwork.count > 0 ? "true" : "false");
    json << ",\"placeholderVariant\":"
         << placeholderVariant(title);
    json << '}';

    json << '}';
    return json.str();
}
