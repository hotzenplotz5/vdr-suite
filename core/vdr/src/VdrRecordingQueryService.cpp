#include "VdrRecordingQueryService.h"

#include "VdrRecordingCacheRepository.h"
#include "VdrRecordingDuration.h"
#include "VdrService.h"
#include "VdrRecordingQueryMatcher.h"

#include <algorithm>
#include <vector>

VdrRecordingQueryService::VdrRecordingQueryService(
    VdrService& vdrService,
    VdrRecordingCacheRepository* recordingCacheRepository,
    const std::string& defaultBackendId)
    : vdrService_(vdrService),
      recordingCacheRepository_(recordingCacheRepository),
      defaultBackendId_(defaultBackendId.empty() ? "default" : defaultBackendId)
{
}

VdrRecordingQueryResult VdrRecordingQueryService::queryRecordings(
    const VdrRecordingQuery& query) const
{
    const auto allRecordings =
        loadRecordings(query);

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

bool VdrRecordingQueryService::findRecordingById(
    const std::string& backendId,
    const std::string& recordingId,
    VdrRecording& recording) const
{
    if (recordingId.empty())
    {
        return false;
    }

    VdrRecordingQuery query = VdrRecordingQuery::all();
    if (!backendId.empty())
    {
        query.setBackendFilter(backendId);
    }

    const auto recordings = loadRecordings(query);
    const auto match = std::find_if(
        recordings.begin(),
        recordings.end(),
        [&recordingId](const VdrRecording& candidate)
        {
            return candidate.id == recordingId;
        });

    if (match == recordings.end())
    {
        return false;
    }

    recording = *match;
    vdrsuite::recording::enrichFromIndex(recording);
    return true;
}

std::vector<VdrRecording> VdrRecordingQueryService::loadRecordings(
    const VdrRecordingQuery& query) const
{
    const std::string backendId =
        effectiveBackendId(query);

    if (recordingCacheRepository_ != nullptr &&
        recordingCacheRepository_->countForBackend(backendId) > 0)
    {
        return recordingCacheRepository_->findAllForBackend(backendId);
    }

    if (query.hasBackendFilter() &&
        backendId != defaultBackendId_)
    {
        return {};
    }

    std::vector<VdrRecording> liveRecordings =
        vdrService_.getRecordings();

    vdrsuite::recording::normalizeForCatalog(liveRecordings);

    if (recordingCacheRepository_ != nullptr &&
        !liveRecordings.empty())
    {
        if (recordingCacheRepository_->replaceRecordingsForBackend(
                backendId,
                liveRecordings))
        {
            recordingCacheRepository_->markRefreshFinished(
                backendId,
                static_cast<int>(liveRecordings.size()));

            return recordingCacheRepository_->findAllForBackend(backendId);
        }
    }

    return liveRecordings;
}

std::string VdrRecordingQueryService::effectiveBackendId(
    const VdrRecordingQuery& query) const
{
    if (query.hasBackendFilter())
    {
        return query.backendFilter();
    }

    return defaultBackendId_;
}
