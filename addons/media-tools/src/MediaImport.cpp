#include "MediaImport.h"

#include <algorithm>
#include <cctype>
#include <cmath>
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
#include <poll.h>
#include <optional>
#include <sstream>
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


std::optional<std::string> readOptionalSidecar(const fs::path& file,
                                               std::size_t maxBytes) {
    // The adjacent sidecar is never taken from a browser or an Agent payload.
    // Absence is fine, but a symlink, special file or oversized sidecar is not.
    if (!fs::exists(fs::symlink_status(file))) return std::nullopt;
    FdGuard input(::open(file.c_str(), O_RDONLY | O_NOFOLLOW | O_CLOEXEC));
    require(input.fd >= 0, "cannot open trusted adjacent sidecar");
    struct stat before{}, after{};
    require(::fstat(input.fd, &before) == 0 && S_ISREG(before.st_mode) &&
            before.st_size >= 0 &&
            static_cast<std::uint64_t>(before.st_size) <= maxBytes,
            "invalid sidecar type or size");
    std::string content;
    content.reserve(static_cast<std::size_t>(before.st_size));
    std::array<char, 8192> bytes{};
    while (true) {
        const ssize_t amount = ::read(input.fd, bytes.data(), bytes.size());
        if (amount < 0 && errno == EINTR) continue;
        require(amount >= 0 && content.size() +
                static_cast<std::size_t>(amount) <= maxBytes,
                "sidecar changed or grew beyond limit");
        if (amount == 0) break;
        content.append(bytes.data(), static_cast<std::size_t>(amount));
    }
    require(::fstat(input.fd, &after) == 0 &&
            same({before.st_dev, before.st_ino, before.st_size,
                  before.st_mtim.tv_sec, before.st_mtim.tv_nsec},
                 {after.st_dev, after.st_ino, after.st_size,
                  after.st_mtim.tv_sec, after.st_mtim.tv_nsec}),
            "sidecar changed during reading");
    return content;
}

std::string xmlText(const std::string& xml, const std::string& name) {
    const std::string open = "<" + name + ">";
    const std::string close = "</" + name + ">";
    const auto first = xml.find(open);
    if (first == std::string::npos) return {};
    const auto start = first + open.size();
    const auto end = xml.find(close, start);
    require(end != std::string::npos && end - start <= 4096,
            "malformed or excessively long NFO field");
    std::string raw = xml.substr(start, end - start);
    require(raw.find('<') == std::string::npos,
            "nested markup in NFO field is unsupported");
    std::string result;
    for (std::size_t i = 0; i < raw.size();) {
        if (raw[i] != '&') {
            const unsigned char ch = static_cast<unsigned char>(raw[i++]);
            result.push_back(ch < 32 || ch == 127 ? ' ' : static_cast<char>(ch));
            continue;
        }
        bool matched = false;
        for (const auto& entity : {
                std::pair<const char*, char>{"&amp;", '&'},
                {"&lt;", '<'}, {"&gt;", '>'}, {"&quot;", '"'},
                {"&apos;", '\''}}) {
            const std::string value(entity.first);
            if (raw.compare(i, value.size(), value) == 0) {
                result.push_back(entity.second);
                i += value.size();
                matched = true;
                break;
            }
        }
        require(matched, "unsupported NFO entity");
    }
    const auto firstCharacter = result.find_first_not_of(' ');
    if (firstCharacter == std::string::npos) return {};
    const auto lastCharacter = result.find_last_not_of(' ');
    return result.substr(firstCharacter, lastCharacter - firstCharacter + 1);
}

struct ImportMetadata {
    std::string title, shortText, description, year, genre;
};

ImportMetadata adjacentMetadata(const fs::path& sourceFile) {
    ImportMetadata metadata;
    fs::path file = sourceFile;
    file.replace_extension(".nfo");
    const auto nfo = readOptionalSidecar(file, 65536);
    if (!nfo) return metadata;
    metadata.title = xmlText(*nfo, "title");
    metadata.shortText = xmlText(*nfo, "outline");
    metadata.description = xmlText(*nfo, "plot");
    metadata.year = xmlText(*nfo, "year");
    metadata.genre = xmlText(*nfo, "genre");
    if (!metadata.year.empty()) {
        require(metadata.year.size() == 4 &&
                std::all_of(metadata.year.begin(), metadata.year.end(),
                    [](unsigned char ch) { return std::isdigit(ch); }),
                "invalid year in NFO");
    }
    return metadata;
}

