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

constexpr int previewTypoFallbackTolerance = 2;

bool hasExplicitComparisonOptions(
    const SearchTimer& searchTimer)
{
    return searchTimer.comparisonOptions().compareTitle() ||
           searchTimer.comparisonOptions().compareSubtitle() ||
           searchTimer.comparisonOptions().compareSummary();
}

EpgSearchMode searchModeForSearchTimerMode(
    int mode)
{
    switch (mode)
    {
    case 1:
        return EpgSearchMode::AllWords;
    case 2:
        return EpgSearchMode::AnyWord;
    case 3:
        return EpgSearchMode::Exact;
    case 4:
        return EpgSearchMode::RegularExpression;
    case 5:
        return EpgSearchMode::Fuzzy;
    default:
        return EpgSearchMode::Phrase;
    }
}

bool shouldApplyPreviewTypoFallback(
    const SearchTimer& searchTimer,
    const EpgSearchResult& result)
{
    return result.totalCount() == 0 &&
           !searchTimer.query().empty() &&
           searchTimer.matchOptions().mode() == 0;
}

void applySearchTimerOptions(
    EpgSearchRequest& request,
    const SearchTimer& searchTimer)
{
    if (hasExplicitComparisonOptions(searchTimer))
    {
        request.setSearchFields(
            searchTimer.comparisonOptions().compareTitle(),
            searchTimer.comparisonOptions().compareSubtitle(),
            searchTimer.comparisonOptions().compareSummary());
    }

    const EpgSearchMode searchMode =
        searchModeForSearchTimerMode(searchTimer.matchOptions().mode());

    request.setSearchMode(searchMode);

    if (searchMode == EpgSearchMode::Fuzzy)
    {
        request.setFuzzyTolerance(searchTimer.matchOptions().tolerance());
    }
}

EpgSearchRequest createPreviewRequest(
    const SearchTimer& searchTimer)
{
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

    applySearchTimerOptions(request, searchTimer);

    return request;
}

EpgSearchRequest createPreviewTypoFallbackRequest(
    const SearchTimer& searchTimer)
{
    EpgSearchRequest request = createPreviewRequest(searchTimer);
    request.setSearchMode(EpgSearchMode::Fuzzy);
    request.setFuzzyTolerance(previewTypoFallbackTolerance);
    return request;
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

    EpgSearchService service;
    EpgSearchRequestMapper requestMapper;

    const EpgSearchResult initialResult =
        service.search(
            events,
            requestMapper.map(createPreviewRequest(searchTimer)));

    const EpgSearchResult unpagedResult =
        shouldApplyPreviewTypoFallback(searchTimer, initialResult)
            ? service.search(
                  events,
                  requestMapper.map(
                      createPreviewTypoFallbackRequest(searchTimer)))
            : initialResult;

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
