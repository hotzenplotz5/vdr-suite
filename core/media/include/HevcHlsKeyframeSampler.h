#pragma once

#include "HevcHlsKeyframeProbe.h"
#include "MediaProcessRunner.h"

#include <chrono>
#include <cstddef>
#include <string>

// Untrusted bounded packet sampling only. Does NOT confer source-identity
// attestation or independently validate HEVC IDR/CRA random access.
struct HevcHlsKeyframeSample {
    bool complete = false;
    std::string reasonCode;
    HevcHlsKeyframeProbeResult packets;
};

inline HevcHlsKeyframeSample sampleHevcHlsKeyframes(
    const MediaProcessRunner& runner,
    const std::string& preparedWorkspace)
{
    HevcHlsKeyframeSample sample;
    if (preparedWorkspace.empty() || preparedWorkspace.front() != '/') {
        sample.reasonCode = "invalid_probe_workspace";
        return sample;
    }
    // FFprobe/codec libraries reserve more than 256 MiB of virtual address
    // space on the tested HEVC TS input. RLIMIT_AS=256 MiB fails even though
    // the same bounded probe succeeds without it. Keep wall/CPU/output caps.
    constexpr std::size_t MaxOutput = 256 * 1024;
    const auto process = runner.runAndCapture(
        hevcHlsKeyframeProbePlan().argv,
        preparedWorkspace,
        std::chrono::seconds(5),
        MaxOutput,
        MediaProcessLimits{0, 6});
    if (!process.started || !process.completed || !process.success ||
        process.timedOut || process.outputLimitExceeded ||
        process.exitCode != 0) {
        sample.reasonCode = process.reasonCode.empty()
            ? "hevc_keyframe_probe_failed" : process.reasonCode;
        return sample;
    }
    sample.packets = parseHevcHlsKeyframeProbe(process.output);
    if (!sample.packets.parsed || sample.packets.truncated) {
        sample.reasonCode = "invalid_hevc_keyframe_packet_output";
        return sample;
    }
    sample.complete = true;
    return sample;
}