void stageOptionalSubtitle(const fs::path& sourceFile,
                           const fs::path& stage) {
    const auto stem = sourceFile.stem().string();
    const auto prefix = stem + ".";
    std::vector<std::pair<fs::path, std::string>> discovered;
    std::size_t inspected = 0;
    for (const auto& entry : fs::directory_iterator(sourceFile.parent_path())) {
        require(++inspected <= 4096, "too many adjacent source files");
        const std::string name = entry.path().filename().string();
        std::string destination;
        if (name == prefix + "srt") {
            // The existing Suite Sidecar reader gives 00001.srt precedence.
            destination = "00001.srt";
        } else if (name.size() > prefix.size() + 4 &&
                   name.compare(0, prefix.size(), prefix) == 0 &&
                   name.compare(name.size() - 4, 4, ".srt") == 0) {
            const auto language = name.substr(
                prefix.size(), name.size() - prefix.size() - 4);
            if (language.size() < 2 || language.size() > 12 ||
                !std::all_of(language.begin(), language.end(),
                    [](unsigned char ch) {
                        return std::isalnum(ch) || ch == '-';
                    })) continue;
            destination = "subtitles." + language + ".srt";
        } else continue;
        require(!fs::is_symlink(entry.symlink_status()) &&
                entry.is_regular_file(), "unsafe adjacent SRT sidecar");
        require(discovered.size() < 16, "too many adjacent SRT sidecars");
        discovered.emplace_back(entry.path(), destination);
    }
    std::sort(discovered.begin(), discovered.end(),
        [](const auto& a, const auto& b) { return a.first < b.first; });
    std::uint64_t totalSubtitleBytes = 0;
    for (const auto& file : discovered) {
        const auto data = readOptionalSidecar(file.first, 8 * kMiB);
        require(data && !data->empty(), "missing or empty SRT sidecar");
        require(data->size() <= 16 * kMiB - totalSubtitleBytes,
                "total SRT sidecar size exceeds staging budget");
        totalSubtitleBytes += data->size();
        std::ofstream output(stage / file.second,
                             std::ios::binary | std::ios::trunc);
        require(static_cast<bool>(output), "cannot stage SRT sidecar");
        output.write(data->data(), static_cast<std::streamsize>(data->size()));
        output.flush();
        require(static_cast<bool>(output), "cannot write SRT sidecar");
    }
}

double probeDuration(ProcessRunner& runner, const fs::path& file) {
    const auto result = runner.run({
        "/usr/bin/ffprobe", "-v", "error", "-show_entries",
        "format=duration", "-of", "default=nw=1:nk=1", file.string()
    }, true);
    require(result.exitCode == 0 && result.output.size() < 64,
            "cannot verify media duration");
    std::size_t used = 0;
    double seconds = 0;
    try { seconds = std::stod(result.output, &used); }
    catch (...) { throw std::runtime_error("invalid media duration"); }
    require(used > 0 && result.output.find_first_not_of(" \r\n\t", used) ==
            std::string::npos && std::isfinite(seconds) && seconds > 0.0,
            "invalid media duration");
    return seconds;
}

void runRequired(ProcessRunner& runner, const std::vector<std::string>& argv) {
    const auto result = runner.run(argv, false);
    require(result.exitCode == 0, "external media command failed");
}

} // namespace

