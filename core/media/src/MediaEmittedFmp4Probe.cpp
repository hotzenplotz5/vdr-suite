#include "MediaEmittedFmp4Probe.h"

#include "MediaProcessRunner.h"

#include <cerrno>
#include <chrono>
#include <climits>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <filesystem>
#include <map>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <utility>
#include <vector>

namespace
{

constexpr std::size_t MaximumInitBytes = 256U * 1024U;
constexpr std::size_t MaximumProbeOutputBytes = 8192U;
constexpr unsigned MaximumProbeCpuSeconds = 2U;
constexpr std::size_t MaximumProbeAddressSpaceBytes = 512U * 1024U * 1024U;

MediaEmittedFmp4Result rejected(const char* reason)
{
    MediaEmittedFmp4Result result;
    result.reasonCode = reason;
    return result;
}

MediaCodec videoCodec(const std::string& codec)
{
    if (codec == "h264") return MediaCodec::H264;
    if (codec == "hevc" || codec == "h265") return MediaCodec::H265;
    if (codec == "mpeg2video") return MediaCodec::Mpeg2Video;
    return MediaCodec::Unknown;
}

MediaCodec audioCodec(const std::string& codec)
{
    if (codec == "aac") return MediaCodec::Aac;
    if (codec == "ac3") return MediaCodec::Ac3;
    if (codec == "eac3") return MediaCodec::Eac3;
    if (codec == "mp2" || codec == "mp3") return MediaCodec::MpegAudio;
    return MediaCodec::Unknown;
}

bool integer(const std::string& text, int& result)
{
    if (text.empty() || text.size() > 6U) return false;
    int value = 0;
    for (unsigned char character : text) {
        if (character < '0' || character > '9') return false;
        if (value > (INT_MAX - (character - '0')) / 10) return false;
        value = value * 10 + (character - '0');
    }
    if (value <= 0 || value > 16384) return false;
    result = value;
    return true;
}

bool compactFields(const std::string& line, std::map<std::string, std::string>& fields)
{
    if (line.empty() || line.size() > 1024U) return false;
    std::size_t pos = 0;
    while (pos < line.size()) {
        const std::size_t end = line.find('|', pos);
        const std::string field = line.substr(
            pos, end == std::string::npos ? end : end - pos);
        const std::size_t equals = field.find('=');
        if (equals == std::string::npos || equals == 0 ||
            equals > 32U || field.size() - equals > 128U)
            return false;
        const std::string key = field.substr(0, equals);
        const std::string value = field.substr(equals + 1);
        if (!fields.emplace(key, value).second) return false;
        for (unsigned char c : key) {
            if (!((c >= 'a' && c <= 'z') || c == '_')) return false;
        }
        for (unsigned char c : value) {
            if (c < 32 || c > 126) return false;
        }
        if (end == std::string::npos) break;
        pos = end + 1;
    }
    return true;
}

bool exactHeaderCodecTag(
    const std::string& codecType,
    const std::string& codecName,
    const std::string& codecTag)
{
    if (codecType == "video") {
        if (codecName == "h264")
            return codecTag == "avc1" || codecTag == "avc3";
        if (codecName == "hevc" || codecName == "h265")
            return codecTag == "hvc1" || codecTag == "hev1";
        if (codecName == "mpeg2video") return codecTag == "mp2v";
    }
    if (codecType == "audio") {
        if (codecName == "aac") return codecTag == "mp4a";
        if (codecName == "ac3") return codecTag == "ac-3";
        if (codecName == "eac3") return codecTag == "ec-3";
    }
    return false;
}

bool readAll(int fd, std::string& out, const std::size_t limit)
{
    char data[4096];
    while (out.size() <= limit) {
        const ssize_t count = ::read(fd, data, sizeof(data));
        if (count == 0) return !out.empty();
        if (count < 0) {
            if (errno == EINTR) continue;
            return false;
        }
        if (out.size() + static_cast<std::size_t>(count) > limit) return false;
        out.append(data, static_cast<std::size_t>(count));
    }
    return false;
}

bool writeAll(int fd, const std::string& bytes)
{
    std::size_t offset = 0;
    while (offset < bytes.size()) {
        const ssize_t count = ::write(
            fd, bytes.data() + offset, bytes.size() - offset);
        if (count < 0) {
            if (errno == EINTR) continue;
            return false;
        }
        if (count == 0) return false;
        offset += static_cast<std::size_t>(count);
    }
    return true;
}

bool sameFileState(const struct stat& a, const struct stat& b)
{
    return a.st_dev == b.st_dev && a.st_ino == b.st_ino &&
        a.st_size == b.st_size && a.st_mtim.tv_sec == b.st_mtim.tv_sec &&
        a.st_mtim.tv_nsec == b.st_mtim.tv_nsec;
}

} // namespace

