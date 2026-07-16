#include "VdrRecordingMetadataCacheCodec.h"

#include <iomanip>
#include <limits>
#include <sstream>
#include <string>

namespace
{

constexpr const char* PayloadPrefix = "VRM1";

void appendToken(
    std::string& payload,
    const std::string& value)
{
    payload += std::to_string(value.size());
    payload.push_back(':');
    payload += value;
}

bool readToken(
    const std::string& payload,
    std::size_t& position,
    std::string& value)
{
    if (position >= payload.size())
    {
        return false;
    }

    std::size_t length = 0;
    bool hasDigit = false;

    while (position < payload.size() &&
           payload[position] >= '0' &&
           payload[position] <= '9')
    {
        hasDigit = true;
        const std::size_t digit =
            static_cast<std::size_t>(payload[position] - '0');

        if (length >
            (std::numeric_limits<std::size_t>::max() - digit) / 10)
        {
            return false;
        }

        length = length * 10 + digit;
        ++position;
    }

    if (!hasDigit ||
        position >= payload.size() ||
        payload[position] != ':')
    {
        return false;
    }

    ++position;

    if (length > payload.size() - position)
    {
        return false;
    }

    value = payload.substr(position, length);
    position += length;
    return true;
}

std::string integerToken(const long long value)
{
    return std::to_string(value);
}

std::string doubleToken(const double value)
{
    std::ostringstream stream;
    stream << std::setprecision(17) << value;
    return stream.str();
}

bool parseLongLong(
    const std::string& value,
    long long& result)
{
    try
    {
        std::size_t parsed = 0;
        result = std::stoll(value, &parsed);
        return parsed == value.size();
    }
    catch (...)
    {
        return false;
    }
}

bool parseInt(
    const std::string& value,
    int& result)
{
    long long parsed = 0;

    if (!parseLongLong(value, parsed) ||
        parsed < std::numeric_limits<int>::min() ||
        parsed > std::numeric_limits<int>::max())
    {
        return false;
    }

    result = static_cast<int>(parsed);
    return true;
}

bool parseDouble(
    const std::string& value,
    double& result)
{
    try
    {
        std::size_t parsed = 0;
        result = std::stod(value, &parsed);
        return parsed == value.size();
    }
    catch (...)
    {
        return false;
    }
}

bool validMetadataSource(const int value)
{
    return value >= static_cast<int>(VdrRecordingMetadataSource::None) &&
           value <= static_cast<int>(
               VdrRecordingMetadataSource::RestfulApiScraperBridge);
}

bool validContentKind(const int value)
{
    return value >= static_cast<int>(VdrRecordingContentKind::Unknown) &&
           value <= static_cast<int>(
               VdrRecordingContentKind::SeriesEpisode);
}

bool validArtworkKind(const int value)
{
    return value >= static_cast<int>(VdrRecordingArtworkKind::Poster) &&
           value <= static_cast<int>(VdrRecordingArtworkKind::Still);
}

bool metadataIsEmpty(const VdrRecordingMetadata& metadata)
{
    return !metadata.native.hasText() &&
           !metadata.provider.hasData() &&
           metadata.artwork.empty();
}

bool readRequiredToken(
    const std::string& payload,
    std::size_t& position,
    std::string& value)
{
    return readToken(payload, position, value);
}

}

std::string VdrRecordingMetadataCacheCodec::encode(
    const VdrRecordingMetadata& metadata)
{
    if (metadataIsEmpty(metadata))
    {
        return {};
    }

    std::string payload = PayloadPrefix;

    appendToken(payload, metadata.native.eventTitle);
    appendToken(payload, metadata.native.shortText);
    appendToken(payload, metadata.native.description);

    appendToken(
        payload,
        integerToken(static_cast<int>(metadata.provider.source)));
    appendToken(
        payload,
        integerToken(static_cast<int>(metadata.provider.contentKind)));
    appendToken(payload, metadata.provider.movieId);
    appendToken(payload, metadata.provider.seriesId);
    appendToken(payload, metadata.provider.episodeId);
    appendToken(payload, metadata.provider.title);
    appendToken(payload, metadata.provider.originalTitle);
    appendToken(payload, metadata.provider.tagline);
    appendToken(payload, metadata.provider.overview);
    appendToken(payload, metadata.provider.genreText);
    appendToken(payload, metadata.provider.releaseDate);
    appendToken(payload, metadata.provider.seriesTitle);
    appendToken(payload, metadata.provider.episodeTitle);
    appendToken(payload, integerToken(metadata.provider.seasonNumber));
    appendToken(payload, integerToken(metadata.provider.episodeNumber));
    appendToken(payload, integerToken(metadata.provider.runtimeMinutes));
    appendToken(payload, doubleToken(metadata.provider.rating));

    appendToken(
        payload,
        integerToken(
            static_cast<long long>(metadata.artwork.size())));

    for (const VdrRecordingArtworkRef& artwork : metadata.artwork)
    {
        appendToken(
            payload,
            integerToken(static_cast<int>(artwork.kind)));
        appendToken(
            payload,
            integerToken(static_cast<int>(artwork.source)));
        appendToken(payload, artwork.reference);
        appendToken(payload, integerToken(artwork.width));
        appendToken(payload, integerToken(artwork.height));
        appendToken(payload, artwork.temporary ? "1" : "0");
    }

    return payload;
}

