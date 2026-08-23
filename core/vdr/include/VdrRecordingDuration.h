#pragma once

#include "VdrRecording.h"

#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <limits>
#include <locale>
#include <sstream>
#include <string>
#include <vector>

namespace vdrsuite
{
namespace recording
{

constexpr std::uintmax_t VdrIndexEntryBytes = 8;

inline std::filesystem::path recordingDirectory(
    const VdrRecording& recording)
{
    const std::string path = !recording.backendNativeId.empty()
        ? recording.backendNativeId
        : recording.path;
    const std::filesystem::path directory(path);
    return directory.is_absolute() ? directory : std::filesystem::path{};
}

inline std::filesystem::path firstRegularFile(
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

inline double framesPerSecond(
    const std::filesystem::path& infoFile)
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
        double value = 0.0;
        if (fields >> value &&
            std::isfinite(value) &&
            value > 0.0 &&
            value <= 240.0)
        {
            return value;
        }

        return 0.0;
    }

    return 0.0;
}

inline int durationSecondsFromIndex(
    const VdrRecording& recording)
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

    const double fps = framesPerSecond(infoFile);
    if (fps <= 0.0)
    {
        return 0;
    }

    const std::uintmax_t frameCount = indexBytes / VdrIndexEntryBytes;
    const double durationSeconds =
        static_cast<double>(frameCount) / fps;
    if (!std::isfinite(durationSeconds) ||
        durationSeconds < 1.0 ||
        durationSeconds > static_cast<double>(std::numeric_limits<int>::max()))
    {
        return 0;
    }

    return static_cast<int>(durationSeconds);
}

inline bool enrichFromIndex(VdrRecording& recording)
{
    if (recording.recordingDurationKnown && recording.durationSeconds > 0)
    {
        return true;
    }

    const int durationSeconds = durationSecondsFromIndex(recording);
    if (durationSeconds <= 0)
    {
        return false;
    }

    recording.durationSeconds = durationSeconds;
    recording.recordingDurationKnown = true;
    return true;
}

inline bool normalizeForCatalog(VdrRecording& recording)
{
    if (recording.recordingDurationKnown && recording.durationSeconds > 0)
    {
        return true;
    }

    if (enrichFromIndex(recording))
    {
        return true;
    }

    // event_duration or any other non-authoritative fallback must never be
    // presented as the duration of the actual recording.
    recording.durationSeconds = 0;
    recording.recordingDurationKnown = false;
    return false;
}

inline void normalizeForCatalog(
    std::vector<VdrRecording>& recordings)
{
    for (VdrRecording& recording : recordings)
    {
        normalizeForCatalog(recording);
    }
}

} // namespace recording
} // namespace vdrsuite
