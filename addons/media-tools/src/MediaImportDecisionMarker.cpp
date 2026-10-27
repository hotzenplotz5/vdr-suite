#include "MediaImportDecisionMarker.h"

#include <algorithm>
#include <cerrno>
#include <cstdint>
#include <fcntl.h>
#include <filesystem>
#include <string>
#include <utility>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <linux/fs.h>
#include <unistd.h>
#include <vector>

namespace vdrsuite::mediatools {
namespace {
class Fd {
public:
    explicit Fd(int value = -1) : value_(value) {}
    ~Fd() { if (value_ >= 0) ::close(value_); }
    Fd(const Fd&) = delete;
    Fd& operator=(const Fd&) = delete;
    Fd(Fd&& other) noexcept : value_(other.value_) { other.value_ = -1; }
    Fd& operator=(Fd&& other) noexcept {
        if (this != &other) {
            if (value_ >= 0) ::close(value_);
            value_ = other.value_;
            other.value_ = -1;
        }
        return *this;
    }
    int get() const { return value_; }
private:
    int value_;
};

bool safeAbsolute(const std::filesystem::path& path)
{
    if (!path.is_absolute() || path.lexically_normal() != path ||
        path.string().size() > 4096 || path.string().find('\0') != std::string::npos)
        return false;
    for (const auto& p : path)
        if (p == "." || p == "..") return false;
    return true;
}

bool insideRoot(const std::filesystem::path& root,
                const std::filesystem::path& child)
{
    if (!safeAbsolute(root) || !safeAbsolute(child) || root == child)
        return false;
    const auto relative = child.lexically_relative(root);
    if (relative.empty() || relative == "." || relative.is_absolute())
        return false;
    for (const auto& part : relative)
        if (part == ".." || part == ".") return false;
    return true;
}

Fd openDirectoryWithoutSymlinks(const std::filesystem::path& absolute)
{
    Fd current(::open("/", O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW));
    if (current.get() < 0 || !safeAbsolute(absolute)) return Fd{};
    for (const auto& part : absolute.relative_path()) {
        if (part.empty()) continue;
        Fd next(::openat(current.get(), part.c_str(),
                         O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW));
        if (next.get() < 0) return Fd{};
        current = std::move(next);
    }
    return current;
}

bool sameIdentity(const struct stat& st, const LocalMediaImportSource& src)
{
    return S_ISREG(st.st_mode) && st.st_nlink == 1 &&
        static_cast<std::uint64_t>(st.st_dev) == src.sourceDevice &&
        static_cast<std::uint64_t>(st.st_ino) == src.sourceInode &&
        st.st_size >= 0 &&
        static_cast<std::uintmax_t>(st.st_size) == src.sourceBytes &&
        st.st_mtim.tv_sec == src.sourceMtimeSeconds &&
        st.st_mtim.tv_nsec == src.sourceMtimeNanoseconds;
}

bool safeExtension(const std::string& extension)
{
    return extension == ".mkv" || extension == ".mp4" ||
        extension == ".ts" || extension == ".mov" ||
        extension == ".avi" || extension == ".webm" ||
        extension == ".m4v";
}

bool markedBefore(const std::string& stem)
{
    const auto has = [&](const char* marker) {
        const std::string text(marker);
        return stem.size() >= text.size() &&
            stem.compare(stem.size() - text.size(), text.size(), text) == 0;
    };
    return has(".pc_encode") || has(".skipped") || has(".duplicate");
}
} // namespace

MediaImportMarkerOutcome markMediaImportSource(
    const LocalMediaImportSource& source,
    MediaImportMarkerDecision decision)
{
    namespace fs = std::filesystem;
    if (!insideRoot(source.sourceRoot, source.sourceFile) ||
        !safeAbsolute(source.videoRoot) ||
        !safeAbsolute(source.sourceFile.parent_path()) ||
        source.sourceDevice == 0 || source.sourceInode == 0 ||
        source.sourceBytes == 0 ||
        (decision != MediaImportMarkerDecision::desktop &&
         decision != MediaImportMarkerDecision::skip &&
         decision != MediaImportMarkerDecision::duplicate &&
         decision != MediaImportMarkerDecision::release))
        return MediaImportMarkerOutcome::rejected;

    const fs::path filename = source.sourceFile.filename();
    const std::string beforeName = filename.string();
    std::string extension = filename.extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(),
        [](unsigned char ch) {
            return static_cast<char>(ch >= 'A' && ch <= 'Z' ? ch + 32 : ch);
        });
    std::string stem = filename.stem().string();
    std::transform(stem.begin(), stem.end(), stem.begin(),
        [](unsigned char ch) {
            return static_cast<char>(ch >= 'A' && ch <= 'Z' ? ch + 32 : ch);
        });
    const bool release = decision == MediaImportMarkerDecision::release;
    const std::string originalStem = filename.stem().string();
    std::string matchedMarker;
    for (const char* candidate : {".pc_encode", ".skipped", ".duplicate"}) {
        const std::string marker(candidate);
        if (stem.size() >= marker.size() &&
            stem.compare(stem.size() - marker.size(), marker.size(), marker) == 0) {
            matchedMarker = marker;
            break;
        }
    }
    if (!safeExtension(extension) || beforeName.empty() ||
        beforeName.front() == '.' ||
        beforeName.find_first_of("\r\n\t") != std::string::npos ||
        (release ? matchedMarker.empty() : !matchedMarker.empty()))
        return MediaImportMarkerOutcome::rejected;
    const std::string baseStem = release
        ? originalStem.substr(0, originalStem.size() - matchedMarker.size())
        : originalStem;
    std::string checkedStem = baseStem;
    std::transform(checkedStem.begin(), checkedStem.end(), checkedStem.begin(),
        [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    if (baseStem.empty() || baseStem.front() == '.' ||
        markedBefore(checkedStem))
        return MediaImportMarkerOutcome::rejected;
    const std::string suffix = release ? "" :
        decision == MediaImportMarkerDecision::desktop ? ".pc_encode" :
        decision == MediaImportMarkerDecision::duplicate ? ".duplicate" : ".skipped";
    const std::string afterName =
        baseStem + suffix + filename.extension().string();
    if (afterName.size() > 255 || afterName == beforeName)
        return MediaImportMarkerOutcome::rejected;

    // Open every ancestor with O_NOFOLLOW. The source is fixed by dev/inode,
    // size and mtime from the independently resolved backend inventory.
    Fd directory = openDirectoryWithoutSymlinks(source.sourceFile.parent_path());
    if (directory.get() < 0)
        return MediaImportMarkerOutcome::rejected;
    Fd input(::openat(directory.get(), beforeName.c_str(),
        O_RDONLY | O_NONBLOCK | O_CLOEXEC | O_NOFOLLOW));
    struct stat first{}, fresh{};
    if (input.get() < 0 || ::fstat(input.get(), &first) != 0 ||
        !sameIdentity(first, source) ||
        ::flock(input.get(), LOCK_EX | LOCK_NB) != 0 ||
        ::fstatat(directory.get(), beforeName.c_str(), &fresh,
                  AT_SYMLINK_NOFOLLOW) != 0 ||
        !sameIdentity(fresh, source))
        return MediaImportMarkerOutcome::rejected;

    // The target is in the SAME directory. RENAME_NOREPLACE means neither a
    // pre-existing skipped item nor a symlink nor a duplicate is overwritten.
    const long renamed = ::syscall(SYS_renameat2,
        directory.get(), beforeName.c_str(), directory.get(),
        afterName.c_str(), RENAME_NOREPLACE);
    if (renamed != 0)
        // Interruption / filesystem I/O can make the exact final state
        // unknowable. Never treat those as permission for a blind retry.
        return errno == EINTR || errno == EIO
            ? MediaImportMarkerOutcome::outcomeUnknown
            : MediaImportMarkerOutcome::rejected;

    // A rename is the sole intentional mutation, never a delete or remux.
    // After the rename there is NO safe blind retry: an unexpected file
    // mutation or fsync error is an uncertain outcome requiring readback.
    if (::fstatat(directory.get(), afterName.c_str(), &fresh,
                  AT_SYMLINK_NOFOLLOW) != 0 ||
        !sameIdentity(fresh, source) ||
        ::fsync(directory.get()) != 0)
        return MediaImportMarkerOutcome::outcomeUnknown;
    return MediaImportMarkerOutcome::marked;
}
} // namespace vdrsuite::mediatools
