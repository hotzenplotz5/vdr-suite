#include "RecordingSubtitleSidecar.h"

#include <algorithm>
#include <cerrno>
#include <cctype>
#include <filesystem>
#include <system_error>
#include <vector>
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

    // Keep the established 00001.srt priority. On recordings without it,
    // discover common named/language-tagged .srt sidecars in this trusted
    // recording directory only. Never follow symlinks or descend into dirs.
    std::filesystem::path sidecar = directory / "00001.srt";
    struct stat status {};
    if (::lstat(sidecar.c_str(), &status) != 0) {
        if (errno != ENOENT) {
            result.reasonCode = "recording_subtitle_sidecar_unavailable";
            return result;
        }
        std::error_code error;
        std::filesystem::directory_iterator it(directory, error);
        if (error) {
            result.reasonCode = "recording_subtitle_sidecar_unavailable";
            return result;
        }
        const std::filesystem::directory_iterator end;
        std::vector<std::filesystem::path> candidates;
        std::size_t inspected = 0;
        for (; it != end && !error; it.increment(error)) {
            if (++inspected > 4096) {
                result.reasonCode = "recording_subtitle_sidecar_directory_too_large";
                return result;
            }
            const std::string name = it->path().filename().string();
            if (name.empty() || name.front() == '.' || name.size() > 128 ||
                name.size() <= 4) continue;
            std::string suffix = name.substr(name.size() - 4);
            std::transform(suffix.begin(), suffix.end(), suffix.begin(),
                [](unsigned char character) {
                    return static_cast<char>(std::tolower(character));
                });
            if (suffix != ".srt") continue;
            // Only conservative, printable ASCII basenames: no control
            // characters, platform separators or unexpected encoding in
            // the public subtitle label.
            const bool safeName = std::all_of(name.begin(), name.end(),
                [](unsigned char character) {
                    return std::isalnum(character) || character == '.' ||
                        character == '_' || character == '-' || character == ' ';
                });
            if (!safeName) continue;
            candidates.push_back(it->path());
            if (candidates.size() > 32) {
                result.reasonCode = "recording_subtitle_sidecar_ambiguous";
                return result;
            }
        }
        if (error) {
            result.reasonCode = "recording_subtitle_sidecar_unavailable";
            return result;
        }
        if (candidates.empty()) {
            result.reasonCode = "recording_subtitle_sidecar_not_found";
            return result;
        }
        std::sort(candidates.begin(), candidates.end());
        sidecar = candidates.front();
        if (::lstat(sidecar.c_str(), &status) != 0) {
            result.reasonCode = "recording_subtitle_sidecar_unavailable";
            return result;
        }
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
    result.track.label = sidecar.filename() == "00001.srt" ?
        "SRT" : sidecar.filename().string();
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
