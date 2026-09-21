#include "EpgSearchRequestMapper.h"

EpgSearchQuery EpgSearchRequestMapper::map(
    const EpgSearchRequest& request) const
{
    EpgSearchQuery query =
        request.hasQueryText()
            ? EpgSearchQuery::byText(request.queryText())
            : EpgSearchQuery::all();

    if (request.hasBackendId())
    {
        query.withBackend(request.backendId());
    }

    if (request.hasChannelId())
    {
        query.withChannelInterval(
            request.channelId(),
            request.channelId());
    }

    if (request.hasSearchField())
    {
        query.searchInTitle(request.searchTitle());
        query.searchInSubtitle(request.searchSubtitle());
        query.searchInDescription(request.searchDescription());
    }

    if (request.hasSearchMode())
    {
        query.withMode(request.searchMode());
    }

    if (request.hasFuzzyTolerance())
    {
        query.withFuzzyTolerance(request.fuzzyTolerance());
    }

    return query;
}
