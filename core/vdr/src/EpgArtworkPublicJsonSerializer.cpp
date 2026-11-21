#include "EpgArtworkPublicJsonSerializer.h"

#include <cctype>
#include <iomanip>
#include <sstream>

std::string EpgArtworkPublicJsonSerializer::serialize(
    const EpgArtworkReference& artwork) const
{
    if (!artwork.valid())
    {
        return "{\"available\":false}";
    }

    std::ostringstream json;
    json
        << "{"
        << "\"available\":true,"
        << "\"provider\":\"" << escapeJsonString(artwork.provider) << "\","
        << "\"width\":" << artwork.width << ","
        << "\"height\":" << artwork.height << ","
        << "\"url\":\"" << escapeJsonString(artworkUrl(artwork)) << "\""
        << "}";

    return json.str();
}

std::string EpgArtworkPublicJsonSerializer::artworkUrl(
    const EpgArtworkReference& artwork) const
{
    if (!artwork.valid())
    {
        return "";
    }

    return
        "/api/epg/cache/artwork?backend=" +
        percentEncodeQueryValue(artwork.backendId) +
        "&channelId=" +
        percentEncodeQueryValue(artwork.channelId) +
        "&eventId=" +
        percentEncodeQueryValue(artwork.eventId);
}

std::string EpgArtworkPublicJsonSerializer::escapeJsonString(
    const std::string& value)
{
    std::ostringstream escaped;

    for (const unsigned char character : value)
    {
        switch (character)
        {
        case '"':
            escaped << "\\\"";
            break;
        case '\\':
            escaped << "\\\\";
            break;
        case '\n':
            escaped << "\\n";
            break;
        case '\r':
            escaped << "\\r";
            break;
        case '\t':
            escaped << "\\t";
            break;
        default:
            if (character < 0x20)
            {
                escaped
                    << "\\u"
                    << std::hex
                    << std::setw(4)
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

std::string EpgArtworkPublicJsonSerializer::percentEncodeQueryValue(
    const std::string& value)
{
    std::ostringstream encoded;
    encoded << std::uppercase << std::hex;

    for (const unsigned char character : value)
    {
        if (std::isalnum(character) ||
            character == '-' ||
            character == '_' ||
            character == '.' ||
            character == '~')
        {
            encoded << static_cast<char>(character);
        }
        else
        {
            encoded
                << '%'
                << std::setw(2)
                << std::setfill('0')
                << static_cast<int>(character)
                << std::setfill(' ');
        }
    }

    return encoded.str();
}
