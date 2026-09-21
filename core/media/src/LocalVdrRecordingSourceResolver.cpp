#include "LocalVdrRecordingSourceResolver.h"

#include "RecordingSourceFingerprint.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <utility>

namespace
{

bool isDigits(const std::string& value)
{
    return !value.empty() &&
        std::all_of(
            value.begin(),
            value.end(),
            [](unsigned char character) {
                return std::isdigit(character) != 0;
            });
}

bool isModernSegment(const std::string& name)
{
    return name.size() == 8 &&
        name.substr(5) == ".ts" &&
        isDigits(name.substr(0, 5));
}

bool isLegacySegment(const std::string& name)
{
    return name.size() == 7 &&
        name.substr(3) == ".vdr" &&
        isDigits(name.substr(0, 3));
}

std::string preferredInternalDirectory(const VdrRecording& recording)
{
    if (!recording.backendNativeId.empty()) {
        return recording.backendNativeId;
    }

    return recording.path;
}

bool modernTransportStreamSet(const std::vector<std::string>& segments)
{
    return !segments.empty() &&
        std::all_of(
            segments.begin(),
            segments.end(),
            [](const std::string& path) {
                return isModernSegment(
                    std::filesystem::path(path).filename().string());
            });
}

} // namespace

LocalVdrRecordingSourceResolver::LocalVdrRecordingSourceResolver(
    RecordingCatalog recordingCatalog)
    : recordingCatalog_(std::move(recordingCatalog))
{
}

LocalVdrRecordingSourceResolution LocalVdrRecordingSourceResolver::resolve(
    const std::string& backendId,
    const std::string& recordingId) const
{
    LocalVdrRecordingSourceResolution resolution;

    if (backendId.empty() || recordingId.empty() || !recordingCatalog_) {
        resolution.reasonCode = "invalid_recording_reference";
        return resolution;
    }

    const std::vector<VdrRecording> recordings = recordingCatalog_(backendId);
    const auto iterator = std::find_if(
        recordings.begin(),
        recordings.end(),
        [&recordingId](const VdrRecording& recording) {
            return recording.id == recordingId;
        });

    if (iterator == recordings.end()) {
        resolution.reasonCode = "recording_not_found";
        return resolution;
    }

    const std::string directory = preferredInternalDirectory(*iterator);
    if (directory.empty() || !std::filesystem::path(directory).is_absolute()) {
        resolution.reasonCode = "recording_internal_path_unavailable";
        return resolution;
    }

    const std::vector<std::string> segments = discoverSegments(directory);
    if (segments.empty()) {
        resolution.reasonCode = "recording_segments_unavailable";
        return resolution;
    }

    const RecordingSourceFingerprint fingerprint =
        inspectRecordingSource(directory, segments);
    if (!fingerprint.valid) {
        resolution.reasonCode = fingerprint.reasonCode.empty()
            ? "recording_source_unavailable"
            : fingerprint.reasonCode;
        return resolution;
    }

    resolution.resolved = true;
    resolution.source.backendId = backendId;
    resolution.source.recordingId = recordingId;
    resolution.source.recordingDirectory = directory;
    resolution.source.segmentPaths = segments;
    resolution.source.growing = fingerprint.growing;
    resolution.source.progressiveDirectSafe = modernTransportStreamSet(segments);
    resolution.source.readableBytes = fingerprint.readableBytes;
    resolution.source.sourceFingerprint = fingerprint.value;
    return resolution;
}

std::vector<std::string> LocalVdrRecordingSourceResolver::discoverSegments(
    const std::string& recordingDirectory)
{
    std::vector<std::string> segments;
    const std::filesystem::path directory(recordingDirectory);
    std::error_code error;

    if (!directory.is_absolute() ||
        !std::filesystem::is_directory(directory, error) ||
        error) {
        return segments;
    }

    for (std::filesystem::directory_iterator iterator(directory, error), end;
         !error && iterator != end;
         iterator.increment(error)) {
        const std::filesystem::directory_entry& entry = *iterator;
        const std::string name = entry.path().filename().string();

        if (!isModernSegment(name) && !isLegacySegment(name)) {
            continue;
        }

        std::error_code typeError;
        if (!entry.is_regular_file(typeError) || typeError) {
            continue;
        }

        segments.push_back(entry.path().string());
    }

    if (error) {
        return {};
    }

    std::sort(segments.begin(), segments.end());
    return segments;
}
