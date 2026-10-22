#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <vector>
#include <string>

namespace vdrsuite::mediatools {

// Standalone, backend-local read-only MPEG-TS structural check.
struct MediaDecodeResult {
    // 'ok' means FFmpeg decoded every supplied segment without a reported
    // error. 'errors' means FFmpeg reported a decode failure; 'incomplete'
    // means timeout, unavailable decoder or source changed during inspection.
    std::string status = "incomplete";
    int exitCode = -1;
    bool timedOut = false;
    std::uint64_t streamedBytes = 0;
    std::string diagnostic; // bounded stderr excerpt, not a log file
};

class MediaCheckDecoder {
public:
    virtual ~MediaCheckDecoder() = default;
    virtual MediaDecodeResult decode(
        int recordingDirectoryFd,
        const std::vector<std::string>& orderedTsSegments) = 0;
};


// Numbered TS segment diagnostics, without exposing recording paths.
struct MediaCheckSegmentReport {
    std::string name;
    std::uint64_t bytes = 0;
    std::uint64_t packets = 0;
    std::uint64_t syncErrors = 0;
    std::uint64_t transportErrors = 0;
    std::uint64_t adaptationErrors = 0;
    std::uint64_t trailingBytes = 0;
    std::uint64_t continuityWarnings = 0;
    std::uint64_t discontinuities = 0;
    std::string status() const;
};

// Sidecars are diagnosed read-only by the same recording descriptor used for
// TS segments. Missing optional files do not count as errors.
struct MediaSidecarReport {
    std::string name;
    std::string status;
    std::uint64_t bytes = 0;
};

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
    std::uint64_t sidecarErrors = 0;
    std::uint64_t sidecarWarnings = 0;
    std::vector<MediaSidecarReport> sidecarReports;
    std::vector<MediaCheckSegmentReport> segmentReports;
    std::optional<MediaDecodeResult> decode;
    std::string status() const;
    std::string json() const;
};

// recordingRelative is a relative .rec directory *within* videoRoot.
// A caller cannot select an arbitrary path outside the backend's root.
// No files, markers, logs, databases or metadata are created or altered.
MediaCheckReport checkRecordingTransportStream(
    const std::filesystem::path& videoRoot,
    const std::filesystem::path& recordingRelative);

// Same read-only TS checks, followed by a single sequential decoding pass.
// Decoder receives only a vetted directory descriptor and segment names;
// it must never write a file, alter the recording or invoke a shell.
MediaCheckReport checkRecordingWithDecoder(
    const std::filesystem::path& videoRoot,
    const std::filesystem::path& recordingRelative,
    MediaCheckDecoder& decoder);

} // namespace vdrsuite::mediatools
