#include "RecordingSourceFingerprint.h"

#include <cerrno>
#include <filesystem>
#include <limits>
#include <sstream>
#include <string>
#include <sys/stat.h>

namespace
{

RecordingSourceFingerprint invalid(const std::string& reasonCode)
{
    RecordingSourceFingerprint result;
    result.reasonCode = reasonCode;
    return result;
}

void appendTextField(std::ostringstream& stream, const std::string& value)
{
    stream << value.size() << ':' << value;
}

} // namespace

RecordingSourceFingerprint inspectRecordingSource(
    const std::string& recordingDirectory,
    const std::vector<std::string>& segmentPaths)
{
    const std::filesystem::path directory =
        std::filesystem::path(recordingDirectory).lexically_normal();
    if (!directory.is_absolute() || segmentPaths.empty()) {
        return invalid("invalid_recording_source");
    }

    RecordingSourceFingerprint result;
    std::ostringstream fingerprint;
    fingerprint << "v1|directory=";
    appendTextField(fingerprint, directory.string());

    for (const std::string& segmentPath : segmentPaths) {
        const std::filesystem::path segment =
            std::filesystem::path(segmentPath).lexically_normal();
        if (!segment.is_absolute() || segment.parent_path() != directory) {
            return invalid("recording_source_segment_outside_directory");
        }

        struct stat status {};
        if (::lstat(segment.c_str(), &status) != 0 || !S_ISREG(status.st_mode) ||
            status.st_size < 0) {
            return invalid("recording_source_segment_unavailable");
        }

        const auto size = static_cast<std::uint64_t>(status.st_size);
        if (result.readableBytes >
            std::numeric_limits<std::uint64_t>::max() - size) {
            return invalid("recording_source_extent_overflow");
        }
        result.readableBytes += size;

        fingerprint << "|segment=";
        appendTextField(fingerprint, segment.filename().string());
        fingerprint << ':'
                    << static_cast<unsigned long long>(status.st_dev) << ':'
                    << static_cast<unsigned long long>(status.st_ino) << ':'
                    << size << ':'
                    << static_cast<long long>(status.st_mtim.tv_sec) << ':'
                    << static_cast<long long>(status.st_mtim.tv_nsec);
    }

    const std::filesystem::path timerMarker = directory / ".timer";
    struct stat markerStatus {};
    if (::lstat(timerMarker.c_str(), &markerStatus) == 0) {
        // VDR owns .timer as its recording-timer indicator. Any existing entry
        // is treated conservatively as growing; an unexpected file type must
        // never make a source look completed.
        result.growing = true;
    }
    else if (errno != ENOENT) {
        return invalid("recording_source_state_unavailable");
    }

    fingerprint << "|growing=" << (result.growing ? '1' : '0');
    result.value = fingerprint.str();
    result.valid = true;
    return result;
}
