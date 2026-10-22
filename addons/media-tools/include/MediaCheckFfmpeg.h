#pragma once

#include "MediaCheck.h"

#include <chrono>
#include <string>

namespace vdrsuite::mediatools {

// Backend-local, read-only software decode into FFmpeg's null muxer.
// The CLI always uses /usr/bin/ffmpeg. The binary argument exists only for
// isolated executable-fixture testing, not for untrusted clients or manifests.
class FfmpegMediaCheckDecoder final : public MediaCheckDecoder {
public:
    explicit FfmpegMediaCheckDecoder(
        std::chrono::seconds deadline = std::chrono::seconds(900),
        std::string trustedExecutable = "/usr/bin/ffmpeg");

    MediaDecodeResult decode(
        int recordingDirectoryFd,
        const std::vector<std::string>& orderedTsSegments) override;

private:
    std::chrono::seconds deadline_;
    std::string executable_;
};

} // namespace vdrsuite::mediatools