std::string parseProbedVideoCodec(const std::string& output) {
    require(!output.empty() && output.size() <= 4096,
            "invalid video codec probe length");
    std::istringstream lines(output);
    std::string uniqueCodec;
    std::string token;
    std::size_t count = 0;
    while (std::getline(lines, token)) {
        if (!token.empty() && token.back() == '\r') token.pop_back();
        if (token.empty()) continue;
        require(token.size() <= 64 &&
                std::all_of(token.begin(), token.end(), [](unsigned char c) {
                    return std::islower(c) || std::isdigit(c) || c == '_';
                }), "invalid video codec from ffprobe");
        if (!uniqueCodec.empty())
            require(token == uniqueCodec,
                    "inconsistent video codec streams from ffprobe");
        else uniqueCodec = token;
        require(++count <= 64, "too many video codec entries");
    }
    require(!uniqueCodec.empty(), "missing video codec from ffprobe");
    return uniqueCodec;
}

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
    const auto metadata = adjacentMetadata(resolved);
    const auto title = safeTitle(metadata.title.empty() ?
        resolved.stem().string() : metadata.title);
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
    require((st.f_flag & ST_RDONLY) == 0 &&
            ::access(plan.videoRoot.c_str(), W_OK | X_OK) == 0,
            "destination is not writable by the media worker");
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
    // VDR's --genindex expects an actual recording directory. Keep the
    // canonical timestamped .rec leaf hidden inside the private staging
    // parent until every media/index verification has succeeded.
    const auto leaf = recordingLeaf();
    const auto stagedRecording = stage.path / leaf;
    require(fs::create_directory(stagedRecording),
            "cannot create private VDR recording directory");
    require(::chmod(stagedRecording.c_str(), 0700) == 0,
            "cannot protect private VDR recording directory");

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
    const std::string codec = parseProbedVideoCodec(probe.output);
    const bool copyVideo = codec == "h264" || codec == "hevc" ||
                           codec == "mpeg2video";
    const double inputDuration = probeDuration(runner, stagedSource);
    const auto audio = runner.run({
        "/usr/bin/ffprobe", "-v", "error", "-select_streams", "a",
        "-show_entries", "stream=codec_name",
        "-of", "default=nw=1:nk=1", stagedSource.string()
    }, true);
    require(audio.exitCode == 0 && audio.output.size() <= 2048,
            "cannot inspect source audio codecs");
    bool copyAudio = true;
    std::istringstream streams(audio.output);
    for (std::string name; std::getline(streams, name);) {
        if (!name.empty() && name.back() == '\r') name.pop_back();
        if (!name.empty() && name != "aac" && name != "ac3" &&
            name != "eac3" && name != "mp2")
            copyAudio = false;
    }

    {
        // VDR's native info sidecar is the only recording metadata writer.
        // NFO stays next to its original media source; no shadow DB exists.
        const auto metadata = adjacentMetadata(plan.sourceFile);
        const std::string currentTitle = safeTitle(metadata.title.empty() ?
            plan.sourceFile.stem().string() : metadata.title);
        require(currentTitle == plan.title, "NFO title changed during import");
        std::ofstream info(stagedRecording / "info", std::ios::binary | std::ios::trunc);
        require(static_cast<bool>(info), "cannot create VDR metadata");
        info << "T " << plan.title << "\n";
        info << "S " << (metadata.shortText.empty() ?
            "Imported via VDR-Suite Media Tools" : metadata.shortText) << "\n";
        std::string description = metadata.description;
        if (!metadata.year.empty()) {
            if (!description.empty()) description += " | ";
            description += "Year: " + metadata.year;
        }
        if (!metadata.genre.empty()) {
            if (!description.empty()) description += " | ";
            description += "Genre: " + metadata.genre;
        }
        if (!description.empty()) info << "D " << description << "\n";
        info.flush();
        require(static_cast<bool>(info), "metadata write failed");
    }
    stageOptionalSubtitle(plan.sourceFile, stagedRecording);

    const auto output = stagedRecording / "00001.ts";
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
    if (copyVideo)
        encode.insert(encode.end(), {"-c:v", "copy"});
    else {
        encode.insert(encode.end(), {
            "-c:v", "libx264", "-preset", "medium", "-crf", "23",
            "-pix_fmt", "yuv420p", "-threads", "2",
            "-g", "50"
        });
    }
    // Compatible video does not imply compatible audio in an MKV/WebM.
    if (copyAudio)
        encode.insert(encode.end(), {"-c:a", "copy"});
    else
        encode.insert(encode.end(), {"-c:a", "aac", "-b:a", "192k"});
    encode.insert(encode.end(), {
        "-fs", std::to_string(outputLimit), "-f", "mpegts",
        "-y", output.string()
    });
    runRequired(runner, encode);
    require(nonemptyRegular(output), "import output missing or empty");
    require(fs::file_size(output) < outputLimit,
            "import output reached size limit; incomplete recording rejected");
    // FFmpeg can exit 0 after writing a partial TS. Compare independently
    // probed output duration before generating a VDR index or publishing.
    const double outputDuration = probeDuration(runner, output);
    require(std::fabs(inputDuration - outputDuration) <=
            std::max(4.0, inputDuration * 0.02),
            "import duration differs from source; incomplete output rejected");
    const auto encodedVideo = runner.run({
        "/usr/bin/ffprobe", "-v", "error", "-select_streams", "v:0",
        "-show_entries", "stream=codec_name",
        "-of", "default=nw=1:nk=1", output.string()
    }, true);
    const std::string expectedCodec = copyVideo ? codec : "h264";
    require(encodedVideo.exitCode == 0 &&
            parseProbedVideoCodec(encodedVideo.output) == expectedCodec,
            "import output video codec verification failed");
    runRequired(runner, {"/usr/bin/vdr", "--genindex=" + stagedRecording.string()});
    require(nonemptyRegular(stagedRecording / "index"), "VDR index missing or empty");
    require(same(before, signature(plan.sourceFile)), "source changed during media processing");
    require(fs::remove(stagedSource), "cannot remove private source snapshot");
    require(!containsRecording(plan.titleDirectory), "duplicate appeared before commit");

    if (!fs::exists(plan.titleDirectory)) {
        require(fs::create_directory(plan.titleDirectory), "cannot create title directory");
    }
    require(!fs::is_symlink(fs::symlink_status(plan.titleDirectory)) &&
            fs::is_directory(plan.titleDirectory), "title directory changed");
    const auto destination = plan.titleDirectory / leaf;
    require(!fs::exists(destination), "destination recording already exists");
    // Atomic no-replace promotion of the canonical .rec only; the private
    // parent and source snapshot never become part of a visible recording.
    const long rc = ::syscall(SYS_renameat2, AT_FDCWD,
                              stagedRecording.c_str(), AT_FDCWD,
                              destination.c_str(), RENAME_NOREPLACE);
    require(rc == 0, "atomic non-overwriting recording promotion failed");
    return destination;
}

