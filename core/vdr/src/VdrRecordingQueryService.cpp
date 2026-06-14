#include "VdrRecordingQueryService.h"

#include "VdrService.h"
#include "VdrRecordingQueryMatcher.h"

#include <algorithm>

VdrRecordingQueryService::VdrRecordingQueryService(
    VdrService& vdrService)
    : vdrService_(vdrService)
{
}

VdrRecordingQueryResult VdrRecordingQueryService::queryRecordings(
    const VdrRecordingQuery& query) const
{
    const auto allRecordings =
        vdrService_.getRecordings();

    std::vector<VdrRecording> filteredRecordings;

    for (const auto& recording : allRecordings)
    {
        VdrRecordingQueryMatcher matcher;

        if (matcher.matches(recording, query))
        {
            filteredRecordings.push_back(recording);
        }
    }

    if (query.hasSort())
    {
        std::sort(
            filteredRecordings.begin(),
            filteredRecordings.end(),
            [&query](
                const VdrRecording& left,
                const VdrRecording& right)
            {
                bool ascendingResult = false;

                if (query.sortField() == VdrRecordingSortField::Title)
                {
                    ascendingResult = left.title < right.title;
                }
                else if (query.sortField() == VdrRecordingSortField::StartTime)
                {
                    ascendingResult = left.startTime < right.startTime;
                }
                else if (query.sortField() == VdrRecordingSortField::Duration)
                {
                    ascendingResult = left.durationSeconds < right.durationSeconds;
                }
                else if (query.sortField() == VdrRecordingSortField::Size)
                {
                    ascendingResult = left.sizeMb < right.sizeMb;
                }

                if (query.sortDescending())
                {
                    return !ascendingResult;
                }

                return ascendingResult;
            });
    }

    const int totalCount =
        static_cast<int>(filteredRecordings.size());

    std::vector<VdrRecording> page;

    const int offset =
        std::max(0, query.offset());

    const int limit =
        query.limit();

    if (offset < totalCount)
    {
        const int end =
            query.hasLimit()
                ? std::min(totalCount, offset + limit)
                : totalCount;

        for (int index = offset; index < end; ++index)
        {
            page.push_back(filteredRecordings.at(
                static_cast<std::size_t>(index)));
        }
    }

    return VdrRecordingQueryResult(
        page,
        totalCount,
        limit,
        offset);
}
