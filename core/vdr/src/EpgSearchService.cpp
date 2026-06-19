#include "EpgSearchService.h"

#include "EpgSearchMatcher.h"

#include <algorithm>

namespace
{
std::vector<std::string> determineMatchedFields(
    const VdrEvent& event,
    const EpgSearchRequest& request)
{
    std::vector<std::string> fields;

    if (!request.hasQueryText() ||
        !request.hasSearchField())
    {
        return fields;
    }

    EpgSearchRequest titleOnly =
        EpgSearchRequest::text(
            request.queryText(),
            0,
            0);
    titleOnly.setSearchFields(
        true,
        false,
        false);

    EpgSearchRequest subtitleOnly =
        EpgSearchRequest::text(
            request.queryText(),
            0,
            0);
    subtitleOnly.setSearchFields(
        false,
        true,
        false);

    EpgSearchRequest descriptionOnly =
        EpgSearchRequest::text(
            request.queryText(),
            0,
            0);
    descriptionOnly.setSearchFields(
        false,
        false,
        true);

    EpgSearchMatcher matcher;

    if (request.searchTitle() &&
        matcher.matches(
            event,
            titleOnly))
    {
        fields.push_back("title");
    }

    if (request.searchSubtitle() &&
        matcher.matches(
            event,
            subtitleOnly))
    {
        fields.push_back("subtitle");
    }

    if (request.searchDescription() &&
        matcher.matches(
            event,
            descriptionOnly))
    {
        fields.push_back("description");
    }

    return fields;
}

bool sortAscending(
    const EpgSearchMatch& left,
    const EpgSearchMatch& right,
    EpgSearchSortField sortField)
{
    if (sortField == EpgSearchSortField::Title)
    {
        return left.event().title < right.event().title;
    }

    if (sortField == EpgSearchSortField::StartTime)
    {
        return left.event().startTime < right.event().startTime;
    }

    if (sortField == EpgSearchSortField::Duration)
    {
        return left.event().durationSeconds < right.event().durationSeconds;
    }

    return false;
}
}

EpgSearchResult EpgSearchService::search(
    const std::vector<VdrEvent>& events,
    const EpgSearchRequest& request) const
{
    std::vector<EpgSearchMatch> filteredMatches;
    EpgSearchMatcher matcher;

    for (const auto& event : events)
    {
        if (matcher.matches(
                event,
                request))
        {
            filteredMatches.emplace_back(
                event,
                request.backendId(),
                determineMatchedFields(
                    event,
                    request));
        }
    }

    if (request.hasSort())
    {
        std::sort(
            filteredMatches.begin(),
            filteredMatches.end(),
            [&request](
                const EpgSearchMatch& left,
                const EpgSearchMatch& right)
            {
                const bool ascendingResult =
                    sortAscending(
                        left,
                        right,
                        request.sortField());

                if (request.sortDescending())
                {
                    return !ascendingResult;
                }

                return ascendingResult;
            });
    }

    const int totalCount =
        static_cast<int>(filteredMatches.size());

    std::vector<EpgSearchMatch> page;

    const int offset =
        std::max(
            0,
            request.offset());

    const int limit =
        request.limit();

    if (offset < totalCount)
    {
        const int end =
            request.hasLimit()
                ? std::min(
                      totalCount,
                      offset + limit)
                : totalCount;

        for (int index = offset;
             index < end;
             ++index)
        {
            page.push_back(
                filteredMatches.at(
                    static_cast<std::size_t>(index)));
        }
    }

    return EpgSearchResult(
        page,
        totalCount,
        limit,
        offset);
}
