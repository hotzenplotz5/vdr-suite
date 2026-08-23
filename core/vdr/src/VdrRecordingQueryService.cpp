#include "VdrRecordingQueryService.h"

#include "VdrRecordingCacheRepository.h"
#include "VdrService.h"
#include "VdrRecordingQueryMatcher.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <limits>
#include <locale>
#include <sstream>
#include <string>
#include <vector>

namespace
{

constexpr std::uintmax_t VdrIndexEntryBytes = 8;

std::filesystem::path recordingDirectory(const VdrRecording& recording)
{
    const std::string path = !recording.backendNativeId.empty()
        ? recording.backendNativeId
        : recording.path;
    const std::filesystem::path directory(path);
    return directory.is_absolute() ? directory : std::filesystem::path{};
}

std::filesystem::path firstRegularFile(
    const std::filesystem::path& directory,
    const std::vector<std::string>& names)
{
    for (const std::string& name : names)
    {
        const std::filesystem::path candidate = directory / name;
        std::error_code error;
        if (std::filesystem::is_regular_file(candidate, error) && !error)
        {
            return candidate;
        }
    }

    return {};
}

double recordingFramesPerSecond(const std::filesystem::path& infoFile)
{
    std::ifstream stream(infoFile);
    if (!stream)
    {
        return 0.0;
    }

    std::string line;
    while (std::getline(stream, line))
    {
        if (line.size() < 3 || line.front() != 'F' ||
            (line[1] != ' ' && line[1] != '\t'))
        {
            continue;
        }

        std::istringstream fields(line.substr(1));
        fields.imbue(std::locale::classic());
        double framesPerSecond = 0.0;
        if (fields >> framesPerSecond &&
            std::isfinite(framesPerSecond) &&
            framesPerSecond > 0.0 &&
            framesPerSecond <= 240.0)
        {
            return framesPerSecond;
        }

        return 0.0;
    }

    return 0.0;
}

int recordingDurationFromVdrIndex(const VdrRecording& recording)
{
    const std::filesystem::path directory = recordingDirectory(recording);
    if (directory.empty())
    {
        return 0;
    }

    std::error_code directoryError;
    if (!std::filesystem::is_directory(directory, directoryError) ||
        directoryError)
    {
        return 0;
    }

    std::error_code timerError;
    const bool stillRecording =
        std::filesystem::exists(directory / ".timer", timerError);
    if (timerError || stillRecording)
    {
        return 0;
    }

    const std::filesystem::path indexFile = firstRegularFile(
        directory,
        {"index", "index.vdr"});
    const std::filesystem::path infoFile = firstRegularFile(
        directory,
        {"info", "info.vdr"});
    if (indexFile.empty() || infoFile.empty())
    {
        return 0;
    }

    std::error_code sizeError;
    const std::uintmax_t indexBytes =
        std::filesystem::file_size(indexFile, sizeError);
    if (sizeError || indexBytes < VdrIndexEntryBytes ||
        indexBytes % VdrIndexEntryBytes != 0)
    {
        return 0;
    }

    const double framesPerSecond = recordingFramesPerSecond(infoFile);
    if (framesPerSecond <= 0.0)
    {
        return 0;
    }

    const std::uintmax_t frameCount = indexBytes / VdrIndexEntryBytes;
    const double durationSeconds =
        static_cast<double>(frameCount) / framesPerSecond;
    if (!std::isfinite(durationSeconds) ||
        durationSeconds < 1.0 ||
        durationSeconds > static_cast<double>(std::numeric_limits<int>::max()))
    {
        return 0;
    }

    return static_cast<int>(durationSeconds);
}

void enrichRecordingDurationFromVdrIndex(VdrRecording& recording)
{
    if (recording.recordingDurationKnown && recording.durationSeconds > 0)
    {
        return;
    }

    const int durationSeconds = recordingDurationFromVdrIndex(recording);
    if (durationSeconds > 0)
    {
        recording.durationSeconds = durationSeconds;
        recording.recordingDurationKnown = true;
    }
}

} // namespace

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
    enrichRecordingDurationFromVdrIndex(recording);
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
