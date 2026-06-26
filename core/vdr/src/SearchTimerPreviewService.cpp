#include "SearchTimerPreviewService.h"

#include "EpgSearchMatch.h"
#include "EpgSearchRequest.h"
#include "EpgSearchRequestMapper.h"
#include "EpgSearchResult.h"
#include "EpgSearchService.h"
#include "SearchTimerPreviewEpgInputContext.h"

#include <algorithm>
#include <vector>

namespace {

bool hasExplicitComparisonOptions(
    const SearchTimer& searchTimer)
{
    return searchTimer.comparisonOptions().compareTitle() ||
           searchTimer.comparisonOptions().compareSubtitle() ||
           searchTimer.comparisonOptions().compareSummary();
}

EpgSearchResult paginateResult(
    const EpgSearchResult& result,
    int limit,
    int offset)
{
    const int safeOffset =
        offset < 0
            ? 0
            : offset;

    const std::vector<EpgSearchMatch>& sourceMatches =
        result.matches();

    std::vector<EpgSearchMatch> pagedMatches;

    if (safeOffset < static_cast<int>(sourceMatches.size()))
    {
        const int sourceSize =
            static_cast<int>(sourceMatches.size());

        const int end =
            limit > 0
                ? std::min(sourceSize, safeOffset + limit)
                : sourceSize;

        for (int index = safeOffset; index < end; ++index)
        {
            pagedMatches.push_back(sourceMatches.at(index));
        }
    }

    return EpgSearchResult(
        pagedMatches,
        result.totalCount(),
        limit,
        safeOffset);
}

} // namespace

SearchTimerPreviewResult SearchTimerPreviewService::preview(
    const SearchTimer& searchTimer,
    const std::vector<VdrEvent>& events,
    int limit,
    int offset) const
{
    const SearchTimerPreviewEpgInputContextState epgInputContext =
        SearchTimerPreviewEpgInputContext::current();

    SearchTimerPreviewEpgInputContext::resetReady();

    EpgSearchRequest request =
        EpgSearchRequest::sorted(
            searchTimer.backendId(),
            searchTimer.query(),
            "",
            -1,
            0,
            0,
            0,
            EpgSearchSortField::StartTime,
            EpgSearchSortOrder::Ascending);

    if (hasExplicitComparisonOptions(searchTimer))
    {
        request.setSearchFields(
            searchTimer.comparisonOptions().compareTitle(),
            searchTimer.comparisonOptions().compareSubtitle(),
            searchTimer.comparisonOptions().compareSummary());
    }

    EpgSearchService service;
    EpgSearchRequestMapper requestMapper;

    const EpgSearchResult unpagedResult =
        service.search(
            events,
            requestMapper.map(request));

    return SearchTimerPreviewResult(
        searchTimer,
        paginateResult(
            unpagedResult,
            limit,
            offset),
        epgInputContext.status,
        epgInputContext.available,
        epgInputContext.warnings);
}