std::vector<std::string> MediaEmittedFmp4Probe::commandPlan() const
{
    return {
        "/usr/bin/ffprobe",
        "-v", "error",
        "-protocol_whitelist", "file",
        "-probesize", "262144",
        "-analyzeduration", "500000",
        "-show_entries",
        "stream=codec_type,codec_name,codec_tag_string,width,height,channels",
        "-of", "compact=p=0:nk=0",
        "-i", "init.mp4"
    };
}

MediaEmittedFmp4Result MediaEmittedFmp4Probe::parse(
    const std::string& output,
    const MediaPlannedOutput& plan) const
{
    if (!plan.valid || plan.container != MediaContainer::Fmp4 ||
        output.empty() || output.size() > MaximumProbeOutputBytes)
        return rejected("invalid_emitted_probe_input");

    MediaEmittedFmp4Result result;
    int videos = 0;
    int audios = 0;
    std::size_t begin = 0;
    while (begin < output.size()) {
        const std::size_t end = output.find('\n', begin);
        std::string line = output.substr(
            begin, end == std::string::npos ? end : end - begin);
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty()) return rejected("malformed_emitted_stream_entry");
        std::map<std::string, std::string> fields;
        if (!compactFields(line, fields))
            return rejected("malformed_emitted_stream_entry");
        const std::string type = fields["codec_type"];
        const std::string codecName = fields["codec_name"];
        const std::string tag = fields["codec_tag_string"];
        if (!exactHeaderCodecTag(type, codecName, tag))
            return rejected("unsupported_emitted_stream_entry");

        if (type == "video") {
            if (++videos != 1)
                return rejected("duplicate_emitted_video_stream");
            result.videoCodec = videoCodec(codecName);
            if (!integer(fields["width"], result.videoWidth) ||
                !integer(fields["height"], result.videoHeight))
                return rejected("invalid_emitted_video_dimensions");
        } else if (type == "audio") {
            if (++audios != 1)
                return rejected("duplicate_emitted_audio_stream");
            result.audioCodec = audioCodec(codecName);
            if (!integer(fields["channels"], result.audioChannels) ||
                result.audioChannels > 32)
                return rejected("invalid_emitted_audio_channels");
        } else {
            return rejected("unexpected_emitted_stream_type");
        }
        begin = end == std::string::npos ? output.size() : end + 1U;
    }

    if ((plan.videoAction == MediaTrackAction::Omit) != (videos == 0) ||
        (plan.audioAction == MediaTrackAction::Omit) != (audios == 0))
        return rejected("missing_or_extra_emitted_stream");

    result.inspected = true;
    result.matchesPlannedOutput =
        result.videoCodec == plan.targetVideoCodec &&
        result.audioCodec == plan.targetAudioCodec &&
        result.videoWidth == plan.targetVideoWidth &&
        result.videoHeight == plan.targetVideoHeight &&
        (plan.targetAudioChannels == 0 ||
         result.audioChannels == plan.targetAudioChannels);
    result.reasonCode = result.matchesPlannedOutput
        ? "emitted_init_codec_matches_plan"
        : "emitted_init_codec_mismatch";
    return result;
}

