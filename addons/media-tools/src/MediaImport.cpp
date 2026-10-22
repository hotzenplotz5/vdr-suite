#include "MediaImport.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <array>
#include <cerrno>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <fcntl.h>
#include <fstream>
#include <cstdint>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <signal.h>
#include <linux/fs.h>

namespace vdrsuite::mediatools {
namespace {

constexpr auto kGiB = std::uint64_t{1024} * 1024 * 1024;
constexpr auto kMiB = std::uint64_t{1024} * 1024;
constexpr auto kMaxProcessOutput = std::size_t{4096};

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

bool isChild(const fs::path& root, const fs::path& candidate) {
    auto a = root.begin();
    auto b = candidate.begin();
    for (; a != root.end() && b != candidate.end(); ++a, ++b) {
        if (*a != *b) return false;
    }
    return a == root.end() && b != candidate.end();
}

bool isChildOrEqual(const fs::path& root, const fs::path& candidate) {
    return root == candidate || isChild(root, candidate);
}

void rejectSymlinkComponents(const fs::path& path) {
    fs::path current;
    for (const auto& part : path) {
        current /= part;
        require(!fs::is_symlink(fs::symlink_status(current)),
                "symlinked path component is prohibited");
    }
}

fs::path checkedRoot(const fs::path& root) {
    require(root.is_absolute() && root.lexically_normal() == root,
            "root must be an absolute normalized directory");
    rejectSymlinkComponents(root);
    require(fs::is_directory(root), "configured root is not a directory");
    return fs::canonical(root);
}

std::string safeTitle(const std::string& original) {
    std::string title;
    title.reserve(std::min<std::size_t>(original.size(), 120));
    for (const unsigned char c : original) {
        if (title.size() >= 120) break;
        if (c < 32 || c == 127) continue;
        if (c == '/' || c == '\\' || c == ':' || c == '*' || c == '?' ||
            c == '"' || c == '<' || c == '>' || c == '|') {
            title.push_back('_');
        } else {
            title.push_back(static_cast<char>(c));
        }
    }
    while (!title.empty() && (title.front() == '.' || title.front() == ' ')) title.erase(title.begin());
    while (!title.empty() && (title.back() == '.' || title.back() == ' ')) title.pop_back();
    require(!title.empty() && title != "." && title != "..", "invalid filename title");
    return title;
}

struct Signature {
    dev_t device{};
    ino_t inode{};
    off_t size{};
    std::int64_t seconds{};
    long nanoseconds{};
};

Signature signature(const fs::path& path) {
    struct stat st{};
    require(::lstat(path.c_str(), &st) == 0 && S_ISREG(st.st_mode) && st.st_size > 0,
            "source changed or is not a nonempty regular file");
    return {st.st_dev, st.st_ino, st.st_size, st.st_mtim.tv_sec, st.st_mtim.tv_nsec};
}

bool same(const Signature& a, const Signature& b) {
    return a.device == b.device && a.inode == b.inode && a.size == b.size &&
           a.seconds == b.seconds && a.nanoseconds == b.nanoseconds;
}

bool containsRecording(const fs::path& titleDirectory) {
    if (!fs::exists(titleDirectory)) return false;
    require(!fs::is_symlink(fs::symlink_status(titleDirectory)) &&
            fs::is_directory(titleDirectory), "invalid existing title directory");
    for (const auto& item : fs::directory_iterator(titleDirectory)) {
        if (item.path().extension() == ".rec")
            return true; // Conservative: even an incomplete .rec counts as a duplicate.
    }
    return false;
}

std::string recordingLeaf() {
    std::time_t now = std::time(nullptr);
    std::tm local{};
    require(::localtime_r(&now, &local) != nullptr, "localtime failed");
    char buffer[64]{};
    require(std::strftime(buffer, sizeof(buffer), "%Y-%m-%d.%H.%M.1-0.rec", &local) != 0,
            "date formatting failed");
    return buffer;
}

bool nonemptyRegular(const fs::path& path) {
    const auto status = fs::symlink_status(path);
    return fs::is_regular_file(status) && fs::file_size(path) > 0;
}

struct StageCleanup {
    fs::path path;
    ~StageCleanup() {
        if (!path.empty()) {
            std::error_code ec;
            fs::remove_all(path, ec);
        }
    }
};

struct FdGuard {
    int fd;
    explicit FdGuard(int value) : fd(value) {}
    ~FdGuard() { if (fd >= 0) ::close(fd); }
    FdGuard(const FdGuard&) = delete;
    FdGuard& operator=(const FdGuard&) = delete;
};

// Copy the exact opened source into private backend-local staging. Both media
// subprocesses operate exclusively on this snapshot, never on a pathname
// that an external importer can swap between probe and remux.
void stageVerifiedSource(const ImportPlan& plan, const Signature& expected,
                         const fs::path& destination) {
    FdGuard input(::open(plan.sourceFile.c_str(),
                         O_RDONLY | O_NOFOLLOW | O_CLOEXEC));
    require(input.fd >= 0, "cannot open source for private staging");
    struct stat initial{};
    require(::fstat(input.fd, &initial) == 0 && S_ISREG(initial.st_mode) &&
            same(expected, {initial.st_dev, initial.st_ino, initial.st_size,
                            initial.st_mtim.tv_sec, initial.st_mtim.tv_nsec}),
            "source identity changed before snapshot");
    FdGuard output(::open(destination.c_str(),
                          O_WRONLY | O_CREAT | O_EXCL | O_NOFOLLOW | O_CLOEXEC,
                          0600));
    require(output.fd >= 0, "cannot create private source snapshot");
    std::array<char, 65536> buffer{};
    std::uintmax_t copied = 0;
    while (true) {
        const ssize_t amount = ::read(input.fd, buffer.data(), buffer.size());
        if (amount < 0 && errno == EINTR) continue;
        require(amount >= 0, "source snapshot read failed");
        if (amount == 0) break;
        require(copied + static_cast<std::uintmax_t>(amount) <= plan.sourceBytes,
                "source grew during snapshot");
        std::size_t offset = 0;
        while (offset < static_cast<std::size_t>(amount)) {
            const ssize_t written = ::write(output.fd, buffer.data() + offset,
                static_cast<std::size_t>(amount) - offset);
            if (written < 0 && errno == EINTR) continue;
            require(written > 0, "source snapshot write failed");
            offset += static_cast<std::size_t>(written);
        }
        copied += static_cast<std::uintmax_t>(amount);
    }
    struct stat after{};
    require(copied == plan.sourceBytes && ::fstat(input.fd, &after) == 0 &&
            same(expected, {after.st_dev, after.st_ino, after.st_size,
                            after.st_mtim.tv_sec, after.st_mtim.tv_nsec}),
            "source changed during snapshot");
    require(::fsync(output.fd) == 0, "cannot sync source snapshot");
}

void runRequired(ProcessRunner& runner, const std::vector<std::string>& argv) {
    const auto result = runner.run(argv, false);
    require(result.exitCode == 0, "external media command failed");
}

} // namespace

ImportPlan inspectImport(const ImportRequest& request) {
    const auto sourceRoot = checkedRoot(request.sourceRoot);
    const auto videoRoot = checkedRoot(request.videoRoot);
    require(!isChildOrEqual(sourceRoot, videoRoot) &&
            !isChildOrEqual(videoRoot, sourceRoot),
            "source and video roots must be separate");
    require(request.sourceFile.is_absolute() &&
            request.sourceFile.lexically_normal() == request.sourceFile,
            "source filename must be absolute and normalized");
    require(isChild(sourceRoot, request.sourceFile),
            "source file is outside the allowed import root");
    rejectSymlinkComponents(request.sourceFile);
    const auto resolved = fs::canonical(request.sourceFile);
    require(isChild(sourceRoot, resolved), "source resolves outside import root");
    const auto sourceIdentity = signature(resolved);
    const auto size = sourceIdentity.size;
    std::string extension = resolved.extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    require(extension == ".mkv" || extension == ".mp4" || extension == ".ts" ||
            extension == ".mov" || extension == ".avi" || extension == ".webm" ||
            extension == ".m4v",
            "unsupported source extension");
    const auto title = safeTitle(resolved.stem().string());
    const auto folder = videoRoot / title;
    require(!containsRecording(folder), "target already has a VDR recording");
    return {sourceRoot, videoRoot, resolved, folder, title,
            static_cast<std::uintmax_t>(size),
            static_cast<std::uint64_t>(sourceIdentity.device),
            static_cast<std::uint64_t>(sourceIdentity.inode),
            sourceIdentity.seconds,
            sourceIdentity.nanoseconds};
}

void requireProductionCapacity(const ImportPlan& plan) {
    struct statvfs st{};
    require(::statvfs(plan.videoRoot.c_str(), &st) == 0, "destination capacity is unavailable");
    const auto block = static_cast<std::uint64_t>(st.f_frsize);
    require(block != 0 && st.f_blocks <= UINT64_MAX / block &&
            st.f_bavail <= UINT64_MAX / block, "capacity overflow");
    const auto total = st.f_blocks * block;
    const auto available = st.f_bavail * block;
    const auto reserve = std::max(4 * kGiB, total / 100 * 15);
    // One unchanged source snapshot and an output capped at twice the
    // source size plus 16 MiB. Keep extra filesystem reserve independent
    // of whether stream copy or software encoding is chosen.
    require(plan.sourceBytes <=
            (static_cast<std::uint64_t>(INT64_MAX) - 64 * kMiB) / 3,
            "source too large for bounded preflight");
    const auto budget = static_cast<std::uint64_t>(plan.sourceBytes) * 3 + 64 * kMiB;
    require(available >= reserve && available - reserve >= budget,
            "insufficient capacity for import staging and reserve");
}

fs::path executeRemuxImport(const ImportPlan& plan, ProcessRunner& runner,
                            const std::function<void(const ImportPlan&)>& capacityPreflight) {
    // Re-inspect at execution time: no untrusted request may reuse a stale plan.
    const auto fresh = inspectImport({plan.sourceRoot, plan.videoRoot, plan.sourceFile});
    require(fresh.titleDirectory == plan.titleDirectory &&
            fresh.sourceBytes == plan.sourceBytes && fresh.title == plan.title &&
            fresh.sourceDevice == plan.sourceDevice &&
            fresh.sourceInode == plan.sourceInode &&
            fresh.sourceMtimeSeconds == plan.sourceMtimeSeconds &&
            fresh.sourceMtimeNanoseconds == plan.sourceMtimeNanoseconds,
            "import plan is stale");
    const auto before = signature(plan.sourceFile);
    require(static_cast<std::uintmax_t>(before.size) == plan.sourceBytes, "source changed");

    // Per-VDR-root interprocess exclusion. The lock file remains owned by the
    // backend worker; no stale-PID guessing or global /tmp 0777 job directory.
    const auto lockName = plan.videoRoot / ".vdr-suite-media-import.lock";
    FdGuard lock(::open(lockName.c_str(), O_CREAT | O_RDWR | O_CLOEXEC | O_NOFOLLOW, 0600));
    require(lock.fd >= 0 && ::flock(lock.fd, LOCK_EX | LOCK_NB) == 0,
            "another import owns the backend lock");
    require(!containsRecording(plan.titleDirectory), "target appeared during import");
    capacityPreflight(plan);

    std::string templatePath = (plan.videoRoot / ".vdr-suite-import-XXXXXX").string();
    std::vector<char> buffer(templatePath.begin(), templatePath.end());
    buffer.push_back('\0');
    char* created = ::mkdtemp(buffer.data());
    require(created != nullptr, "cannot create private staging directory");
    StageCleanup stage{fs::path(created)};
    require(::chmod(stage.path.c_str(), 0700) == 0, "staging permissions failed");

    // The reserve accounts for this complete source snapshot plus remux.
    const auto stagedSource =
        stage.path / ("input" + plan.sourceFile.extension().string());
    stageVerifiedSource(plan, before, stagedSource);
    const auto probe = runner.run({
        "/usr/bin/ffprobe", "-v", "error", "-select_streams", "v:0",
        "-show_entries", "stream=codec_name", "-of", "default=nw=1:nk=1",
        stagedSource.string()
    }, true);
    require(probe.exitCode == 0, "ffprobe failed");
    std::string codec = probe.output;
    while (!codec.empty() && (codec.back() == '\n' || codec.back() == '\r'))
        codec.pop_back();
    require(!codec.empty() && codec.size() <= 64 &&
            std::all_of(codec.begin(), codec.end(), [](unsigned char ch) {
                return std::islower(ch) || std::isdigit(ch) || ch == '_';
            }), "invalid video codec from ffprobe");
    const bool copyVideo = codec == "h264" || codec == "hevc" ||
                           codec == "mpeg2video";

    {
        std::ofstream info(stage.path / "info", std::ios::binary | std::ios::trunc);
        require(static_cast<bool>(info), "cannot create VDR metadata");
        info << "T " << plan.title << "\n";
        info << "S Imported via VDR-Suite Media Tools\n";
        info.flush();
        require(static_cast<bool>(info), "metadata write failed");
    }

    const auto output = stage.path / "00001.ts";
    // FFmpeg may otherwise consume all free space on badly behaved inputs.
    // Its -fs threshold is additionally checked before publishing: hitting
    // the threshold can return exit 0 with a truncated transport stream.
    const auto outputLimit = static_cast<std::uint64_t>(plan.sourceBytes) * 2 +
                             16 * kMiB;
    std::vector<std::string> encode = {
        "/usr/bin/ffmpeg", "-nostdin", "-hide_banner", "-v", "error",
        "-i", stagedSource.string(), "-map", "0:v:0", "-map", "0:a?",
        "-sn"
    };
    if (copyVideo) {
        encode.insert(encode.end(), {"-c", "copy"});
    } else {
        // Unsupported source codecs are converted to VDR-compatible H.264.
        // The separate HEVC shrink operation never runs in this path.
        encode.insert(encode.end(), {
            "-c:v", "libx264", "-preset", "medium", "-crf", "23",
            "-pix_fmt", "yuv420p", "-threads", "2",
            "-c:a", "aac", "-b:a", "192k"
        });
    }
    encode.insert(encode.end(), {
        "-fs", std::to_string(outputLimit), "-f", "mpegts",
        "-y", output.string()
    });
    runRequired(runner, encode);
    require(nonemptyRegular(output), "import output missing or empty");
    require(fs::file_size(output) < outputLimit,
            "import output reached size limit; incomplete recording rejected");
    runRequired(runner, {"/usr/bin/vdr", "--genindex=" + stage.path.string()});
    require(nonemptyRegular(stage.path / "index"), "VDR index missing or empty");
    require(same(before, signature(plan.sourceFile)), "source changed during media processing");
    require(fs::remove(stagedSource), "cannot remove private source snapshot");
    require(!containsRecording(plan.titleDirectory), "duplicate appeared before commit");

    if (!fs::exists(plan.titleDirectory)) {
        require(fs::create_directory(plan.titleDirectory), "cannot create title directory");
    }
    require(!fs::is_symlink(fs::symlink_status(plan.titleDirectory)) &&
            fs::is_directory(plan.titleDirectory), "title directory changed");
    const auto destination = plan.titleDirectory / recordingLeaf();
    require(!fs::exists(destination), "destination recording already exists");
    // Atomic no-replace promotion; no source deletion and no rename-overwrite.
    const long rc = ::syscall(SYS_renameat2, AT_FDCWD, stage.path.c_str(),
                              AT_FDCWD, destination.c_str(), RENAME_NOREPLACE);
    require(rc == 0, "atomic non-overwriting recording promotion failed");
    stage.path.clear();
    return destination;
}

ProcessResult NativeProcessRunner::run(const std::vector<std::string>& args, bool captureOutput) {
    require(!args.empty() && !args[0].empty() && args[0][0] == '/',
            "absolute external command required");
    int pipefd[2]{-1, -1};
    if (captureOutput) require(::pipe2(pipefd, O_CLOEXEC) == 0, "pipe failed");
    const auto pid = ::fork();
    if (pid < 0) {
        if (captureOutput) { ::close(pipefd[0]); ::close(pipefd[1]); }
        throw std::runtime_error("fork failed");
    }
    if (pid == 0) {
        const int nullfd = ::open("/dev/null", O_RDWR);
        if (nullfd < 0) _exit(127);
        ::dup2(nullfd, STDIN_FILENO);
        ::dup2(nullfd, STDERR_FILENO);
        if (captureOutput) {
            ::close(pipefd[0]);
            ::dup2(pipefd[1], STDOUT_FILENO);
            ::close(pipefd[1]);
        } else {
            ::dup2(nullfd, STDOUT_FILENO);
        }
        ::close(nullfd);
        std::vector<char*> argv;
        for (const auto& arg : args) argv.push_back(const_cast<char*>(arg.c_str()));
        argv.push_back(nullptr);
        ::execv(argv[0], argv.data());
        _exit(127);
    }
    ProcessResult result;
    if (captureOutput) {
        ::close(pipefd[1]);
        std::array<char, 256> chunk{};
        while (true) {
            const ssize_t bytes = ::read(pipefd[0], chunk.data(), chunk.size());
            if (bytes == 0) break;
            if (bytes < 0) {
                if (errno == EINTR) continue;
                break;
            }
            if (result.output.size() + static_cast<std::size_t>(bytes) <= kMaxProcessOutput)
                result.output.append(chunk.data(), static_cast<std::size_t>(bytes));
            else
                result.output = "probe-output-too-long";
        }
        ::close(pipefd[0]);
    }
    int status = 0;
    while (::waitpid(pid, &status, 0) < 0) {
        if (errno != EINTR) throw std::runtime_error("waitpid failed");
    }
    result.exitCode = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
    return result;
}

} // namespace vdrsuite::mediatools
