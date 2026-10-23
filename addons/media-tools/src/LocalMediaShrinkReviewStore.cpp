#include "LocalMediaShrinkReviewStore.h"
#include "MediaShrinkOperationSelection.h"

#include <array>
#include <cerrno>
#include <fcntl.h>
#include <limits>
#include <stdexcept>
#include <string>
#include <sys/random.h>
#include <sys/stat.h>
#include <unistd.h>
#include <utility>
#include <vector>

namespace vdrsuite::mediatools {
namespace {
struct Fd {
    int fd = -1;
    explicit Fd(int n = -1) : fd(n) {}
    ~Fd() { if (fd >= 0) ::close(fd); }
    Fd(const Fd&) = delete;
    Fd& operator=(const Fd&) = delete;
};

void require(bool ok, const char* detail) {
    if (!ok) throw std::runtime_error(detail);
}

bool identifier(const std::string& value) {
    if (value.empty() || value.size() > 160) return false;
    for (unsigned char c : value)
        if (!((c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') ||
              (c >= 'a' && c <= 'z') || c == '-' || c == '_'))
            return false;
    return true;
}

bool pathValid(const fs::path& p, bool absolute) {
    if (p.empty() || p.is_absolute() != absolute ||
        p.lexically_normal() != p) return false;
    for (const auto& part : p)
        if (part == ".." || part == ".") return false;
    return p.string().size() <= 2048;
}

bool noSymlinks(const fs::path& path) {
    fs::path prefix;
    for (const auto& component : path) {
        prefix /= component;
        struct stat s{};
        if (::lstat(prefix.c_str(), &s) != 0 || S_ISLNK(s.st_mode))
            return false;
    }
    return true;
}

bool safeDirectory(const fs::path& path) {
    if (!pathValid(path, true) || !noSymlinks(path)) return false;
    struct stat s{};
    return ::lstat(path.c_str(), &s) == 0 && S_ISDIR(s.st_mode) &&
        s.st_uid == ::geteuid() && (s.st_mode & 0077) == 0;
}

bool soundEvidence(const LocalMediaShrinkReview& e) {
    if (!shrinkOpaqueId(e.reviewId, "shk_") ||
        !shrinkOpaqueId(e.recordingId, "rec_") ||
        !identifier(e.actorId) || !identifier(e.backendId) ||
        !identifier(e.stageOperationId) ||
        e.sourceFingerprint.empty() || e.sourceFingerprint.size() > 512 ||
        e.sourceFingerprint.find_first_of("\r\n") != std::string::npos ||
        e.backendGeneration == 0 ||
        e.backendGeneration >
            static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max()) ||
        !pathValid(e.videoRoot, true) ||
        !pathValid(e.relativeRecording, false) ||
        e.relativeRecording.extension() != ".rec" ||
        e.relativeRecording.parent_path().empty() ||
        !pathValid(e.candidateDirectory, true) ||
        e.candidateDirectory.filename() != e.relativeRecording.filename() ||
        e.candidateDirectory.parent_path().parent_path() != e.videoRoot ||
        e.candidateDirectory.parent_path().filename().string().rfind(
            ".vdr-suite-shrink-", 0) != 0)
        return false;
    // A record is only accepted for the exact protected stage hierarchy.
    if (!noSymlinks(e.videoRoot) || !noSymlinks(e.candidateDirectory))
        return false;
    struct stat dir{}, file{};
    const auto stage = e.candidateDirectory.parent_path();
    return ::lstat(stage.c_str(), &dir) == 0 && S_ISDIR(dir.st_mode) &&
        dir.st_uid == ::geteuid() && (dir.st_mode & 0077) == 0 &&
        ::lstat(e.candidateDirectory.c_str(), &file) == 0 &&
        S_ISDIR(file.st_mode) && file.st_uid == ::geteuid() &&
        (file.st_mode & 0077) == 0;
}

std::string randomId() {
    std::array<unsigned char, 16> bytes{};
    std::size_t done = 0;
    while (done < bytes.size()) {
        const ssize_t n = ::getrandom(bytes.data() + done,
                                      bytes.size() - done, 0);
        if (n < 0 && errno == EINTR) continue;
        require(n > 0, "cryptographic review identity unavailable");
        done += static_cast<std::size_t>(n);
    }
    constexpr char hex[] = "0123456789abcdef";
    std::string id = "shk_";
    for (unsigned char b : bytes) {
        id += hex[b >> 4];
        id += hex[b & 15];
    }
    return id;
}

void field(std::string& output, const std::string& value) {
    require(value.size() <= 2048, "local shrink evidence too large");
    output += std::to_string(value.size()) + "\n" + value;
}

std::string serialize(const LocalMediaShrinkReview& e) {
    std::string result = "VDR-SUITE-LOCAL-SHRINK/1\n";
    for (const auto& item : {
        e.reviewId, e.actorId, e.backendId, e.recordingId,
        e.stageOperationId, e.sourceFingerprint,
        std::to_string(e.backendGeneration), e.videoRoot.string(),
        e.relativeRecording.string(), e.candidateDirectory.string()
    }) field(result, item);
    require(result.size() <= 8192, "local shrink evidence exceeds bound");
    return result;
}

bool decodeField(const std::string& bytes, std::size_t& offset,
                 std::string& output) {
    const auto newline = bytes.find('\n', offset);
    if (newline == std::string::npos || newline == offset ||
        newline - offset > 4) return false;
    std::size_t length = 0;
    for (std::size_t i = offset; i < newline; ++i) {
        const auto c = bytes[i];
        if (c < '0' || c > '9') return false;
        length = length * 10 + static_cast<std::size_t>(c - '0');
    }
    offset = newline + 1;
    if (length > 2048 || length > bytes.size() - offset) return false;
    output.assign(bytes, offset, length);
    offset += length;
    return true;
}

std::optional<LocalMediaShrinkReview> deserialize(const std::string& bytes) {
    constexpr const char* prefix = "VDR-SUITE-LOCAL-SHRINK/1\n";
    if (bytes.compare(0, std::char_traits<char>::length(prefix),
                      prefix) != 0) return std::nullopt;
    std::size_t offset = std::char_traits<char>::length(prefix);
    std::array<std::string, 10> values;
    for (auto& value : values)
        if (!decodeField(bytes, offset, value)) return std::nullopt;
    if (offset != bytes.size() || values[6].empty() ||
        values[6].size() > 19) return std::nullopt;
    std::uint64_t generation = 0;
    for (const char c : values[6]) {
        if (c < '0' || c > '9') return std::nullopt;
        const auto digit = static_cast<std::uint64_t>(c - '0');
        if (generation > (UINT64_MAX - digit) / 10) return std::nullopt;
        generation = generation * 10 + digit;
    }
    LocalMediaShrinkReview e{
        values[0],values[1],values[2],values[3],values[4],values[5],
        generation,values[7],values[8],values[9]
    };
    return soundEvidence(e) ? std::optional<LocalMediaShrinkReview>(e)
                            : std::nullopt;
}

bool writeFully(int fd, const std::string& bytes) {
    std::size_t offset = 0;
    while (offset < bytes.size()) {
        const auto n = ::write(fd, bytes.data() + offset,
                               bytes.size() - offset);
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) return false;
        offset += static_cast<std::size_t>(n);
    }
    return true;
}
} // namespace

