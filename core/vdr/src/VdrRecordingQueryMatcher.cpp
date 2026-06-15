#include "VdrRecordingQueryMatcher.h"

#include <algorithm>
#include <cctype>
#include <string>

namespace
{
std::string lowerCopy(
    const std::string& value)
{
    std::string result = value;

    std::transform(
        result.begin(),
        result.end(),
        result.begin(),
        [](unsigned char character)
        {
            return static_cast<char>(
                std::tolower(character));
        });

    return result;
}

bool containsCaseInsensitive(
    const std::string& value,
    const std::string& needle)
{
    return lowerCopy(value).find(
               lowerCopy(needle)) != std::string::npos;
}
}

bool VdrRecordingQueryMatcher::matches(
    const VdrRecording& recording,
    const VdrRecordingQuery& query) const
{
    if (query.hasTitleFilter() &&
        !containsCaseInsensitive(
            recording.title,
            query.titleFilter()))
    {
        return false;
    }

    if (query.hasPathFilter() &&
        !containsCaseInsensitive(
            recording.path,
            query.pathFilter()))
    {
        return false;
    }

    if (query.hasBackendFilter())
    {
        const bool legacyDefaultMatch =
            query.backendFilter() == "default" &&
            recording.backendId.empty();

        if (!legacyDefaultMatch &&
            recording.backendId != query.backendFilter())
        {
            return false;
        }
    }

    if (query.hasFromStartTime() &&
        recording.startTime < query.fromStartTime())
    {
        return false;
    }

    if (query.hasToStartTime() &&
        recording.startTime > query.toStartTime())
    {
        return false;
    }

    if (query.hasMinDurationSeconds() &&
        recording.durationSeconds < query.minDurationSeconds())
    {
        return false;
    }

    if (query.hasMaxDurationSeconds() &&
        recording.durationSeconds > query.maxDurationSeconds())
    {
        return false;
    }

    return true;
}
