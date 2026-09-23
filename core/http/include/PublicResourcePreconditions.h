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
