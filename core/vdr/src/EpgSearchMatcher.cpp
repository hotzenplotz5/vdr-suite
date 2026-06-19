#include "EpgSearchMatcher.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <string>

namespace
{
std::string lowerCopy(
    const std::string& value)
{
    std::string result = value;

    std::transform(
        result.begin(),
        result.end(),
        result.begin(),
        [](unsigned char character)
        {
            return static_cast<char>(
                std::tolower(character));
        });

    return result;
}

bool containsCaseInsensitive(
    const std::string& value,
    const std::string& needle)
{
    return lowerCopy(value).find(
               lowerCopy(needle)) != std::string::npos;
}

bool textMatches(
    const VdrEvent& event,
    const EpgSearchRequest& request)
{
    if (!request.hasQueryText())
    {
        return true;
    }

    if (!request.hasSearchField())
    {
        return false;
    }

    if (request.searchTitle() &&
        containsCaseInsensitive(
            event.title,
            request.queryText()))
    {
        return true;
    }

    if (request.searchSubtitle() &&
        containsCaseInsensitive(
            event.subtitle,
            request.queryText()))
    {
        return true;
    }

    if (request.searchDescription() &&
        containsCaseInsensitive(
            event.description,
            request.queryText()))
    {
        return true;
    }

    return false;
}

int parseInteger(
    const std::string& value,
    int fallback)
{
    if (value.empty())
    {
        return fallback;
    }

    char* end = nullptr;
    const long parsed = std::strtol(
        value.c_str(),
        &end,
        10);

    if (end == value.c_str() || *end != '\0')
    {
        return fallback;
    }

    return static_cast<int>(parsed);
}
}

bool EpgSearchMatcher::matches(
    const VdrEvent& event,
    const EpgSearchRequest& request) const
{
    if (request.hasChannelId() &&
        event.channelId != request.channelId())
    {
        return false;
    }

    if (!textMatches(
            event,
            request))
    {
        return false;
    }

    if (request.hasFrom())
    {
        const int startTime =
            parseInteger(
                event.startTime,
                -1);

        if (startTime >= 0 &&
            startTime < request.from())
        {
            return false;
        }
    }

    if (request.hasTimespan() &&
        request.hasFrom())
    {
        const int startTime =
            parseInteger(
                event.startTime,
                -1);

        if (startTime >= 0 &&
            startTime >= request.from() + request.timespan())
        {
            return false;
        }
    }

    return true;
}
