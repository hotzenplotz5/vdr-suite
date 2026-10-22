#include "MediaShrink.h"

#include <algorithm>
#include <array>
#include <cerrno>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <ctime>
#include <fcntl.h>
#include <fstream>
#include <iomanip>
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
    demand(result.exitCode == 0 && result.output.size() <= 64,
           "shrink codec probe failed");
    std::string codec = result.output;
    while (!codec.empty() && (codec.back() == '\n' || codec.back() == '\r'))
        codec.pop_back();
    demand(!codec.empty() && codec.size() < 64,
           "shrink codec missing");
    return codec;
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
    demand(result.exitCode == 0 && result.output.size() <= 16,
           "shrink height probe failed");
    unsigned height = 0;
    try { height = static_cast<unsigned>(std::stoul(result.output)); }
    catch (...) { throw std::runtime_error("invalid source height"); }
    demand(height > 0 && height <= 16384, "invalid source height");
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

void copyInfo(const ShrinkPlan& plan, const fs::path& destination) {
    const fs::path original = plan.videoRoot / plan.relativeRecording / "info";
    Descriptor source(::open(original.c_str(), O_RDONLY | O_NOFOLLOW | O_CLOEXEC));
    struct stat st{};
    demand(source.fd >= 0 && ::fstat(source.fd, &st) == 0 &&
           S_ISREG(st.st_mode) && st.st_size > 0 && st.st_size <= 1024 * 1024,
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

std::string signature(const ShrinkPlan& plan) {
    std::ostringstream raw;
    raw << plan.videoRoot << '\n' << plan.relativeRecording << '\n'
        << plan.totalBytes << '\n';
    for (const auto& segment : plan.segments)
        raw << segment.filename << ':' << segment.device << ':'
            << segment.inode << ':' << segment.bytes << ':'
            << segment.seconds << ':' << segment.nanoseconds << '\n';
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
    ShrinkEncoder requested, bool scaled)
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
    if (plan.options.nightMode)
        args.insert(args.end(), {"-c:a", "aac", "-b:a", "192k",
                                 "-ac", "2", "-af", "loudnorm"});
    else
        args.insert(args.end(), {"-c:a", "copy"});
    args.insert(args.end(), {"-fs", std::to_string(limit),
                             "-f", "mpegts", "-y", output.string()});
    return args;
}

} // namespace

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
    demand(nonempty(recording / "index") || nonempty(recording / "index.vdr"),
           "original VDR index missing");
    return plan;
}

void requireShrinkCapacity(const ShrinkPlan& plan)
{
    struct statvfs fsInfo{};
    demand(::statvfs(plan.videoRoot.c_str(), &fsInfo) == 0 &&
           fsInfo.f_frsize > 0, "cannot inspect shrink destination capacity");
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
    const auto source = stage.path / "input.ts";
    const auto output = stage.path / "00001.ts";
    copySegments(current, source);
    const auto codec = probeCodec(runner, source);
    demand(codec != "hevc", "recording already contains HEVC video");
    const double originalDuration = probeDuration(runner, source);
    const auto height = probeHeight(runner, source);
    const bool scaling = current.options.maxHeight != 0 &&
        height > current.options.maxHeight;
    demand(current.totalBytes <= (UINT64_MAX - 16 * MiB) / 2,
           "shrink output limit overflow");
    const auto limit = current.totalBytes * 2 + 16 * MiB;
    auto argv = ffmpegArguments(current, source, output, limit,
                                 current.options.encoder, scaling);
    auto conversion = runner.run(argv, false);
    if (conversion.exitCode != 0 &&
        current.options.encoder != ShrinkEncoder::software) {
        // Hardware failure may leave a partial TS. Only our private output
        // can be removed; software starts from the same immutable snapshot.
        std::error_code error;
        fs::remove(output, error);
        demand(!error, "cannot clear failed hardware candidate");
        argv = ffmpegArguments(current, source, output, limit,
                               ShrinkEncoder::software, scaling);
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
    invoke(runner, {"/usr/bin/vdr", "--genindex=" + stage.path.string()});
    demand(nonempty(stage.path / "index"), "HEVC recording index missing");
    copyInfo(current, stage.path / "info");
    for (const auto& part : current.segments)
        demand(same(identity(current.videoRoot / current.relativeRecording /
                             part.filename), part),
               "source recording changed during HEVC operation");
    std::ofstream review(stage.path / ".shrink-review",
                         std::ios::binary | std::ios::trunc);
    demand(static_cast<bool>(review), "cannot create shrink review evidence");
    review << signature(current);
    review.flush();
    demand(static_cast<bool>(review), "cannot finish shrink review evidence");
    demand(fs::remove(source), "cannot remove private TS snapshot");
    const fs::path candidate = stage.path;
    stage.path.clear();
    return candidate;
}

fs::path publishShrinkCandidate(const ShrinkPlan& plan,
                                const fs::path& candidate, ProcessRunner& runner)
{
    const auto checked = inspectShrink(plan.videoRoot,
        plan.relativeRecording, plan.options);
    demand(signature(checked) == signature(plan), "original recording changed");
    demand(candidate.is_absolute() &&
           candidate.lexically_normal() == candidate &&
           candidate.parent_path() == checked.videoRoot &&
           candidate.filename().string().rfind(".vdr-suite-shrink-", 0) == 0,
           "invalid shrink review location");
    prohibitSymlinks(candidate);
    demand(fs::is_directory(candidate), "shrink review candidate missing");
    std::ifstream evidence(candidate / ".shrink-review", std::ios::binary);
    demand(static_cast<bool>(evidence), "shrink review evidence missing");
    const std::string saved((std::istreambuf_iterator<char>(evidence)),
                            std::istreambuf_iterator<char>());
    demand(saved == signature(checked), "candidate belongs to a different recording");
    demand(nonempty(candidate / "00001.ts") &&
           nonempty(candidate / "index") && nonempty(candidate / "info"),
           "unverified HEVC candidate files");
    demand(probeCodec(runner, candidate / "00001.ts") == "hevc",
           "published candidate is not HEVC");
    const auto title = checked.relativeRecording.parent_path();
    demand(!title.empty(), "shrink recording requires a title directory");
    const auto destinationParent =
        checked.videoRoot / (title.string() + "_HEVC");
    demand(!fs::exists(destinationParent), "shrink target title already exists");
    demand(fs::create_directory(destinationParent),
           "cannot create new HEVC title directory");
    const auto destination = destinationParent / checked.relativeRecording.filename();
    // Commit by non-replacing rename only. Neither TS nor index of the
    // original recording is ever renamed or deleted by this operation.
    const long moved = ::syscall(SYS_renameat2, AT_FDCWD, candidate.c_str(),
                                  AT_FDCWD, destination.c_str(), RENAME_NOREPLACE);
    if (moved != 0) {
        std::error_code error;
        fs::remove(destinationParent, error); // empty newly created directory
        throw std::runtime_error("cannot publish HEVC without overwrite");
    }
    std::error_code error;
    fs::remove(destination / ".shrink-review", error);
    return destination;
}

} // namespace vdrsuite::mediatools
