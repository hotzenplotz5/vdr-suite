#pragma once

#include <cstdint>
#include <filesystem>
#include <string>

namespace vdrsuite::mediatools {

// Standalone, backend-local read-only MPEG-TS structural check.
// This is NOT a public HTTP path or a recording decoder / repair service.
// The owning backend must supply the configured video root, not the actor.
struct MediaCheckReport {
    std::string recording;
    std::uint64_t segments = 0;
    std::uint64_t bytes = 0;
    std::uint64_t packets = 0;
    std::uint64_t syncErrors = 0;
    std::uint64_t transportErrors = 0;
    std::uint64_t adaptationErrors = 0;
    std::uint64_t trailingBytes = 0;
    std::uint64_t continuityWarnings = 0;
    std::uint64_t discontinuities = 0;
    bool indexPresent = false;
    std::string status() const;
    std::string json() const;
};

// recordingRelative is a relative .rec directory *within* videoRoot.
// A caller cannot select an arbitrary path outside the backend's root.
// No files, markers, logs, databases or metadata are created or altered.
MediaCheckReport checkRecordingTransportStream(
    const std::filesystem::path& videoRoot,
    const std::filesystem::path& recordingRelative);

} // namespace vdrsuite::mediatools
