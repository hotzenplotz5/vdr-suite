#include "RecordingSubtitleSidecar.h"

#include <algorithm>
#include <cerrno>
#include <filesystem>
#include <sys/stat.h>

RecordingSubtitleSidecarResult RecordingSubtitleSidecar::discover(
    const std::string& recordingDirectory)
{
    RecordingSubtitleSidecarResult result;

    const std::filesystem::path directory =
        std::filesystem::path(recordingDirectory).lexically_normal();
    if (!directory.is_absolute()) {
        result.reasonCode = "invalid_recording_subtitle_sidecar_directory";
        return result;
    }

    const std::filesystem::path sidecar = directory / "00001.srt";
    struct stat status {};
    if (::lstat(sidecar.c_str(), &status) != 0) {
        result.reasonCode = errno == ENOENT
            ? "recording_subtitle_sidecar_not_found"
            : "recording_subtitle_sidecar_unavailable";
        return result;
    }
    if (!S_ISREG(status.st_mode)) {
        result.reasonCode = "recording_subtitle_sidecar_not_regular_file";
        return result;
    }
    if (status.st_size <= 0) {
        result.reasonCode = "recording_subtitle_sidecar_empty";
        return result;
    }

    result.available = true;
    result.path = sidecar.string();
    result.track.format = MediaSubtitleFormat::SubRip;
    result.track.label = "SRT";
    result.track.externalSourcePath = result.path;
    return result;
}

bool RecordingSubtitleSidecar::appendTo(
    MediaSourceDescriptor& source,
    const std::string& recordingDirectory)
{
    const RecordingSubtitleSidecarResult sidecar = discover(recordingDirectory);
    if (!sidecar.available) return false;

    const auto existing = std::find_if(
        source.subtitleStreams.begin(),
        source.subtitleStreams.end(),
        [&sidecar](const MediaSubtitleStreamDescriptor& track) {
            return !track.externalSourcePath.empty() &&
                track.externalSourcePath == sidecar.path;
        });
    if (existing == source.subtitleStreams.end()) {
        source.subtitleStreams.push_back(sidecar.track);
    }
    return true;
}
