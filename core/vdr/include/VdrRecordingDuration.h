#pragma once

#include "VdrRecording.h"

#include <array>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <limits>
#include <locale>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

namespace vdrsuite
{
namespace recording
{

constexpr std::uintmax_t VdrIndexEntryBytes = 8;
constexpr std::uintmax_t VdrTsPacketBytes = 188;
constexpr std::uintmax_t VdrMaximumFrameBytes =
    (3 * 1024 * 1024 / VdrTsPacketBytes) * VdrTsPacketBytes;
constexpr std::uintmax_t VdrMaximumIndexedTailBytes =
    VdrMaximumFrameBytes + 64 * VdrTsPacketBytes;

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

inline bool completedRecordingFiles(
    const std::filesystem::path& directory,
    std::filesystem::path& indexFile,
    std::filesystem::path& infoFile)
{
    if (directory.empty())
    {
        return false;
    }

    std::error_code directoryError;
    if (!std::filesystem::is_directory(directory, directoryError) ||
        directoryError)
    {
        return false;
    }

    std::error_code timerError;
    const bool stillRecording =
        std::filesystem::exists(directory / ".timer", timerError);
    if (timerError || stillRecording)
    {
        return false;
    }

    indexFile = firstRegularFile(directory, {"index", "index.vdr"});
    infoFile = firstRegularFile(directory, {"info", "info.vdr"});
    return !indexFile.empty() && !infoFile.empty();
}

inline int modernSegmentNumber(const std::string& path)
{
    const std::string name =
        std::filesystem::path(path).filename().string();
    if (name.size() != 8 || name.substr(5) != ".ts")
    {
        return 0;
    }

    int number = 0;
    for (std::size_t index = 0; index < 5; ++index)
    {
        const char character = name[index];
        if (character < '0' || character > '9')
        {
            return 0;
        }
        number = number * 10 + (character - '0');
    }

    return number > 0 && number <= 65535 ? number : 0;
}

inline bool littleEndianTsIndexLayout()
{
    const std::uint16_t endianProbe = 1;
    return *reinterpret_cast<const unsigned char*>(&endianProbe) == 1;
}

inline bool modernTsIndexComplete(
    const std::filesystem::path& directory,
    const std::filesystem::path& indexFile)
{
    if (!littleEndianTsIndexLayout())
    {
        return false;
    }

    int finalSegmentNumber = 0;
    std::filesystem::path finalSegmentPath;
    std::error_code iteratorError;
    std::filesystem::directory_iterator iterator(directory, iteratorError);
    const std::filesystem::directory_iterator end;
    if (iteratorError)
    {
        return false;
    }

    for (; iterator != end; iterator.increment(iteratorError))
    {
        if (iteratorError)
        {
            return false;
        }

        std::error_code regularError;
        if (!iterator->is_regular_file(regularError) || regularError)
        {
            continue;
        }

        const int number = modernSegmentNumber(iterator->path().string());
        if (number > finalSegmentNumber)
        {
            finalSegmentNumber = number;
            finalSegmentPath = iterator->path();
        }
    }

    if (iteratorError)
    {
        return false;
    }

    // Preserve the existing duration fallback for legacy PES recordings. The
    // Phase-65.D.2 indexed seek path itself only accepts modern TS segments.
    if (finalSegmentNumber <= 0 || finalSegmentPath.empty())
    {
        return true;
    }

    std::error_code indexSizeError;
    const std::uintmax_t indexBytes =
        std::filesystem::file_size(indexFile, indexSizeError);
    if (indexSizeError || indexBytes < VdrIndexEntryBytes ||
        indexBytes % VdrIndexEntryBytes != 0)
    {
        return false;
    }

    std::ifstream stream(indexFile, std::ios::binary);
    if (!stream)
    {
        return false;
    }

    stream.seekg(
        -static_cast<std::streamoff>(VdrIndexEntryBytes),
        std::ios::end);
    if (!stream)
    {
        return false;
    }

    std::array<unsigned char, VdrIndexEntryBytes> entry{};
    stream.read(
        reinterpret_cast<char*>(entry.data()),
        static_cast<std::streamsize>(entry.size()));
    if (stream.gcount() != static_cast<std::streamsize>(entry.size()))
    {
        return false;
    }

    std::uint64_t finalOffset = 0;
    for (std::size_t index = 0; index < 5; ++index)
    {
        finalOffset |= static_cast<std::uint64_t>(entry[index]) << (index * 8U);
    }
    const int indexedSegmentNumber =
        static_cast<int>(entry[6]) |
        (static_cast<int>(entry[7]) << 8);

    if (indexedSegmentNumber != finalSegmentNumber)
    {
        return false;
    }

    std::error_code segmentSizeError;
    const std::uintmax_t segmentBytes =
        std::filesystem::file_size(finalSegmentPath, segmentSizeError);
    if (segmentSizeError || segmentBytes == 0 ||
        finalOffset >= segmentBytes)
    {
        return false;
    }

    const std::uintmax_t trailingBytes =
        segmentBytes - static_cast<std::uintmax_t>(finalOffset);

    // VDR's tIndexTs stores the start offset of each frame. A completed index
    // therefore ends at the start of the final frame in the final TS segment.
    // VDR defines a single frame as at most ~3 MiB; the additional TS packets
    // cover PAT/PMT data that may precede an independent frame. A substantially
    // larger unindexed tail proves that the index is stale or incomplete.
    return trailingBytes <= VdrMaximumIndexedTailBytes;
}

inline int durationSecondsFromIndex(
    const VdrRecording& recording)
{
    const std::filesystem::path directory = recordingDirectory(recording);
    std::filesystem::path indexFile;
    std::filesystem::path infoFile;
    if (!completedRecordingFiles(directory, indexFile, infoFile))
    {
        return 0;
    }

    std::error_code sizeError;
    const std::uintmax_t indexBytes =
        std::filesystem::file_size(indexFile, sizeError);
    if (sizeError || indexBytes < VdrIndexEntryBytes ||
        indexBytes % VdrIndexEntryBytes != 0 ||
        !modernTsIndexComplete(directory, indexFile))
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

inline std::vector<double> segmentDurationsSecondsFromIndex(
    const VdrRecording& recording,
    const std::vector<std::string>& segmentPaths)
{
    if (segmentPaths.empty())
    {
        return {};
    }

    if (!littleEndianTsIndexLayout())
    {
        return {};
    }

    const std::filesystem::path directory = recordingDirectory(recording);
    std::filesystem::path indexFile;
    std::filesystem::path infoFile;
    if (!completedRecordingFiles(directory, indexFile, infoFile) ||
        !modernTsIndexComplete(directory, indexFile))
    {
        return {};
    }

    const double fps = framesPerSecond(infoFile);
    if (fps <= 0.0)
    {
        return {};
    }

    std::ifstream stream(indexFile, std::ios::binary);
    if (!stream)
    {
        return {};
    }

    std::map<int, std::uint64_t> framesBySegment;
    std::uint64_t totalFrames = 0;
    std::array<unsigned char, VdrIndexEntryBytes> entry{};

    while (true)
    {
        stream.read(
            reinterpret_cast<char*>(entry.data()),
            static_cast<std::streamsize>(entry.size()));
        const std::streamsize bytesRead = stream.gcount();
        if (bytesRead == 0)
        {
            break;
        }
        if (bytesRead != static_cast<std::streamsize>(entry.size()))
        {
            return {};
        }

        // tIndexTs is 8 packed bytes. On little-endian VDR the final two bytes
        // are the uint16_t recording file number.
        const int fileNumber =
            static_cast<int>(entry[6]) |
            (static_cast<int>(entry[7]) << 8);
        if (fileNumber <= 0)
        {
            return {};
        }

        ++framesBySegment[fileNumber];
        ++totalFrames;
    }

    if (totalFrames == 0)
    {
        return {};
    }

    std::vector<double> durations;
    durations.reserve(segmentPaths.size());
    std::set<int> seenSegments;
    std::uint64_t mappedFrames = 0;

    for (const std::string& segmentPath : segmentPaths)
    {
        const int fileNumber = modernSegmentNumber(segmentPath);
        if (fileNumber <= 0 || !seenSegments.insert(fileNumber).second)
        {
            return {};
        }

        const auto found = framesBySegment.find(fileNumber);
        if (found == framesBySegment.end() || found->second == 0)
        {
            return {};
        }

        const double durationSeconds =
            static_cast<double>(found->second) / fps;
        if (!std::isfinite(durationSeconds) || durationSeconds <= 0.0)
        {
            return {};
        }

        durations.push_back(durationSeconds);
        mappedFrames += found->second;
    }

    if (mappedFrames != totalFrames ||
        durations.size() != segmentPaths.size())
    {
        return {};
    }

    return durations;
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
