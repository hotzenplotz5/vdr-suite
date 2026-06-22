#include "EpgSearchMatcher.h"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <string>
#include <vector>

namespace {
std::string lowerCopy(
    const std::string& value)
{
    std::string lowered = value;

    std::transform(
        lowered.begin(),
        lowered.end(),
        lowered.begin(),
        [](unsigned char character) {
            return static_cast<char>(
                std::tolower(character));
        });

    return lowered;
}

bool containsText(
    const std::string& haystack,
    const std::string& needle,
    bool matchCase)
{
    if (needle.empty())
    {
        return true;
    }

    if (matchCase)
    {
        return haystack.find(needle) != std::string::npos;
    }

    return lowerCopy(haystack).find(lowerCopy(needle)) != std::string::npos;
}

bool matchesText(
    const VdrEvent& event,
    const EpgSearchQuery& query)
{
    if (!query.hasText())
    {
        return true;
    }

    const bool matchCase =
        query.hasMatchCase() && query.matchCase();

    if (!query.hasFieldSelection())
    {
        return containsText(event.title, query.text(), matchCase)
            || containsText(event.subtitle, query.text(), matchCase)
            || containsText(event.description, query.text(), matchCase);
    }

    bool matched = false;

    if (query.useTitle())
    {
        matched = matched
            || containsText(event.title, query.text(), matchCase);
    }

    if (query.useSubtitle())
    {
        matched = matched
            || containsText(event.subtitle, query.text(), matchCase);
    }

    if (query.useDescription())
    {
        matched = matched
            || containsText(event.description, query.text(), matchCase);
    }

    return matched;
}

bool matchesChannel(
    const VdrEvent& event,
    const EpgSearchQuery& query)
{
    if (!query.hasChannelScope())
    {
        return true;
    }

    if (query.channelScope() == EpgSearchChannelScope::Interval)
    {
        return event.channelId >= query.channelMin()
            && event.channelId <= query.channelMax();
    }

    return true;
}

bool matchesDuration(
    const VdrEvent& event,
    const EpgSearchQuery& query)
{
    if (!query.hasDurationWindow())
    {
        return true;
    }

    const int durationMinutes =
        event.durationSeconds / 60;

    return durationMinutes >= query.durationMinMinutes()
        && durationMinutes <= query.durationMaxMinutes();
}

std::vector<std::string> splitCommaSeparated(
    const std::string& value)
{
    std::vector<std::string> result;
    std::stringstream stream(value);
    std::string item;

    while (std::getline(stream, item, ','))
    {
        if (!item.empty())
        {
            result.push_back(item);
        }
    }

    return result;
}

bool matchesContentDescriptors(
    const VdrEvent& event,
    const EpgSearchQuery& query)
{
    if (!query.hasContentDescriptors())
    {
        return true;
    }

    const std::vector<std::string> expected =
        splitCommaSeparated(query.contentDescriptors());

    for (const std::string& descriptor : expected)
    {
        const bool found =
            std::find(
                event.contentDescriptors.begin(),
                event.contentDescriptors.end(),
                descriptor)
            != event.contentDescriptors.end();

        if (!found)
        {
            return false;
        }
    }

    return true;
}
}

bool EpgSearchMatcher::matches(
    const VdrEvent& event,
    const EpgSearchQuery& query) const
{
    return matchesText(event, query)
        && matchesChannel(event, query)
        && matchesDuration(event, query)
        && matchesContentDescriptors(event, query);
}