LocalMediaShrinkReviewStore::LocalMediaShrinkReviewStore(fs::path privateDirectory)
    : directory_(std::move(privateDirectory)) {
    require(safeDirectory(directory_), "unsafe Agent shrink review directory");
}

std::string LocalMediaShrinkReviewStore::registerCandidate(
    const LocalMediaShrinkReview& incoming) {
    require(safeDirectory(directory_), "Agent shrink review directory changed");
    LocalMediaShrinkReview review = incoming;
    review.reviewId = randomId();
    require(soundEvidence(review), "invalid local HEVC candidate evidence");
    const std::string bytes = serialize(review);
    Fd dir(::open(directory_.c_str(),
                  O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW));
    require(dir.fd >= 0, "cannot pin shrink state directory");
    Fd output(::openat(dir.fd, review.reviewId.c_str(),
        O_WRONLY | O_CREAT | O_EXCL | O_NOFOLLOW | O_CLOEXEC, 0600));
    require(output.fd >= 0, "cannot reserve HEVC candidate identity");
    require(writeFully(output.fd, bytes) && ::fsync(output.fd) == 0 &&
            ::fsync(dir.fd) == 0,
            "cannot durably register HEVC candidate");
    return review.reviewId;
}

std::optional<LocalMediaShrinkReview> LocalMediaShrinkReviewStore::find(
    const std::string& reviewId) const {
    if (!shrinkOpaqueId(reviewId, "shk_") || !safeDirectory(directory_))
        return std::nullopt;
    Fd dir(::open(directory_.c_str(),
                  O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW));
    if (dir.fd < 0) return std::nullopt;
    Fd input(::openat(dir.fd, reviewId.c_str(), O_RDONLY | O_NOFOLLOW |
                       O_CLOEXEC | O_NONBLOCK));
    struct stat state{};
    if (input.fd < 0 || ::fstat(input.fd, &state) != 0 ||
        !S_ISREG(state.st_mode) || state.st_uid != ::geteuid() ||
        (state.st_mode & 0077) != 0 ||
        state.st_size <= 0 || state.st_size > 8192)
        return std::nullopt;
    std::string bytes(static_cast<std::size_t>(state.st_size), '\0');
    std::size_t offset = 0;
    while (offset < bytes.size()) {
        const auto n = ::read(input.fd, &bytes[offset], bytes.size() - offset);
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) return std::nullopt;
        offset += static_cast<std::size_t>(n);
    }
    struct stat after{};
    if (::fstat(input.fd, &after) != 0 ||
        state.st_dev != after.st_dev || state.st_ino != after.st_ino ||
        state.st_size != after.st_size ||
        state.st_mtim.tv_sec != after.st_mtim.tv_sec ||
        state.st_mtim.tv_nsec != after.st_mtim.tv_nsec)
        return std::nullopt;
    auto e = deserialize(bytes);
    if (!e || e->reviewId != reviewId) return std::nullopt;
    return e;
}

bool LocalMediaShrinkReviewStore::claimOnce(
    const std::string& reviewId, const std::string& publishOperationId) {
    if (!identifier(publishOperationId) || !find(reviewId) ||
        !safeDirectory(directory_)) return false;
    Fd dir(::open(directory_.c_str(),
                  O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW));
    if (dir.fd < 0) return false;
    const auto name = reviewId + ".publishing";
    Fd output(::openat(dir.fd, name.c_str(),
        O_WRONLY | O_CREAT | O_EXCL | O_NOFOLLOW | O_CLOEXEC, 0600));
    if (output.fd < 0) return false;
    const auto contents = publishOperationId + "\n";
    return writeFully(output.fd, contents) && ::fsync(output.fd) == 0 &&
           ::fsync(dir.fd) == 0;
}
} // namespace vdrsuite::mediatools
