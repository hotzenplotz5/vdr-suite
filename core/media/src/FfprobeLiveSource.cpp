#include "FfprobeLiveSource.h"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <string>

namespace
{
constexpr std::size_t MaximumLiveSocketPathLength = 100;
constexpr const char* LiveIoTimeoutMicros = "2000000";
constexpr const char* LiveAnalyzeDurationMicros = "500000";
constexpr const char* LiveProbeSizeBytes = "524288";

bool validSocketPath(const std::string& value)
{
    if (value.empty() || value.front() != '/' ||
        value.size() > MaximumLiveSocketPathLength ||
        value.find("..") != std::string::npos ||
        value.find('?') != std::string::npos ||
        value.find('#') != std::string::npos) return false;
    return std::all_of(value.begin(), value.end(), [](unsigned char character) {
        return std::isalnum(character) != 0 || character == '/' ||
            character == '-' || character == '_' || character == '.' ||
            character == ':';
    });
}

bool knownVideoScanType(const std::string& output)
{
    std::istringstream stream(output);
    std::string line;
    while (std::getline(stream, line)) {
        if (line.find("codec_type=video") == std::string::npos) continue;

        const std::string marker = "field_order=";
        const std::size_t start = line.find(marker);
        if (start == std::string::npos) return false;
        const std::size_t valueStart = start + marker.size();
        const std::size_t end = line.find('|', valueStart);
        const std::string value = line.substr(
            valueStart,
            end == std::string::npos ? std::string::npos : end - valueStart);
        if (value != "progressive" && value != "tt" && value != "bb" &&
            value != "tb" && value != "bt") {
            return false;
        }
    }
    return true;
}
}

FfprobeLivePlan FfprobeLiveSource::commandPlan(
    const std::string& unixSocketPath) const
{
    FfprobeLivePlan plan;
    if (!validSocketPath(unixSocketPath)) {
        plan.reasonCode = "invalid_live_source_socket";
        return plan;
    }
    plan.argv = {
        "/usr/bin/ffprobe",
        "-v", "error",
        "-rw_timeout", LiveIoTimeoutMicros,
        "-analyzeduration", LiveAnalyzeDurationMicros,
        "-probesize", LiveProbeSizeBytes,
        "-f", "mpegts",
        "-show_entries",
        "stream=codec_type,codec_name,width,height,r_frame_rate,field_order,channels:stream_tags=language",
        "-of", "compact=p=0:nk=0",
        "-i", "unix://" + unixSocketPath
    };
    plan.valid = true;
    return plan;
}

FfprobeRecordingResult FfprobeLiveSource::parse(const std::string& output) const
{
    FfprobeRecordingSource recordingProbe;
    auto result = recordingProbe.parse(output);
    if (!result.valid) return result;
    if (!knownVideoScanType(output)) {
        result.valid = false;
        result.reasonCode = "live_video_scan_type_unknown";
        return result;
    }
    result.source.resourceKind = MediaResourceKind::LiveChannel;
    result.source.container = MediaContainer::MpegTs;
    result.source.seekable = false;
    result.source.growing = true;
    return result;
}
