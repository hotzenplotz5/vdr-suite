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

    return true;
}
