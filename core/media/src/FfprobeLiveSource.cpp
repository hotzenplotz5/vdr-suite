#include "FfprobeLiveSource.h"

#include <algorithm>
#include <cctype>

namespace
{
constexpr std::size_t MaximumLiveSocketPathLength = 100;

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
        "-f", "mpegts",
        "-read_intervals", "%+3",
        "-show_entries",
        "stream=codec_type,codec_name,width,height,r_frame_rate,field_order,channels:stream_tags=language",
        "-of", "compact=p=0:nk=0",
        "-i", "unix://" + unixSocketPath + "?timeout=5000000&type=stream"
    };
    plan.valid = true;
    return plan;
}

FfprobeRecordingResult FfprobeLiveSource::parse(const std::string& output) const
{
    FfprobeRecordingSource recordingProbe;
    auto result = recordingProbe.parse(output);
    if (!result.valid) return result;
    result.source.resourceKind = MediaResourceKind::LiveChannel;
    result.source.container = MediaContainer::MpegTs;
    result.source.seekable = false;
    result.source.growing = false;
    return result;
}