VdrRecordingMetadata VdrRecordingMetadataCacheCodec::decode(
    const std::string& payload)
{
    VdrRecordingMetadata metadata;

    if (payload.empty())
    {
        return metadata;
    }

    const std::string prefix = PayloadPrefix;

    if (payload.compare(0, prefix.size(), prefix) != 0)
    {
        return metadata;
    }

    std::size_t position = prefix.size();
    std::string token;

    if (!readRequiredToken(
            payload,
            position,
            metadata.native.eventTitle) ||
        !readRequiredToken(
            payload,
            position,
            metadata.native.shortText) ||
        !readRequiredToken(
            payload,
            position,
            metadata.native.description))
    {
        return VdrRecordingMetadata{};
    }

    int source = 0;
    int contentKind = 0;

    if (!readRequiredToken(payload, position, token) ||
        !parseInt(token, source) ||
        !validMetadataSource(source) ||
        !readRequiredToken(payload, position, token) ||
        !parseInt(token, contentKind) ||
        !validContentKind(contentKind))
    {
        return VdrRecordingMetadata{};
    }

    metadata.provider.source =
        static_cast<VdrRecordingMetadataSource>(source);
    metadata.provider.contentKind =
        static_cast<VdrRecordingContentKind>(contentKind);

    if (!readRequiredToken(payload, position, metadata.provider.movieId) ||
        !readRequiredToken(payload, position, metadata.provider.seriesId) ||
        !readRequiredToken(payload, position, metadata.provider.episodeId) ||
        !readRequiredToken(payload, position, metadata.provider.title) ||
        !readRequiredToken(payload, position, metadata.provider.originalTitle) ||
        !readRequiredToken(payload, position, metadata.provider.tagline) ||
        !readRequiredToken(payload, position, metadata.provider.overview) ||
        !readRequiredToken(payload, position, metadata.provider.genreText) ||
        !readRequiredToken(payload, position, metadata.provider.releaseDate) ||
        !readRequiredToken(payload, position, metadata.provider.seriesTitle) ||
        !readRequiredToken(payload, position, metadata.provider.episodeTitle))
    {
        return VdrRecordingMetadata{};
    }

    if (!readRequiredToken(payload, position, token) ||
        !parseInt(token, metadata.provider.seasonNumber) ||
        !readRequiredToken(payload, position, token) ||
        !parseInt(token, metadata.provider.episodeNumber) ||
        !readRequiredToken(payload, position, token) ||
        !parseInt(token, metadata.provider.runtimeMinutes) ||
        !readRequiredToken(payload, position, token) ||
        !parseDouble(token, metadata.provider.rating))
    {
        return VdrRecordingMetadata{};
    }

    long long artworkCount = 0;

    if (!readRequiredToken(payload, position, token) ||
        !parseLongLong(token, artworkCount) ||
        artworkCount < 0 ||
        artworkCount > 256)
    {
        return VdrRecordingMetadata{};
    }

    for (long long index = 0; index < artworkCount; ++index)
    {
        VdrRecordingArtworkRef artwork;
        int artworkKind = 0;
        int artworkSource = 0;
        int temporary = 0;

        if (!readRequiredToken(payload, position, token) ||
            !parseInt(token, artworkKind) ||
            !validArtworkKind(artworkKind) ||
            !readRequiredToken(payload, position, token) ||
            !parseInt(token, artworkSource) ||
            !validMetadataSource(artworkSource) ||
            !readRequiredToken(payload, position, artwork.reference) ||
            !readRequiredToken(payload, position, token) ||
            !parseInt(token, artwork.width) ||
            !readRequiredToken(payload, position, token) ||
            !parseInt(token, artwork.height) ||
            !readRequiredToken(payload, position, token) ||
            !parseInt(token, temporary) ||
            (temporary != 0 && temporary != 1))
        {
            return VdrRecordingMetadata{};
        }

        artwork.kind =
            static_cast<VdrRecordingArtworkKind>(artworkKind);
        artwork.source =
            static_cast<VdrRecordingMetadataSource>(artworkSource);
        artwork.temporary = temporary == 1;

        if (artwork.isValid())
        {
            metadata.artwork.push_back(artwork);
        }
    }

    if (position != payload.size())
    {
        return VdrRecordingMetadata{};
    }

    return metadata;
}