MediaEmittedFmp4Result MediaEmittedFmp4Probe::inspectOwnedWorkspace(
    const std::string& workspace,
    const MediaPlannedOutput& plan) const
{
    if (!plan.valid || plan.container != MediaContainer::Fmp4 ||
        workspace.empty() || workspace.front() != '/')
        return rejected("invalid_emitted_workspace_request");

    // The caller must already own an authorized active MediaSession. Never
    // accept this method's workspace path from a client HTTP request.
    const int directory = ::open(
        workspace.c_str(), O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
    if (directory < 0) return rejected("emitted_workspace_unavailable");

    struct stat directoryState{};
    if (::fstat(directory, &directoryState) != 0 ||
        !S_ISDIR(directoryState.st_mode) ||
        directoryState.st_uid != ::geteuid() ||
        (directoryState.st_mode & 0077) != 0) {
        ::close(directory);
        return rejected("emitted_workspace_not_private");
    }

    const int init = ::openat(directory, "init.mp4",
                             O_RDONLY | O_NONBLOCK | O_NOFOLLOW | O_CLOEXEC);
    ::close(directory);
    if (init < 0) return rejected("emitted_init_unavailable");
    struct stat before{};
    if (::fstat(init, &before) != 0 || !S_ISREG(before.st_mode) ||
        before.st_uid != ::geteuid() ||
        before.st_nlink != 1 ||
        before.st_size <= 0 ||
        static_cast<unsigned long long>(before.st_size) > MaximumInitBytes) {
        ::close(init);
        return rejected("emitted_init_not_bounded_regular_file");
    }

    std::string contents;
    const bool complete = readAll(init, contents, MaximumInitBytes);
    struct stat after{};
    const bool unchanged = ::fstat(init, &after) == 0 &&
        sameFileState(before, after) &&
        contents.size() == static_cast<std::size_t>(before.st_size);
    ::close(init);
    if (!complete || !unchanged)
        return rejected("emitted_init_changed_or_unreadable");

    // Secure, bounded private snapshot: ffprobe never follows an active
    // MediaSession path, producer symlink, network URL or user-provided URL.
    char pattern[] = "/tmp/vdr-suite-fmp4-evidence-XXXXXX";
    char* directoryPath = ::mkdtemp(pattern);
    if (!directoryPath) return rejected("emitted_probe_snapshot_unavailable");
    const std::string snapshotDirectory(directoryPath);
    const std::filesystem::path target =
        std::filesystem::path(snapshotDirectory) / "init.mp4";
    const int snapshot = ::open(
        target.c_str(), O_WRONLY | O_CREAT | O_EXCL | O_NOFOLLOW | O_CLOEXEC, 0600);
    const bool saved = snapshot >= 0 && writeAll(snapshot, contents) &&
        ::fsync(snapshot) == 0;
    if (snapshot >= 0) ::close(snapshot);
    if (!saved) {
        std::error_code ignored;
        std::filesystem::remove_all(snapshotDirectory, ignored);
        return rejected("emitted_probe_snapshot_failed");
    }

    const MediaProcessCaptureResult process = MediaProcessRunner().runAndCapture(
        commandPlan(), snapshotDirectory, std::chrono::milliseconds(3000),
        MaximumProbeOutputBytes,
        {MaximumProbeAddressSpaceBytes, MaximumProbeCpuSeconds});
    std::error_code ignored;
    std::filesystem::remove_all(snapshotDirectory, ignored);
    if (!process.success) {
        MediaEmittedFmp4Result result =
            rejected("emitted_probe_process_failed");
        // Safe bounded diagnostic: no private path, URL or secret output.
        result.reasonCode += ":" + std::to_string(process.exitCode);
        result.reasonCode += process.timedOut ? ":timeout" : ":no-timeout";
        result.reasonCode += process.outputLimitExceeded ? ":output-limit" : ":within-output-limit";
        if (!process.reasonCode.empty() && process.reasonCode.size() <= 64U)
            result.reasonCode += ":" + process.reasonCode;
        return result;
    }
    return parse(process.output, plan);
}
