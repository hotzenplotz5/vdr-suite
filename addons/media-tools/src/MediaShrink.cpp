#include "MediaShrink.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cerrno>
#include <cmath>
#include <cstdlib>
#include <cstdint>
#include <cstdio>
#include <ctime>
#include <fcntl.h>
#include <fstream>
#include <iomanip>
#include <iterator>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <linux/fs.h>
#include <utility>

namespace vdrsuite::mediatools {
namespace {

constexpr std::uint64_t MiB = UINT64_C(1024) * 1024;
constexpr std::uint64_t GiB = MiB * 1024;

void demand(bool valid, const char* problem) {
    if (!valid) throw std::runtime_error(problem);
}

struct Descriptor {
    int fd = -1;
    explicit Descriptor(int value = -1) : fd(value) {}
    ~Descriptor() { if (fd >= 0) ::close(fd); }
    Descriptor(const Descriptor&) = delete;
    Descriptor& operator=(const Descriptor&) = delete;
};

struct CleanStage {
    fs::path path;
    ~CleanStage() {
        if (!path.empty()) {
            std::error_code error;
            fs::remove_all(path, error);
        }
    }
};

bool below(const fs::path& root, const fs::path& file) {
    auto a = root.begin(), b = file.begin();
    for (; a != root.end() && b != file.end(); ++a, ++b)
        if (*a != *b) return false;
    return a == root.end() && b != file.end();
}

void prohibitSymlinks(const fs::path& path) {
    fs::path prefix;
    for (const auto& part : path) {
        prefix /= part;
        demand(!fs::is_symlink(fs::symlink_status(prefix)),
               "symlinked shrink path");
    }
}

void checkRoot(const fs::path& root) {
    demand(root.is_absolute() && root.lexically_normal() == root,
           "shrink video root must be normalized and absolute");
    prohibitSymlinks(root);
    demand(fs::is_directory(root), "shrink video root missing");
}

ShrinkSegment identity(const fs::path& file) {
    struct stat s{};
    demand(::lstat(file.c_str(), &s) == 0 && S_ISREG(s.st_mode) &&
           s.st_size > 0, "shrink segment is missing or not a regular file");
    ShrinkSegment result;
    result.filename = file.filename().string();
    result.device = static_cast<std::uint64_t>(s.st_dev);
    result.inode = static_cast<std::uint64_t>(s.st_ino);
    result.bytes = static_cast<std::uint64_t>(s.st_size);
    result.seconds = s.st_mtim.tv_sec;
    result.nanoseconds = s.st_mtim.tv_nsec;
    return result;
}

bool same(const ShrinkSegment& a, const ShrinkSegment& b) {
    return a.filename == b.filename && a.device == b.device &&
        a.inode == b.inode && a.bytes == b.bytes &&
        a.seconds == b.seconds && a.nanoseconds == b.nanoseconds;
}

std::string segmentName(unsigned index) {
    char number[32]{};
    demand(std::snprintf(number, sizeof number, "%05u.ts", index) > 0,
           "invalid segment number");
    return number;
}

bool nonempty(const fs::path& file) {
    std::error_code ec;
    return fs::is_regular_file(fs::symlink_status(file, ec)) &&
           fs::file_size(file, ec) > 0 && !ec;
}

void invoke(ProcessRunner& runner, const std::vector<std::string>& args) {
    demand(runner.run(args, false).exitCode == 0,
           "shrink external command failed");
}

std::string probeCodec(ProcessRunner& runner, const fs::path& input) {
    const auto result = runner.run({
        "/usr/bin/ffprobe", "-v", "error", "-select_streams", "v:0",
        "-show_entries", "stream=codec_name", "-of",
        "default=nw=1:nk=1", input.string()
    }, true);
    demand(result.exitCode == 0,
           "shrink codec probe failed");
    return parseProbedVideoCodec(result.output);
}

bool canCopyAudio(ProcessRunner& runner, const fs::path& input) {
    const auto probe = runner.run({
        "/usr/bin/ffprobe", "-v", "error", "-select_streams", "a",
        "-show_entries", "stream=codec_name", "-of",
        "default=nw=1:nk=1", input.string()
    }, true);
    demand(probe.exitCode == 0 && probe.output.size() <= 2048,
           "cannot inspect shrink source audio");
    bool compatible = true;
    std::istringstream streams(probe.output);
    std::string name;
    while (std::getline(streams, name)) {
        if (!name.empty() && name.back() == '\r') name.pop_back();
        if (!name.empty() && name != "aac" && name != "ac3" &&
            name != "eac3" && name != "mp2")
            compatible = false;
    }
    return compatible;
}

double probeDuration(ProcessRunner& runner, const fs::path& input) {
    const auto result = runner.run({
        "/usr/bin/ffprobe", "-v", "error", "-show_entries",
        "format=duration", "-of", "default=nw=1:nk=1", input.string()
    }, true);
    demand(result.exitCode == 0 && result.output.size() < 64,
           "shrink duration probe failed");
    std::size_t used = 0;
    double duration = 0.0;
    try { duration = std::stod(result.output, &used); }
    catch (...) { throw std::runtime_error("invalid shrink duration"); }
    demand(used > 0 && result.output.find_first_not_of(" \t\r\n", used) ==
           std::string::npos && std::isfinite(duration) && duration > 0,
           "invalid shrink duration");
    return duration;
}

unsigned probeHeight(ProcessRunner& runner, const fs::path& input) {
    const auto result = runner.run({
        "/usr/bin/ffprobe", "-v", "error", "-select_streams", "v:0",
        "-show_entries", "stream=height", "-of", "default=nw=1:nk=1",
        input.string()
    }, true);
    demand(result.exitCode == 0 && !result.output.empty() &&
           result.output.size() <= 1024, "shrink height probe failed");
    std::istringstream heightValues(result.output);
    std::string token;
    unsigned height = 0;
    unsigned entries = 0;
    while (std::getline(heightValues, token)) {
        if (!token.empty() && token.back() == '\r') token.pop_back();
        if (token.empty()) continue;
        demand(token.size() <= 5 && std::all_of(token.begin(), token.end(),
            [](unsigned char ch) { return std::isdigit(ch); }),
            "invalid source height");
        unsigned observed = 0;
        try { observed = static_cast<unsigned>(std::stoul(token)); }
        catch (...) { throw std::runtime_error("invalid source height"); }
        demand(observed > 0 && observed <= 16384,
               "invalid source height");
        if (height != 0)
            demand(height == observed, "inconsistent source video heights");
        else height = observed;
        demand(++entries <= 64, "too many height entries");
    }
    demand(height != 0, "missing source height");
    return height;
}

void copySegments(const ShrinkPlan& plan, const fs::path& file) {
    Descriptor output(::open(file.c_str(), O_CREAT | O_EXCL | O_WRONLY |
                             O_NOFOLLOW | O_CLOEXEC, 0600));
    demand(output.fd >= 0, "cannot create private shrink snapshot");
    std::array<char, 65536> buffer{};
    std::uint64_t writtenTotal = 0;
    const fs::path recording = plan.videoRoot / plan.relativeRecording;
    for (const auto& expected : plan.segments) {
        const fs::path inputPath = recording / expected.filename;
        Descriptor input(::open(inputPath.c_str(),
                                O_RDONLY | O_NOFOLLOW | O_CLOEXEC));
        demand(input.fd >= 0 && same(identity(inputPath), expected),
               "shrink source changed before opening");
        struct stat opened{};
        demand(::fstat(input.fd, &opened) == 0 &&
               static_cast<std::uint64_t>(opened.st_ino) == expected.inode &&
               static_cast<std::uint64_t>(opened.st_dev) == expected.device &&
               static_cast<std::uint64_t>(opened.st_size) == expected.bytes,
               "shrink source inode changed");
        std::uint64_t copied = 0;
        while (copied < expected.bytes) {
            const auto wanted = static_cast<std::size_t>(
                std::min<std::uint64_t>(buffer.size(), expected.bytes - copied));
            const ssize_t got = ::read(input.fd, buffer.data(), wanted);
            if (got < 0 && errno == EINTR) continue;
            demand(got > 0, "shrink snapshot read failed");
            std::size_t sent = 0;
            while (sent < static_cast<std::size_t>(got)) {
                const ssize_t put = ::write(output.fd, buffer.data() + sent,
                                            static_cast<std::size_t>(got) - sent);
                if (put < 0 && errno == EINTR) continue;
                demand(put > 0, "shrink snapshot write failed");
                sent += static_cast<std::size_t>(put);
            }
            copied += static_cast<std::uint64_t>(got);
        }
        demand(same(identity(inputPath), expected),
               "shrink source changed during snapshot");
        writtenTotal += copied;
    }
    demand(writtenTotal == plan.totalBytes && ::fsync(output.fd) == 0,
           "shrink snapshot is incomplete");
}

void copySidecarFile(const fs::path& original,
                     const fs::path& destination, std::int64_t maxBytes) {
    Descriptor source(::open(original.c_str(), O_RDONLY | O_NOFOLLOW | O_CLOEXEC));
    struct stat st{};
    demand(source.fd >= 0 && ::fstat(source.fd, &st) == 0 &&
           S_ISREG(st.st_mode) && st.st_size > 0 && st.st_size <= maxBytes,
           "native VDR info sidecar missing or unsafe");
    Descriptor output(::open(destination.c_str(), O_WRONLY | O_EXCL |
                             O_CREAT | O_NOFOLLOW | O_CLOEXEC, 0600));
    demand(output.fd >= 0, "cannot prepare shrink info sidecar");
    std::array<char, 4096> buffer{};
    std::uint64_t copied = 0;
    while (copied < static_cast<std::uint64_t>(st.st_size)) {
        const ssize_t count = ::read(source.fd, buffer.data(), buffer.size());
        if (count < 0 && errno == EINTR) continue;
        demand(count > 0 && copied + static_cast<std::uint64_t>(count) <=
               static_cast<std::uint64_t>(st.st_size), "info sidecar changed");
        std::size_t offset = 0;
        while (offset < static_cast<std::size_t>(count)) {
            const ssize_t written = ::write(output.fd, buffer.data() + offset,
                static_cast<std::size_t>(count) - offset);
            if (written < 0 && errno == EINTR) continue;
            demand(written > 0, "info sidecar copy failed");
            offset += static_cast<std::size_t>(written);
        }
        copied += static_cast<std::uint64_t>(count);
    }
    struct stat after{};
    demand(::fstat(source.fd, &after) == 0 &&
           after.st_ino == st.st_ino && after.st_size == st.st_size &&
           after.st_mtim.tv_sec == st.st_mtim.tv_sec &&
           after.st_mtim.tv_nsec == st.st_mtim.tv_nsec,
           "info sidecar changed during shrink");
}

void copyInfo(const ShrinkPlan& plan, const fs::path& destination) {
    copySidecarFile(plan.videoRoot / plan.relativeRecording / "info",
                    destination, 1024 * 1024);
}

std::vector<std::string> subtitleNames(const ShrinkPlan& plan) {
    std::vector<std::string> found;
    for (const auto& item : fs::directory_iterator(
            plan.videoRoot / plan.relativeRecording)) {
        if (item.path().extension() != ".srt") continue;
        demand(found.size() < 16, "too many recording subtitle sidecars");
        demand(fs::is_regular_file(item.symlink_status()),
               "unsafe recording subtitle sidecar");
        found.push_back(item.path().filename().string());
    }
    std::sort(found.begin(), found.end());
    return found;
}

void copySubtitles(const ShrinkPlan& plan, const fs::path& destination) {
    std::uint64_t total = 0;
    for (const auto& name : subtitleNames(plan)) {
        const auto source = plan.videoRoot / plan.relativeRecording / name;
        struct stat file{};
        demand(::lstat(source.c_str(), &file) == 0 &&
               S_ISREG(file.st_mode) && file.st_size > 0 &&
               file.st_size <= 8 * 1024 * 1024 &&
               static_cast<std::uint64_t>(file.st_size) <= 16 * MiB - total,
               "total shrink subtitle size exceeds staging budget");
        total += static_cast<std::uint64_t>(file.st_size);
        copySidecarFile(source, destination / name, 8 * 1024 * 1024);
    }
}

void requireMatchingSidecar(const fs::path& original, const fs::path& staged) {
    Descriptor left(::open(original.c_str(), O_RDONLY | O_NOFOLLOW | O_CLOEXEC));
    Descriptor right(::open(staged.c_str(), O_RDONLY | O_NOFOLLOW | O_CLOEXEC));
    struct stat a{}, b{}, afterA{}, afterB{};
    demand(left.fd >= 0 && right.fd >= 0 &&
           ::fstat(left.fd, &a) == 0 && ::fstat(right.fd, &b) == 0 &&
           S_ISREG(a.st_mode) && S_ISREG(b.st_mode) &&
           a.st_size > 0 && a.st_size == b.st_size &&
           a.st_size <= 8 * 1024 * 1024,
           "shrink candidate sidecar is missing or changed");
    std::array<char, 4096> source{}, candidate{};
    std::uint64_t offset = 0;
    while (offset < static_cast<std::uint64_t>(a.st_size)) {
        const auto size = static_cast<std::size_t>(
            std::min<std::uint64_t>(source.size(),
                static_cast<std::uint64_t>(a.st_size) - offset));
        const ssize_t n = ::pread(left.fd, source.data(), size,
                                  static_cast<off_t>(offset));
        const ssize_t m = ::pread(right.fd, candidate.data(), size,
                                  static_cast<off_t>(offset));
        demand(n == static_cast<ssize_t>(size) &&
               m == static_cast<ssize_t>(size) &&
               std::equal(source.begin(), source.begin() + size,
                          candidate.begin()),
               "shrink candidate sidecar differs from original");
        offset += size;
    }
    demand(::fstat(left.fd, &afterA) == 0 &&
           ::fstat(right.fd, &afterB) == 0 &&
           a.st_dev == afterA.st_dev && a.st_ino == afterA.st_ino &&
           a.st_size == afterA.st_size &&
           a.st_mtim.tv_sec == afterA.st_mtim.tv_sec &&
           a.st_mtim.tv_nsec == afterA.st_mtim.tv_nsec &&
           b.st_dev == afterB.st_dev && b.st_ino == afterB.st_ino &&
           b.st_size == afterB.st_size &&
           b.st_mtim.tv_sec == afterB.st_mtim.tv_sec &&
           b.st_mtim.tv_nsec == afterB.st_mtim.tv_nsec,
           "shrink sidecar changed during publication verification");
}

std::string signature(const ShrinkPlan& plan) {
    std::ostringstream raw;
    raw << plan.videoRoot << '\n' << plan.relativeRecording << '\n'
        << plan.totalBytes << '\n';
    for (const auto& segment : plan.segments)
        raw << segment.filename << ':' << segment.device << ':'
            << segment.inode << ':' << segment.bytes << ':'
            << segment.seconds << ':' << segment.nanoseconds << '\n';
    for (const auto& sidecar : plan.sidecars)
        raw << "sidecar:" << sidecar.filename << ':' << sidecar.device << ':'
            << sidecar.inode << ':' << sidecar.bytes << ':'
            << sidecar.seconds << ':' << sidecar.nanoseconds << '\n';
    std::uint64_t hash = UINT64_C(14695981039346656037);
    for (unsigned char ch : raw.str()) {
        hash ^= ch;
        hash *= UINT64_C(1099511628211);
    }
    std::ostringstream digest;
    digest << "VDR-SUITE-SHRINK/1\n" << std::hex
           << std::setfill('0') << std::setw(16) << hash << '\n';
    // This identifies a source revision; it is NOT a credential or MAC.
    return digest.str();
}

std::vector<std::string> ffmpegArguments(
    const ShrinkPlan& plan, const fs::path& input,
    const fs::path& output, std::uint64_t limit,
    ShrinkEncoder requested, bool scaled, bool copyAudio)
{
    std::vector<std::string> args = {
        "/usr/bin/ffmpeg", "-nostdin", "-hide_banner", "-v", "error",
        "-xerror"
    };
    if (requested == ShrinkEncoder::vaapi)
        args.insert(args.end(), {"-vaapi_device", "/dev/dri/renderD128"});
    args.insert(args.end(), {"-i", input.string(), "-map", "0:v:0",
                             "-map", "0:a?", "-sn", "-dn"});
    if (scaled) {
        const std::string scale = "scale=-2:" +
            std::to_string(plan.options.maxHeight);
        args.insert(args.end(), {"-vf", requested == ShrinkEncoder::vaapi
            ? scale + ",format=nv12,hwupload" : scale});
    } else if (requested == ShrinkEncoder::vaapi) {
        args.insert(args.end(), {"-vf", "format=nv12,hwupload"});
    }
    switch (requested) {
    case ShrinkEncoder::vaapi:
        args.insert(args.end(), {"-c:v", "hevc_vaapi", "-qp", "27"}); break;
    case ShrinkEncoder::qsv:
        args.insert(args.end(), {"-c:v", "hevc_qsv", "-global_quality", "27"}); break;
    case ShrinkEncoder::nvenc:
        args.insert(args.end(), {"-c:v", "hevc_nvenc", "-cq", "27"}); break;
    case ShrinkEncoder::software:
        args.insert(args.end(), {"-c:v", "libx265", "-preset", "medium",
                                 "-crf", "27", "-threads", "2"}); break;
    }
    // VDR's HEVC indexer requires independently decodable frames at
    // predictable intervals. Default x265 GOPs may contain a single IDR
    // over short captures, leaving VDR unable to build a valid index.
    args.insert(args.end(), {"-g", "50"});
    if (plan.options.nightMode)
        args.insert(args.end(), {"-c:a", "aac", "-b:a", "192k",
                                 "-ac", "2", "-af", "loudnorm"});
    else if (copyAudio)
        args.insert(args.end(), {"-c:a", "copy"});
    else
        args.insert(args.end(), {"-c:a", "aac", "-b:a", "192k"});
    args.insert(args.end(), {"-fs", std::to_string(limit),
                             "-f", "mpegts", "-y", output.string()});
    return args;
}

} // namespace

std::string shrinkSourceFingerprint(const ShrinkPlan& plan)
{
    const std::string value = signature(plan);
    const std::string prefix = "VDR-SUITE-SHRINK/1\n";
    demand(value.compare(0, prefix.size(), prefix) == 0 &&
           value.size() == prefix.size() + 17 &&
           value.back() == '\n',
           "invalid shrink source snapshot fingerprint");
    return "shrink_v1_" + value.substr(prefix.size(), 16);
}

ShrinkPlan inspectShrink(const fs::path& videoRoot,
                         const fs::path& relative, const ShrinkOptions& options)
{
    checkRoot(videoRoot);
    demand(!relative.empty() && relative.is_relative() &&
           relative.lexically_normal() == relative &&
           relative.extension() == ".rec" &&
           (options.maxHeight == 0 ||
            (options.maxHeight >= 480 && options.maxHeight <= 2160)),
           "invalid shrink recording or options");
    for (const auto& p : relative)
        demand(p != "." && p != "..", "invalid shrink path component");
    const fs::path recording = videoRoot / relative;
    demand(below(videoRoot, recording), "shrink recording outside video root");
    prohibitSymlinks(recording);
    demand(fs::is_directory(recording), "shrink recording not found");
    ShrinkPlan plan;
    plan.videoRoot = videoRoot;
    plan.relativeRecording = relative;
    plan.options = options;
    unsigned numbered = 0;
    for (const auto& item : fs::directory_iterator(recording)) {
        const std::string name = item.path().filename().string();
        if (name.size() == 8 && name.substr(5) == ".ts" &&
            std::all_of(name.begin(), name.begin() + 5,
                [](unsigned char ch) { return ch >= '0' && ch <= '9'; })) {
            demand(++numbered <= 256, "too many shrink TS segments");
            const auto part = identity(item.path());
            demand(part.bytes <= UINT64_MAX - plan.totalBytes,
                   "shrink source size overflow");
            plan.totalBytes += part.bytes;
            plan.segments.push_back(part);
        }
    }
    demand(numbered > 0, "no MPEG-TS segments in recording");
    std::sort(plan.segments.begin(), plan.segments.end(),
              [](const ShrinkSegment& a, const ShrinkSegment& b) {
                  return a.filename < b.filename;
              });
    for (std::size_t i = 0; i < plan.segments.size(); ++i)
        demand(plan.segments[i].filename == segmentName(static_cast<unsigned>(i + 1)),
               "noncontiguous VDR TS segments");
    const auto index = nonempty(recording / "index")
        ? recording / "index" : recording / "index.vdr";
    demand(nonempty(index), "original VDR index missing");
    plan.sidecars.push_back(identity(index));
    plan.sidecars.push_back(identity(recording / "info"));
    const auto subtitles = subtitleNames(plan);
    for (const auto& name : subtitles)
        plan.sidecars.push_back(identity(recording / name));
    return plan;
}

void requireShrinkCapacity(const ShrinkPlan& plan)
{
    struct statvfs fsInfo{};
    demand(::statvfs(plan.videoRoot.c_str(), &fsInfo) == 0 &&
           fsInfo.f_frsize > 0, "cannot inspect shrink destination capacity");
    demand((fsInfo.f_flag & ST_RDONLY) == 0 &&
           ::access(plan.videoRoot.c_str(), W_OK | X_OK) == 0,
           "shrink destination is not writable by the media worker");
    const std::uint64_t block = fsInfo.f_frsize;
    demand(fsInfo.f_blocks <= UINT64_MAX / block &&
           fsInfo.f_bavail <= UINT64_MAX / block,
           "shrink capacity overflow");
    const auto total = static_cast<std::uint64_t>(fsInfo.f_blocks) * block;
    const auto available = static_cast<std::uint64_t>(fsInfo.f_bavail) * block;
    const auto reserve = std::max(4 * GiB, total / 100 * 15);
    demand(plan.totalBytes <= (UINT64_MAX - 64 * MiB) / 3,
           "shrink source exceeds bounded size");
    const auto required = plan.totalBytes * 3 + 64 * MiB;
    demand(available >= reserve && available - reserve >= required,
           "insufficient shrink staging capacity and reserve");
}

fs::path stageShrink(const ShrinkPlan& expected, ProcessRunner& runner,
                     const std::function<void(const ShrinkPlan&)>& preflight)
{
    const auto current = inspectShrink(expected.videoRoot,
        expected.relativeRecording, expected.options);
    demand(signature(current) == signature(expected), "shrink plan became stale");
    preflight(current);
    std::string pattern =
        (current.videoRoot / ".vdr-suite-shrink-XXXXXX").string();
    std::vector<char> name(pattern.begin(), pattern.end());
    name.push_back('\0');
    char* made = ::mkdtemp(name.data());
    demand(made != nullptr, "cannot create shrink staging directory");
    CleanStage stage{fs::path(made)};
    demand(::chmod(stage.path.c_str(), 0700) == 0,
           "cannot protect shrink candidate");
    const fs::path candidate = stage.path /
        current.relativeRecording.filename();
    demand(candidate.extension() == ".rec" &&
           fs::create_directory(candidate),
           "cannot create staged VDR recording");
    demand(::chmod(candidate.c_str(), 0700) == 0,
           "cannot protect staged recording");
    const auto source = stage.path / "input.ts";
    const auto output = candidate / "00001.ts";
    copySegments(current, source);
    const auto codec = probeCodec(runner, source);
    demand(codec != "hevc", "recording already contains HEVC video");
    const double originalDuration = probeDuration(runner, source);
    const auto height = probeHeight(runner, source);
    const bool copyAudio = canCopyAudio(runner, source);
    const bool scaling = current.options.maxHeight != 0 &&
        height > current.options.maxHeight;
    demand(current.totalBytes <= (UINT64_MAX - 16 * MiB) / 2,
           "shrink output limit overflow");
    const auto limit = current.totalBytes * 2 + 16 * MiB;
    auto argv = ffmpegArguments(current, source, output, limit,
                                 current.options.encoder, scaling, copyAudio);
    auto conversion = runner.run(argv, false);
    if (conversion.exitCode != 0 &&
        current.options.encoder != ShrinkEncoder::software) {
        // Hardware failure may leave a partial TS. Only our private output
        // can be removed; software starts from the same immutable snapshot.
        std::error_code error;
        fs::remove(output, error);
        demand(!error, "cannot clear failed hardware candidate");
        argv = ffmpegArguments(current, source, output, limit,
                               ShrinkEncoder::software, scaling, copyAudio);
        conversion = runner.run(argv, false);
    }
    demand(conversion.exitCode == 0 && nonempty(output),
           "HEVC transcode failed");
    const auto generated = fs::file_size(output);
    demand(generated < limit && generated < current.totalBytes,
           "HEVC output is incomplete or does not save space");
    demand(probeCodec(runner, output) == "hevc", "HEVC codec verification failed");
    const double finalDuration = probeDuration(runner, output);
    demand(std::fabs(finalDuration - originalDuration) <=
           std::max(4.0, originalDuration * 0.02),
           "HEVC output duration differs from original");
    copyInfo(current, candidate / "info");
    copySubtitles(current, candidate);
    invoke(runner, {"/usr/bin/vdr", "--genindex=" + candidate.string()});
    demand(nonempty(candidate / "index"), "HEVC recording index missing");
    // Reinspect all TS/Index/Info/SRT identities after encoding. The
    // source recording may have been enriched while FFmpeg was running.
    const auto verified = inspectShrink(current.videoRoot,
        current.relativeRecording, current.options);
    demand(signature(verified) == signature(current),
           "source recording or sidecars changed during HEVC operation");
    std::ofstream review(candidate / ".shrink-review",
                         std::ios::binary | std::ios::trunc);
    demand(static_cast<bool>(review), "cannot create shrink review evidence");
    review << signature(current);
    review.flush();
    demand(static_cast<bool>(review), "cannot finish shrink review evidence");
    demand(fs::remove(source), "cannot remove private TS snapshot");
    stage.path.clear();
    return candidate;
}

namespace {
constexpr const char* kCoreReviewMarker = ".shrink-core-review";

bool safeReviewBinding(const ShrinkReviewBinding& e) {
    auto identifier = [](const std::string& value) {
        return !value.empty() && value.size() <= 160 &&
            std::all_of(value.begin(), value.end(), [](unsigned char c) {
                return (c >= 'a' && c <= 'z') ||
                       (c >= 'A' && c <= 'Z') ||
                       (c >= '0' && c <= '9') ||
                       c == '_' || c == '-' || c == '.' || c == ':';
            });
    };
    auto opaque = [](const std::string& value, const std::string& prefix) {
        return value.size() == prefix.size() + 32 &&
            value.compare(0, prefix.size(), prefix) == 0 &&
            std::all_of(value.begin() + prefix.size(), value.end(),
                [](unsigned char c) {
                    return (c >= '0' && c <= '9') ||
                           (c >= 'a' && c <= 'f');
                });
    };
    return opaque(e.reviewId, "shk_") &&
        identifier(e.stageOperationId) && identifier(e.actorId) &&
        identifier(e.backendId) && opaque(e.recordingId, "rec_") &&
        e.sourceFingerprint.size() == 26 &&
        e.sourceFingerprint.compare(0, 10, "shrink_v1_") == 0 &&
        std::all_of(e.sourceFingerprint.begin() + 10,
            e.sourceFingerprint.end(), [](unsigned char c) {
                return (c >= '0' && c <= '9') ||
                       (c >= 'a' && c <= 'f');
            }) &&
        e.backendGeneration > 0 &&
        e.backendGeneration <=
            static_cast<std::uint64_t>(
                std::numeric_limits<std::int64_t>::max());
}
std::string serializeBinding(const ShrinkReviewBinding& e) {
    return "VDR-SUITE-SHRINK-REVIEW/1\n" +
        e.reviewId + "\n" + e.stageOperationId + "\n" +
        e.actorId + "\n" + e.backendId + "\n" +
        e.recordingId + "\n" + e.sourceFingerprint + "\n" +
        std::to_string(e.backendGeneration) + "\n";
}
}

void writeShrinkReviewBinding(const fs::path& candidate,
                              const ShrinkReviewBinding& binding) {
    demand(safeReviewBinding(binding) &&
        candidate.is_absolute() && candidate.lexically_normal() == candidate,
        "invalid private shrink binding");
    prohibitSymlinks(candidate);
    struct stat folder{};
    demand(::lstat(candidate.c_str(), &folder) == 0 &&
           S_ISDIR(folder.st_mode) && folder.st_uid == ::geteuid() &&
           (folder.st_mode & 0077) == 0,
           "unprotected private HEVC binding destination");
    const std::string bytes = serializeBinding(binding);
    demand(bytes.size() <= 1024, "private HEVC binding too large");
    Descriptor fd(::open((candidate / kCoreReviewMarker).c_str(),
        O_WRONLY | O_CREAT | O_EXCL | O_NOFOLLOW | O_CLOEXEC, 0600));
    demand(fd.fd >= 0, "cannot create durable Core review marker");
    std::size_t position = 0;
    while (position < bytes.size()) {
        const auto n = ::write(fd.fd, bytes.data() + position,
                               bytes.size() - position);
        if (n < 0 && errno == EINTR) continue;
        demand(n > 0, "cannot persist Core review marker");
        position += static_cast<std::size_t>(n);
    }
    Descriptor directory(::open(candidate.c_str(),
        O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC));
    demand(::fsync(fd.fd) == 0 && directory.fd >= 0 &&
           ::fsync(directory.fd) == 0,
           "Core review marker not durable");
}

std::optional<ShrinkReviewBinding> readShrinkReviewBinding(
    const fs::path& candidate) {
    try {
        if (!candidate.is_absolute() ||
            candidate.lexically_normal() != candidate)
            return std::nullopt;
        prohibitSymlinks(candidate);
        struct stat folder{}, before{}, after{};
        if (::lstat(candidate.c_str(), &folder) != 0 ||
            !S_ISDIR(folder.st_mode) || (folder.st_mode & 0077) != 0)
            return std::nullopt;
        Descriptor fd(::open((candidate / kCoreReviewMarker).c_str(),
            O_RDONLY | O_NOFOLLOW | O_NONBLOCK | O_CLOEXEC));
        if (fd.fd < 0 || ::fstat(fd.fd, &before) != 0 ||
            !S_ISREG(before.st_mode) || before.st_uid != folder.st_uid ||
            (before.st_mode & 0077) != 0 ||
            before.st_size < 80 || before.st_size > 1024)
            return std::nullopt;
        std::string bytes(static_cast<std::size_t>(before.st_size), '\0');
        std::size_t offset = 0;
        while (offset < bytes.size()) {
            const auto n = ::read(fd.fd, &bytes[offset],
                                  bytes.size() - offset);
            if (n < 0 && errno == EINTR) continue;
            if (n <= 0) return std::nullopt;
            offset += static_cast<std::size_t>(n);
        }
        if (::fstat(fd.fd, &after) != 0 ||
            before.st_dev != after.st_dev || before.st_ino != after.st_ino ||
            before.st_size != after.st_size ||
            before.st_mtim.tv_sec != after.st_mtim.tv_sec ||
            before.st_mtim.tv_nsec != after.st_mtim.tv_nsec)
            return std::nullopt;
        std::istringstream input(bytes);
        std::string header, extra, generation;
        ShrinkReviewBinding binding;
        if (!std::getline(input, header) ||
            header != "VDR-SUITE-SHRINK-REVIEW/1" ||
            !std::getline(input, binding.reviewId) ||
            !std::getline(input, binding.stageOperationId) ||
            !std::getline(input, binding.actorId) ||
            !std::getline(input, binding.backendId) ||
            !std::getline(input, binding.recordingId) ||
            !std::getline(input, binding.sourceFingerprint) ||
            !std::getline(input, generation) ||
            std::getline(input, extra) ||
            generation.empty() || generation.size() > 19 ||
            std::any_of(generation.begin(), generation.end(),
                [](unsigned char c) {return c < '0' || c > '9';}))
            return std::nullopt;
        binding.backendGeneration = std::stoull(generation);
        return safeReviewBinding(binding)
            ? std::optional<ShrinkReviewBinding>(binding) : std::nullopt;
    } catch (...) { return std::nullopt; }
}

std::uint64_t verifyShrinkCandidate(const ShrinkPlan& plan,
                                    const fs::path& candidate,
                                    ProcessRunner& runner)
{
    const auto checked = inspectShrink(plan.videoRoot,
        plan.relativeRecording, plan.options);
    demand(signature(checked) == signature(plan), "original recording changed");
    const auto privateParent = candidate.parent_path();
    demand(candidate.is_absolute() &&
           candidate.lexically_normal() == candidate &&
           privateParent.parent_path() == checked.videoRoot &&
           privateParent.filename().string().rfind(".vdr-suite-shrink-", 0) == 0 &&
           candidate.filename() == checked.relativeRecording.filename() &&
           candidate.extension() == ".rec",
           "invalid shrink review location");
    prohibitSymlinks(candidate);
    struct stat parent{};
    demand(::lstat(privateParent.c_str(), &parent) == 0 &&
           S_ISDIR(parent.st_mode) && parent.st_uid == ::geteuid() &&
           (parent.st_mode & 0077) == 0,
           "unprotected shrink review parent");
    demand(fs::is_directory(candidate), "shrink review candidate missing");
    Descriptor evidence(::open((candidate / ".shrink-review").c_str(),
         O_RDONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK));
    struct stat evidenceStat{};
    demand(evidence.fd >= 0 && ::fstat(evidence.fd, &evidenceStat) == 0 &&
           S_ISREG(evidenceStat.st_mode) &&
           evidenceStat.st_size > 0 && evidenceStat.st_size <= 128,
           "shrink review evidence missing or unsafe");
    std::array<char, 128> bytes{};
    const ssize_t length = ::read(evidence.fd, bytes.data(), bytes.size());
    demand(length == evidenceStat.st_size,
           "shrink review evidence changed");
    const std::string saved(bytes.data(), static_cast<std::size_t>(length));
    demand(saved == signature(checked), "candidate belongs to a different recording");
    demand(nonempty(candidate / "00001.ts") &&
           nonempty(candidate / "index") && nonempty(candidate / "info"),
           "unverified HEVC candidate files");
    std::error_code sizeError;
    const auto candidateBytes = fs::file_size(candidate / "00001.ts", sizeError);
    demand(!sizeError && candidateBytes < checked.totalBytes,
           "HEVC candidate no longer saves space");
    demand(probeCodec(runner, candidate / "00001.ts") == "hevc",
           "published candidate is not HEVC");
    // Recheck duration during independent candidate verification.
    // For a single VDR TS segment, the original provides an exact
    // immutable duration reference. Combined TS was checked at staging.
    const double candidateDuration = probeDuration(
        runner, candidate / "00001.ts");
    if (checked.segments.size() == 1) {
        const double originalDuration = probeDuration(runner,
            checked.videoRoot / checked.relativeRecording /
            checked.segments.front().filename);
        demand(std::fabs(candidateDuration - originalDuration) <=
               std::max(4.0, originalDuration * 0.02),
               "HEVC candidate duration differs from original");
    }
    struct stat folder{};
    demand(::lstat(candidate.c_str(), &folder) == 0 &&
           S_ISDIR(folder.st_mode) && folder.st_uid == ::geteuid() &&
           (folder.st_mode & 0077) == 0,
           "unprotected shrink review directory");
    std::vector<std::string> expectedFiles = {
        ".shrink-review", "00001.ts", "index", "info"
    };
    const bool bound = fs::exists(candidate / kCoreReviewMarker);
    if (bound) {
        demand(readShrinkReviewBinding(candidate).has_value(),
               "shrink review binding invalid");
        expectedFiles.push_back(kCoreReviewMarker);
    }
    const auto subtitleFiles = subtitleNames(checked);
    expectedFiles.insert(expectedFiles.end(),
                         subtitleFiles.begin(), subtitleFiles.end());
    // Reject modifications to previously staged metadata/subtitles before
    // exposing a new recording. The original files remain read-only.
    const auto originalSidecars = checked.videoRoot / checked.relativeRecording;
    requireMatchingSidecar(originalSidecars / "info", candidate / "info");
    for (const auto& name : subtitleFiles)
        requireMatchingSidecar(originalSidecars / name, candidate / name);
    std::size_t seen = 0;
    for (const auto& entry : fs::directory_iterator(candidate)) {
        const auto filename = entry.path().filename().string();
        demand(std::find(expectedFiles.begin(), expectedFiles.end(),
                         filename) != expectedFiles.end() &&
               fs::is_regular_file(entry.symlink_status()),
               "shrink candidate has unexpected or unsafe contents");
        ++seen;
    }
    demand(seen == expectedFiles.size(),
           "shrink candidate is incomplete");
    return candidateBytes;
}

fs::path publishShrinkCandidate(const ShrinkPlan& plan,
                                const fs::path& candidate, ProcessRunner& runner)
{
    // This same verifier is available to an independent, read-only Core
    // reconciliation. Do not publish based solely on FFmpeg exit status.
    (void)verifyShrinkCandidate(plan, candidate, runner);
    const auto checked = inspectShrink(plan.videoRoot,
        plan.relativeRecording, plan.options);
    demand(signature(checked) == signature(plan),
           "original recording changed before publication");
    const auto privateParent = candidate.parent_path();
    std::vector<std::string> expectedFiles = {
        ".shrink-review", "00001.ts", "index", "info"
    };
    const auto subtitles = subtitleNames(checked);
    expectedFiles.insert(expectedFiles.end(),
                         subtitles.begin(), subtitles.end());
    struct stat originalDirectory{};
    const auto originalPath = checked.videoRoot / checked.relativeRecording;
    demand(::lstat(originalPath.c_str(), &originalDirectory) == 0 &&
           S_ISDIR(originalDirectory.st_mode),
           "original ownership unavailable");
    const auto title = checked.relativeRecording.parent_path();
    demand(!title.empty(), "shrink recording requires a title directory");
    const auto destinationParent =
        checked.videoRoot / (title.string() + "_HEVC");
    // Multiple recordings may share one title. Reuse an existing HEVC
    // sibling title only if it is an actual VDR-owned directory. A symlink,
    // writable-by-others parent or preexisting target never gets replaced.
    std::error_code directoryError;
    const bool createdTitle = fs::create_directory(
        destinationParent, directoryError);
    demand(!directoryError ||
           directoryError == std::errc::file_exists,
           "cannot create HEVC title directory");
    Descriptor titleFd(::open(destinationParent.c_str(),
        O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC));
    struct stat titleStat{};
    demand(titleFd.fd >= 0 && ::fstat(titleFd.fd, &titleStat) == 0 &&
           S_ISDIR(titleStat.st_mode),
           "unsafe HEVC title directory");
    if (createdTitle) {
        demand(::fchmod(titleFd.fd, 0700) == 0,
               "cannot protect unpublished HEVC title");
    } else {
        demand(titleStat.st_uid == originalDirectory.st_uid &&
               titleStat.st_gid == originalDirectory.st_gid &&
               (titleStat.st_mode & 0022) == 0,
               "existing HEVC title has unsafe ownership or permissions");
    }
    const auto destination = destinationParent / checked.relativeRecording.filename();
    // Pin the destination directory descriptor through the no-replace move.
    // On an existing title, the candidate remains private until finalized.
    const std::string leaf = checked.relativeRecording.filename().string();
    const long moved = ::syscall(SYS_renameat2, AT_FDCWD, candidate.c_str(),
                                 titleFd.fd, leaf.c_str(), RENAME_NOREPLACE);
    if (moved != 0) {
        if (createdTitle) {
            std::error_code error;
            fs::remove(destinationParent, error);
        }
        throw std::runtime_error("cannot publish HEVC without overwrite");
    }
    bool ready = true;
    for (const auto& name : expectedFiles) {
        if (name == ".shrink-review") continue;
        const auto file = destination / name;
        ready = ready && ::chown(file.c_str(), originalDirectory.st_uid,
                                 originalDirectory.st_gid) == 0 &&
                ::chmod(file.c_str(), 0644) == 0;
    }
    const auto receipt = destination / ".shrink-review";
    if (fs::exists(destination / kCoreReviewMarker))
        ready = ready &&
            ::unlink((destination / kCoreReviewMarker).c_str()) == 0;
    ready = ready && ::unlink(receipt.c_str()) == 0 &&
            ::chown(destination.c_str(), originalDirectory.st_uid,
                    originalDirectory.st_gid) == 0 &&
            ::chmod(destination.c_str(), 0755) == 0 &&
            (!createdTitle ||
                 (::fchown(titleFd.fd, originalDirectory.st_uid,
                            originalDirectory.st_gid) == 0 &&
                  ::fchmod(titleFd.fd, 0755) == 0));
    if (!ready) {
        // A permissions failure is never "success". Keep the original
        // recording untouched and return a private candidate if possible.
        (void)::syscall(SYS_renameat2, AT_FDCWD, destination.c_str(),
                        AT_FDCWD, candidate.c_str(), RENAME_NOREPLACE);
        if (createdTitle) {
            std::error_code error;
            fs::remove(destinationParent, error);
        }
        throw std::runtime_error("HEVC candidate cannot be exposed to VDR");
    }
    // The private candidate parent should now be empty; a non-empty or
    // unexpectedly replaced parent is never recursively removed.
    std::error_code cleanupError;
    fs::remove(privateParent, cleanupError);
    return destination;
}

} // namespace vdrsuite::mediatools
