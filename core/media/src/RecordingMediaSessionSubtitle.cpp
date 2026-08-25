#include "RecordingMediaSessionRuntime.h"

#include "MediaProcessRunner.h"
#include "MediaSessionWorkspace.h"
#include "RecordingSubtitleWebVtt.h"

#include <chrono>
#include <string>

namespace
{

constexpr auto SubtitleExtractionTimeout = std::chrono::seconds(60);
constexpr std::size_t MaximumSubtitleOutputBytes = 16U * 1024U * 1024U;

std::string processFailureReason(const MediaProcessCaptureResult& process)
{
    if (process.timedOut) return "recording_subtitle_extract_timeout";
    if (process.outputLimitExceeded) return "recording_subtitle_output_too_large";
    if (!process.started) return "recording_subtitle_extract_start_failed";
    return "recording_subtitle_extract_failed";
}

bool validWebVtt(const std::string& value)
{
    return value.rfind("WEBVTT", 0) == 0;
}

} // namespace

RecordingMediaSessionSubtitleWebVttResult
RecordingMediaSessionRuntime::subtitleWebVtt(
    const std::string& sessionId,
    int sourceSubtitleStreamIndex,
    MediaSubtitleFormat format)
{
    RecordingMediaSessionSubtitleWebVttResult result;
    result.sourceSubtitleStreamIndex = sourceSubtitleStreamIndex;

    const RecordingSubtitleWebVttPlan plan =
        RecordingSubtitleWebVtt::build(sourceSubtitleStreamIndex, format);
    if (!plan.valid) {
        result.reasonCode = plan.reasonCode.empty()
            ? "recording_subtitle_delivery_not_supported"
            : plan.reasonCode;
        return result;
    }

    std::string workspaceDirectory;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        const auto found = active_.find(sessionId);
        if (found == active_.end() || found->second.direct ||
            !found->second.workspace) {
            result.reasonCode = "recording_subtitle_session_not_available";
            return result;
        }
        const auto cached = found->second.subtitleWebVttCache.find(
            sourceSubtitleStreamIndex);
        if (cached != found->second.subtitleWebVttCache.end()) {
            result.ready = true;
            result.webVtt = cached->second;
            return result;
        }
        workspaceDirectory = found->second.workspace->directory();
    }

    if (workspaceDirectory.empty()) {
        result.reasonCode = "recording_subtitle_workspace_not_available";
        return result;
    }

    const MediaProcessCaptureResult process = MediaProcessRunner().runAndCapture(
        plan.argv,
        workspaceDirectory,
        SubtitleExtractionTimeout,
        MaximumSubtitleOutputBytes);
    if (!process.success) {
        result.reasonCode = processFailureReason(process);
        return result;
    }
    if (!validWebVtt(process.output)) {
        result.reasonCode = "recording_subtitle_webvtt_invalid";
        return result;
    }

    {
        std::lock_guard<std::mutex> lock(mutex_);
        const auto found = active_.find(sessionId);
        if (found == active_.end() || found->second.direct ||
            !found->second.workspace ||
            found->second.workspace->directory() != workspaceDirectory) {
            result.reasonCode = "recording_subtitle_session_changed";
            return result;
        }
        found->second.subtitleWebVttCache[sourceSubtitleStreamIndex] = process.output;
    }

    result.ready = true;
    result.webVtt = process.output;
    return result;
}
