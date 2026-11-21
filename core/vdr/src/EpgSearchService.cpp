#include "EpgSearchService.h"

#include "EpgSearchMatch.h"
#include "EpgSearchMatcher.h"

#include <vector>

EpgSearchResult EpgSearchService::search(
    const std::vector<VdrEvent>& events,
    const EpgSearchQuery& query) const
{
    std::vector<EpgSearchMatch> matches;
    const EpgSearchMatcher matcher;

    for (const VdrEvent& event : events)
    {
        if (matcher.matches(event, query))
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
