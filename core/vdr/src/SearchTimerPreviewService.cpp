#include "SearchTimerPreviewService.h"

#include "EpgSearchRequest.h"
#include "EpgSearchService.h"

SearchTimerPreviewResult SearchTimerPreviewService::preview(
    const SearchTimer& searchTimer,
    const std::vector<VdrEvent>& events,
    int limit,
    int offset) const
{
    EpgSearchRequest request =
        EpgSearchRequest::text(
            searchTimer.query(),
            limit,
            offset);

    request = EpgSearchRequest::sorted(
        searchTimer.backendId(),
        searchTimer.query(),
        "",
        -1,
        0,
        limit,
        offset,
        EpgSearchSortField::StartTime,
        EpgSearchSortOrder::Ascending);

    EpgSearchService service;

    return SearchTimerPreviewResult(
        searchTimer,
        service.search(
            events,
            request));
}
