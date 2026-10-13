#pragma once

#include "HevcHlsSegmentPolicy.h"

#include <cmath>
#include <cstdlib>
#include <sstream>
#include <string>
#include <vector>

// Read-only, capped ffprobe command and conservative packet evidence parser.
// Caller must run within a hard timeout, verify an immutable source identity,
// and independently validate HEVC random-access semantics before marking
// packetKeyframesVerified; this parser alone never marks eligibility.
struct HevcHlsKeyframeProbePlan {
    std::vector<std::string> argv;
};

struct HevcHlsKeyframeProbeResult {
    bool parsed = false;
    bool truncated = false;
    std::vector<double> keyframePtsSeconds;
};

inline HevcHlsKeyframeProbePlan hevcHlsKeyframeProbePlan()
{
    return {{
        "/usr/bin/ffprobe", "-v", "error", "-f", "concat", "-safe", "1",
        "-read_intervals", "%+30", "-i", "input.ffconcat",
        "-select_streams", "v:0", "-show_packets",
        "-show_entries", "packet=pts_time,flags",
        "-of", "csv=p=0"
    }};
}

inline HevcHlsKeyframeProbeResult parseHevcHlsKeyframeProbe(
    const std::string& output)
{
    HevcHlsKeyframeProbeResult result;
    constexpr std::size_t MaxOutputBytes = 256 * 1024;
    constexpr std::size_t MaxPackets = 4096;
    if (output.empty() || output.size() > MaxOutputBytes) {
        result.truncated = output.size() > MaxOutputBytes;
        return result;
    }
    std::istringstream stream(output);
    std::string line;
    std::size_t packets = 0;
    bool previousPtsPresent = false;
    double previousPts = 0.0;
    while (std::getline(stream, line)) {
        if (++packets > MaxPackets) { result.truncated = true; return result; }
        if (!line.empty() && line.back() == '\r') line.pop_back();
        const auto comma = line.find(',');
        if (comma == std::string::npos || comma == 0) return result;
        const std::string value = line.substr(0, comma);
        char* end = nullptr;
        const double pts = std::strtod(value.c_str(), &end);
        if (end == value.c_str() || *end != '\0' ||
            !std::isfinite(pts) || pts < 0.0 ||
            (previousPtsPresent && pts <= previousPts)) return result;
        previousPtsPresent = true;
        previousPts = pts;
        const std::string flags = line.substr(comma + 1);
        if (flags.empty()) return result;
        if (flags[0] == 'K') result.keyframePtsSeconds.push_back(pts);
    }
    result.parsed = packets > 0;
    return result;
}
