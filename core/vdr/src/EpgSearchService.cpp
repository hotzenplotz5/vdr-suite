#include "EpgSearchService.h"

#include "EpgSearchMatch.h"

#include <algorithm>
#include <cctype>
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

bool matchesQuery(
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
}

EpgSearchResult EpgSearchService::search(
    const std::vector<VdrEvent>& events,
    const EpgSearchQuery& query) const
{
    std::vector<EpgSearchMatch> matches;

    for (const VdrEvent& event : events)
    {
        if (matchesQuery(event, query))
        {
            matches.push_back(
                EpgSearchMatch::fromEvent(event));
        }
    }

    return EpgSearchResult(
        matches,
        static_cast<int>(matches.size()),
        0,
        0);
}
