#include "RecordingTrickplayPreviewPlan.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <string>

namespace {
constexpr int MaximumRecordingSeconds = 12 * 60 * 60;
constexpr std::size_t MaximumSourceSegments = 1024;
constexpr int PreviewStepSeconds = 10;
constexpr int MaximumPreviewSeconds = 12 * 60 * 60;

RecordingTrickplayPreviewPlan invalid(const std::string& reason)
{
    RecordingTrickplayPreviewPlan plan;
    plan.reasonCode = reason;
    return plan;
}
}

RecordingTrickplayPreviewPlan RecordingTrickplayPreviewPlanner::build(
    int requestedSeconds,
    int indexedDurationSeconds,
    std::size_t verifiedSourceSegmentCount,
    const std::vector<double>& verifiedSegmentDurationsSeconds) const
{
    // Never use the HLS playback clock as permission for an approximate
    // FFmpeg seek. Only a completed, trusted VDR index may supply this data.
    if (indexedDurationSeconds <= 0 ||
        indexedDurationSeconds > MaximumRecordingSeconds ||
        verifiedSourceSegmentCount == 0 ||
        verifiedSourceSegmentCount > MaximumSourceSegments ||
        verifiedSegmentDurationsSeconds.size() != verifiedSourceSegmentCount)
        return invalid("trickplay_index_unavailable");
    if (requestedSeconds < 0 || requestedSeconds >= indexedDurationSeconds)
        return invalid("trickplay_position_out_of_bounds");

    double mappedSeconds = 0;
    for (double length : verifiedSegmentDurationsSeconds) {
        if (!std::isfinite(length) || length <= 0.0 ||
            length > MaximumRecordingSeconds)
            return invalid("trickplay_index_unavailable");
        mappedSeconds += length;
        if (!std::isfinite(mappedSeconds) ||
            mappedSeconds > MaximumRecordingSeconds + 10)
            return invalid("trickplay_index_unavailable");
    }
    // Reject timelines whose declared overall duration conflicts with the
    // trusted per-segment index. The tolerance accommodates integer-second
    // rounding; it cannot be used to invent missing source segments.
    const double tolerance = std::max(2.0,
        std::min(10.0, static_cast<double>(verifiedSourceSegmentCount)));
    if (std::abs(mappedSeconds - indexedDurationSeconds) > tolerance)
        return invalid("trickplay_index_inconsistent");

    RecordingTrickplayPreviewPlan plan;
    plan.requestedSeconds = requestedSeconds;
    plan.intervalSeconds = PreviewStepSeconds;
    plan.sampleSeconds = (requestedSeconds / PreviewStepSeconds) *
                         PreviewStepSeconds;
    if (plan.sampleSeconds >= indexedDurationSeconds ||
        plan.sampleSeconds > MaximumPreviewSeconds)
        return invalid("trickplay_position_out_of_bounds");

    // FFmpeg seeks a trusted input.ffconcat in its privately owned working
    // directory. Paths and filters cannot be influenced by request content.
    // Re-encode ONE MJPEG still; never transcode a rolling video playlist.
    plan.argv = {
        "/usr/bin/ffmpeg", "-nostdin", "-hide_banner", "-loglevel", "error",
        "-max_alloc", "134217728", "-threads", "1", "-filter_threads", "1",
        "-protocol_whitelist", "file,pipe",
        "-ss", std::to_string(plan.sampleSeconds),
        "-f", "concat", "-safe", "1", "-i", "input.ffconcat",
        "-map", "0:v:0", "-frames:v", "1",
        "-an", "-sn", "-dn", "-map_metadata", "-1",
        "-vf", "scale=320:-2:flags=fast_bilinear,setsar=1",
        "-threads", "1", "-c:v", "mjpeg", "-q:v", "5",
        "-pix_fmt", "yuvj420p", "-f", "image2pipe", "pipe:1"
    };
    plan.valid = true;
    return plan;
}
