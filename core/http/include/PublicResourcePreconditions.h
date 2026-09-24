#pragma once

#include <cctype>
#include <cstddef>
#include <string>
#include <utility>
#include <vector>

namespace vdrsuite::http
{

enum class PublicEntityTagConditionResult
{
    missing,
    matched,
    notMatched,
    malformed
};

struct PublicEntityTag
{
    bool weak = false;
    std::string opaque;
};

inline std::string publicStrongEntityTag(const std::string& resourceRevision)
{
    if (resourceRevision.empty())
    {
        return "";
    }

    static constexpr char Hex[] = "0123456789abcdef";

    std::string value;
    value.reserve(7U + (resourceRevision.size() * 2U));
    value += "\"vsr-";

    for (const unsigned char byte : resourceRevision)
    {
        value.push_back(Hex[(byte >> 4U) & 0x0fU]);
        value.push_back(Hex[byte & 0x0fU]);
    }

    value += "\"";
    return value;
}

inline bool publicEntityTagParseList(
    const std::string& headerValue,
    bool& wildcard,
    std::vector<PublicEntityTag>& tags)
{
    wildcard = false;
    tags.clear();

    std::size_t offset = 0;

    const auto skipOws = [&headerValue](std::size_t& position)
    {
        while (position < headerValue.size() &&
               (headerValue[position] == ' ' ||
                headerValue[position] == '\t'))
        {
            ++position;
        }
    };

    skipOws(offset);
    if (offset >= headerValue.size())
    {
        return false;
    }

    if (headerValue[offset] == '*')
    {
        ++offset;
        skipOws(offset);
        if (offset != headerValue.size())
        {
            return false;
        }

        wildcard = true;
        return true;
    }

    while (offset < headerValue.size())
    {
        skipOws(offset);

        PublicEntityTag tag;
        if (headerValue.compare(offset, 2U, "W/") == 0)
        {
            tag.weak = true;
            offset += 2U;
        }

        if (offset >= headerValue.size() ||
            headerValue[offset] != '"')
        {
            return false;
        }

        ++offset;
        const std::size_t opaqueStart = offset;

        while (offset < headerValue.size() &&
               headerValue[offset] != '"')
        {
            const unsigned char character =
                static_cast<unsigned char>(headerValue[offset]);

            if (character == 0x7fU || character < 0x21U)
            {
                return false;
            }

            ++offset;
        }

        if (offset >= headerValue.size())
        {
            return false;
        }

        tag.opaque =
            headerValue.substr(opaqueStart, offset - opaqueStart);
        ++offset;
        tags.push_back(std::move(tag));

        skipOws(offset);
        if (offset == headerValue.size())
        {
            break;
        }

        if (headerValue[offset] != ',')
        {
            return false;
        }

        ++offset;
        skipOws(offset);
        if (offset == headerValue.size())
        {
            return false;
        }
    }

    return !tags.empty();
}

inline bool publicEntityTagParseSingle(
    const std::string& value,
    PublicEntityTag& tag)
{
    bool wildcard = false;
    std::vector<PublicEntityTag> tags;
    if (!publicEntityTagParseList(value, wildcard, tags) ||
        wildcard ||
        tags.size() != 1U)
    {
        return false;
    }

    tag = tags.front();
    return true;
}

inline bool publicStrongEntityTagResourceRevision(
    const std::string& entityTag,
    std::string& resourceRevision)
{
    resourceRevision.clear();

    PublicEntityTag parsed;
    if (!publicEntityTagParseSingle(entityTag, parsed) || parsed.weak)
    {
        return false;
    }

    static constexpr char Prefix[] = "vsr-";
    const std::string opaque = parsed.opaque;
    const std::size_t prefixLength = sizeof(Prefix) - 1U;

    if (opaque.size() <= prefixLength ||
        opaque.compare(0, prefixLength, Prefix) != 0)
    {
        return false;
    }

    const std::string encoded = opaque.substr(prefixLength);
    if ((encoded.size() % 2U) != 0U)
    {
        return false;
    }

    auto hexValue = [](char character) -> int
    {
        if (character >= '0' && character <= '9')
            return character - '0';
        if (character >= 'a' && character <= 'f')
            return character - 'a' + 10;
        return -1;
    };

    std::string decoded;
    decoded.reserve(encoded.size() / 2U);

    for (std::size_t index = 0; index < encoded.size(); index += 2U)
    {
        const int high = hexValue(encoded[index]);
        const int low = hexValue(encoded[index + 1U]);
        if (high < 0 || low < 0)
        {
            return false;
        }

        decoded.push_back(
            static_cast<char>((high << 4) | low));
    }

    if (decoded.empty() ||
        publicStrongEntityTag(decoded) != entityTag)
    {
        return false;
    }

    resourceRevision = std::move(decoded);
    return true;
}

inline PublicEntityTagConditionResult publicEvaluateIfMatch(
    const std::string& headerValue,
    const std::string& currentEntityTag)
{
    if (headerValue.empty())
    {
        return PublicEntityTagConditionResult::missing;
    }

    PublicEntityTag current;
    if (!publicEntityTagParseSingle(currentEntityTag, current) ||
        current.weak)
    {
        return PublicEntityTagConditionResult::malformed;
    }

    bool wildcard = false;
    std::vector<PublicEntityTag> candidates;
    if (!publicEntityTagParseList(
            headerValue,
            wildcard,
            candidates))
    {
        return PublicEntityTagConditionResult::malformed;
    }

    if (wildcard)
    {
        return PublicEntityTagConditionResult::matched;
    }

    for (const PublicEntityTag& candidate : candidates)
    {
        if (!candidate.weak &&
            candidate.opaque == current.opaque)
        {
            return PublicEntityTagConditionResult::matched;
        }
    }

    return PublicEntityTagConditionResult::notMatched;
}

inline PublicEntityTagConditionResult publicEvaluateIfNoneMatch(
    const std::string& headerValue,
    const std::string& currentEntityTag)
{
    if (headerValue.empty())
    {
        return PublicEntityTagConditionResult::missing;
    }

    PublicEntityTag current;
    if (!publicEntityTagParseSingle(currentEntityTag, current))
    {
        return PublicEntityTagConditionResult::malformed;
    }

    bool wildcard = false;
    std::vector<PublicEntityTag> candidates;
    if (!publicEntityTagParseList(
            headerValue,
            wildcard,
            candidates))
    {
        return PublicEntityTagConditionResult::malformed;
    }

    if (wildcard)
    {
        return PublicEntityTagConditionResult::matched;
    }

    for (const PublicEntityTag& candidate : candidates)
    {
        if (candidate.opaque == current.opaque)
        {
            return PublicEntityTagConditionResult::matched;
        }
    }

    return PublicEntityTagConditionResult::notMatched;
}

}
