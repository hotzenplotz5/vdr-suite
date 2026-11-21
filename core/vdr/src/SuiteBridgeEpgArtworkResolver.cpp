#include "SuiteBridgeEpgArtworkResolver.h"

#include <cctype>
#include <chrono>
#include <cstdlib>

namespace
{
int hexDigitValue(const char character)
{
    if (character >= '0' && character <= '9')
    {
        return character - '0';
    }
    if (character >= 'a' && character <= 'f')
    {
        return character - 'a' + 10;
    }
    if (character >= 'A' && character <= 'F')
    {
        return character - 'A' + 10;
    }
    return -1;
}

bool appendEscapedCharacter(
    const std::string& json,
    std::size_t& index,
    std::string& result)
{
    const char character = json[index];
    switch (character)
    {
        case 'b': result.push_back('\b'); return true;
        case 'f': result.push_back('\f'); return true;
        case 'n': result.push_back('\n'); return true;
        case 'r': result.push_back('\r'); return true;
        case 't': result.push_back('\t'); return true;
        case '\\': result.push_back('\\'); return true;
        case '"': result.push_back('"'); return true;
        case 'u':
        {
            if (index + 4 >= json.size())
            {
                return false;
            }

            int value = 0;
            for (std::size_t offset = 1; offset <= 4; ++offset)
            {
                const int digit = hexDigitValue(json[index + offset]);
                if (digit < 0)
                {
                    return false;
                }
                value = value * 16 + digit;
            }

            // SuiteBridge currently emits \u00XX only for JSON control bytes.
            // Reject wider code points instead of silently producing invalid UTF-8.
            if (value > 0x7f)
            {
                return false;
            }

            result.push_back(static_cast<char>(value));
            index += 4;
            return true;
        }
        default:
            return false;
    }
}

std::string jsonString(const std::string& json, const std::string& key)
{
    const std::string marker = "\"" + key + "\":\"";
    const std::size_t start = json.find(marker);
    if (start == std::string::npos)
    {
        return {};
    }

    std::string result;
    bool escaped = false;
    for (std::size_t index = start + marker.size(); index < json.size(); ++index)
    {
        const char character = json[index];
        if (escaped)
        {
            if (!appendEscapedCharacter(json, index, result))
            {
                return {};
            }
            escaped = false;
            continue;
        }

        if (character == '\\')
        {
            escaped = true;
            continue;
        }
        if (character == '"')
        {
            return result;
        }
        if (static_cast<unsigned char>(character) < 0x20)
        {
            return {};
        }
        result.push_back(character);
    }

    return {};
}

bool jsonBool(const std::string& json, const std::string& key, bool& value)
{
    const std::string marker = "\"" + key + "\":";
    const std::size_t start = json.find(marker);
    if (start == std::string::npos)
    {
        return false;
    }

    const std::size_t valueStart = start + marker.size();
    if (json.compare(valueStart, 4, "true") == 0)
    {
        value = true;
        return true;
    }
    if (json.compare(valueStart, 5, "false") == 0)
    {
        value = false;
        return true;
    }
    return false;
}

bool jsonInt(const std::string& json, const std::string& key, int& value)
{
    const std::string marker = "\"" + key + "\":";
    const std::size_t start = json.find(marker);
    if (start == std::string::npos)
    {
        return false;
    }

    const char* begin = json.c_str() + start + marker.size();
    char* end = nullptr;
    const long parsed = std::strtol(begin, &end, 10);
    if (end == begin || parsed < 0 || parsed > 2147483647L)
    {
        return false;
    }
    value = static_cast<int>(parsed);
    return true;
}

long long epochSeconds()
{
    return std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}
}

SuiteBridgeEpgArtworkResolver::SuiteBridgeEpgArtworkResolver(
    ISuiteBridgeArtworkTransport& transport)
    : transport_(transport)
{
}

EpgArtworkResolution SuiteBridgeEpgArtworkResolver::resolve(
    const std::string& backendId,
    const VdrEvent& event)
{
    EpgArtworkResolution resolution;
    if (event.channelId.empty() || event.id.empty())
    {
        return resolution;
    }

    const SuiteBridgeArtworkCommandReply reply =
        transport_.requestArtwork(event.channelId, event.id);
    if (!reply.transportSucceeded)
    {
        return resolution;
    }

    resolution.attempted = true;
    if (reply.replyCode != 250)
    {
        return resolution;
    }

    bool found = false;
    if (!jsonBool(reply.payload, "found", found))
    {
        resolution.attempted = false;
        return resolution;
    }

    resolution.found = found;
    if (!found)
    {
        return resolution;
    }

    EpgArtworkReference artwork;
    artwork.backendId = backendId;
    artwork.channelId = event.channelId;
    artwork.eventId = event.id;
    artwork.provider = jsonString(reply.payload, "provider");
    artwork.path = jsonString(reply.payload, "path");
    if (!jsonInt(reply.payload, "width", artwork.width) ||
        !jsonInt(reply.payload, "height", artwork.height))
    {
        resolution.attempted = false;
        resolution.found = false;
        return resolution;
    }
    artwork.resolvedAt = epochSeconds();

    if (!artwork.valid())
    {
        resolution.attempted = false;
        resolution.found = false;
        return resolution;
    }

    resolution.artwork = artwork;
    return resolution;
}