NativeProcessRunner::NativeProcessRunner(std::chrono::seconds maximum)
    : maximum_(maximum) {
    require(maximum_.count() >= 1 && maximum_.count() <= 4 * 3600,
            "invalid native media command deadline");
}

ProcessResult NativeProcessRunner::run(const std::vector<std::string>& args,
                                       bool captureOutput) {
    require(!args.empty() && !args[0].empty() && args[0][0] == '/',
            "absolute external command required");
    int pipefd[2]{-1, -1};
    if (captureOutput)
        require(::pipe2(pipefd, O_CLOEXEC | O_NONBLOCK) == 0,
                "media command pipe failed");
    const pid_t pid = ::fork();
    if (pid < 0) {
        if (captureOutput) { ::close(pipefd[0]); ::close(pipefd[1]); }
        throw std::runtime_error("fork failed");
    }
    if (pid == 0) {
        // Keep the exact protected shared maintenance lock across execv.
        // If the Agent crashes while ffmpeg is still running, the kernel
        // lock must remain held by that child until it terminates.
        if (maintenanceLockFd_ >= 0) {
            const int flags = ::fcntl(maintenanceLockFd_, F_GETFD);
            if (flags < 0 ||
                ::fcntl(maintenanceLockFd_, F_SETFD,
                        flags & ~FD_CLOEXEC) != 0)
                ::_exit(127);
        }
        // The child remains in the Agent handler's process group: the
        // canonical Agent deadline can terminate it even if this parent dies.
        const int nullfd = ::open("/dev/null", O_RDWR | O_CLOEXEC);
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
        for (const auto& arg : args)
            argv.push_back(const_cast<char*>(arg.c_str()));
        argv.push_back(nullptr);
        ::execv(argv[0], argv.data());
        _exit(127);
    }
    if (captureOutput) ::close(pipefd[1]);
    ProcessResult result;
    int status = 0;
    bool exited = false;
    bool eof = !captureOutput;
    bool failed = false;
    const auto limit = std::chrono::steady_clock::now() + maximum_;
    while (!exited || !eof) {
        if (!exited) {
            const pid_t waited = ::waitpid(pid, &status, WNOHANG);
            if (waited == pid) exited = true;
            else if (waited < 0 && errno != EINTR) { failed = true; break; }
        }
        if (captureOutput && !eof) {
            std::array<char, 512> buffer{};
            while (true) {
                const ssize_t count = ::read(pipefd[0], buffer.data(), buffer.size());
                if (count > 0) {
                    if (result.output.size() +
                        static_cast<std::size_t>(count) <= kMaxProcessOutput)
                        result.output.append(buffer.data(), static_cast<std::size_t>(count));
                    else failed = true;
                } else if (count == 0) {
                    eof = true;
                    break;
                } else if (errno == EINTR) continue;
                else if (errno == EAGAIN || errno == EWOULDBLOCK) break;
                else { failed = true; break; }
            }
        }
        if (failed || std::chrono::steady_clock::now() >= limit) {
            failed = true;
            break;
        }
        if (exited && eof) break;
        struct pollfd readFd{
            captureOutput && !eof ? pipefd[0] : -1, POLLIN, 0};
        (void)::poll(&readFd, 1, 25);
    }
    if (!exited) {
        (void)::kill(pid, SIGKILL);
        while (::waitpid(pid, &status, 0) < 0 && errno == EINTR) {}
    }
    if (captureOutput) ::close(pipefd[0]);
    result.exitCode = !failed && WIFEXITED(status) ? WEXITSTATUS(status) : -1;
    if (failed) result.output = "external media command failed or timed out";
    return result;
}

} // namespace vdrsuite::mediatools
