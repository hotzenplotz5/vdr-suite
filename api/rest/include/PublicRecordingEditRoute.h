#pragma once

// Shared, bounded path/query recognition for the Public API v1 recording
// editing facade. The legacy RecordingMarks/Cut runtimes remain the sole
// native Recording editing owners.
#include <algorithm>
#include <cctype>
#include <string>

namespace PublicRecordingEditRoute
{
inline bool matchPath(
    const std::string& path,
    const std::string& suffix,
    std::string& recordingId)
{
    constexpr const char* prefix = "/api/v1/recordings/";
    const std::string base(prefix);
    recordingId.clear();
    if (path.compare(0U, base.size(), base) != 0 ||
        path.size() <= base.size() + suffix.size() ||
        path.compare(path.size() - suffix.size(), suffix.size(), suffix) != 0)
        return false;
    recordingId = path.substr(
        base.size(), path.size() - base.size() - suffix.size());
    return recordingId.find('/') == std::string::npos;
}

inline bool validRecordingId(const std::string& value)
{
    return value.size() == 36U && value.compare(0U, 4U, "rec_") == 0 &&
        std::all_of(value.begin() + 4, value.end(), [](unsigned char c) {
            return (c >= '0' && c <= '9') ||
                   (c >= 'a' && c <= 'f');
        });
}

inline bool backendFromTarget(
    const std::string& requestTarget,
    std::string& backendId)
{
    backendId.clear();
    const std::size_t index = requestTarget.find('?');
    if (index == std::string::npos) return false;
    const std::string query = requestTarget.substr(index + 1U);
    constexpr const char* key = "backendId=";
    if (query.compare(0U, 10U, key) != 0) return false;
    backendId = query.substr(10U);
    return !backendId.empty() && backendId.size() <= 128U &&
        backendId != "*" && backendId != "." && backendId != ".." &&
        std::all_of(backendId.begin(), backendId.end(),
            [](unsigned char c) {
                return std::isalnum(c) || c == '.' || c == '_' || c == '-';
            });
}
}
