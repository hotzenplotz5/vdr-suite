#pragma once

#include "AddonInstalledManifest.h"

#include <cerrno>
#include <fcntl.h>
#include <optional>
#include <string>
#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>
#include <utility>
#include <vector>

namespace vdrsuite::mediatools {

// Persistent, root-owned, fixed inode in the same protected directory as
// the administrator's approval. Source/manual and eventual Debian installs
// must explicitly stage this empty 0644 file before enabling an executor.
// NEVER create it from an unprivileged Agent or a package hook during use.
inline constexpr const char* kMediaImportRunLockName =
    "rectools.import-running.lock";

// An advisory kernel lock *held across the handler's exec* is necessary
// to prove that an Agent crash did not orphan an actively writing worker.
// A future package maintainer must also acquire AND HOLD the exclusive lock
// throughout the actual replacement, after global freeze, full operations
// drain, effective grants and independent provenance checks. A successful
// exclusive try-lock by itself does NOT authorize installing or removing.
class MediaToolsImportRunLock final {
public:
    enum class Mode { sharedExecution, exclusiveMaintenance };

    ~MediaToolsImportRunLock()
    {
        if (fd_ >= 0) ::close(fd_);
    }
    MediaToolsImportRunLock(const MediaToolsImportRunLock&) = delete;
    MediaToolsImportRunLock& operator=(const MediaToolsImportRunLock&) = delete;
    MediaToolsImportRunLock(MediaToolsImportRunLock&& other) noexcept
        : fd_(std::exchange(other.fd_, -1)), mode_(other.mode_) {}
    MediaToolsImportRunLock& operator=(MediaToolsImportRunLock&& other) noexcept
    {
        if (this != &other) {
            if (fd_ >= 0) ::close(fd_);
            fd_ = std::exchange(other.fd_, -1);
            mode_ = other.mode_;
        }
        return *this;
    }

    bool ownsExclusiveLock() const noexcept
    {
        return fd_ >= 0 && mode_ == Mode::exclusiveMaintenance;
    }

    // A borrowed descriptor, valid only while THIS kernel-lock owner lives.
    // The approved Agent may bind it to its fixed NativeProcessRunner;
    // untrusted module requests never supply or choose it.
    int borrowedDescriptorForChild() const noexcept { return fd_; }

    static std::optional<MediaToolsImportRunLock> tryAcquire(
        const std::string& trustedRoot, Mode mode)
    {
        using namespace vdrsuite::addons::installed_manifest_detail;
        std::vector<std::string> parts;
        if (!components(trustedRoot, parts)) return std::nullopt;
        for (const char* p : {"etc", "vdr-suite", "addon-approvals"})
            parts.emplace_back(p);
        Fd directory(::open("/", O_RDONLY | O_DIRECTORY |
                                  O_NOFOLLOW | O_CLOEXEC));
        if (directory.fd < 0) return std::nullopt;
        for (const auto& part : parts) {
            if (!safeDirectory(directory.fd)) return std::nullopt;
            Fd next(::openat(directory.fd, part.c_str(),
                             O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC));
            if (next.fd < 0) return std::nullopt;
            directory = std::move(next);
        }
        if (!safeDirectory(directory.fd)) return std::nullopt;
        int file = ::openat(directory.fd, kMediaImportRunLockName,
                            O_RDONLY | O_NOFOLLOW | O_NONBLOCK | O_CLOEXEC);
        if (file < 0) return std::nullopt;
        Fd guard(file);
        struct stat before{}, after{}, named{};
        if (::fstat(file, &before) != 0 ||
            !S_ISREG(before.st_mode) || before.st_uid != 0 ||
            (before.st_mode & 0022) != 0 ||
            before.st_nlink != 1 || before.st_size != 0)
            return std::nullopt;

        if (::flock(file, (mode == Mode::sharedExecution
                              ? LOCK_SH : LOCK_EX) | LOCK_NB) != 0)
            return std::nullopt;
        if (::fstat(file, &after) != 0 ||
            ::fstatat(directory.fd, kMediaImportRunLockName, &named,
                      AT_SYMLINK_NOFOLLOW) != 0 ||
            !unchanged(before, after) ||
            before.st_dev != named.st_dev ||
            before.st_ino != named.st_ino ||
            !safeDirectory(directory.fd))
            return std::nullopt;
        return MediaToolsImportRunLock(std::exchange(guard.fd, -1), mode);
    }

    // Child-only: retain the shared open-file-description lock through
    // fexecve, and intentionally into any importer-spawned ffmpeg descendants.
    // Parent keeps FD_CLOEXEC so unrelated Agent execs cannot inherit it.
    bool inheritInExecChild() const
    {
        if (fd_ < 0) return false;
        const int flags = ::fcntl(fd_, F_GETFD);
        return flags >= 0 &&
            ::fcntl(fd_, F_SETFD, flags & ~FD_CLOEXEC) == 0;
    }

private:
    explicit MediaToolsImportRunLock(int fd, Mode mode)
        : fd_(fd), mode_(mode) {}
    int fd_ = -1;
    Mode mode_ = Mode::sharedExecution;
};

} // namespace vdrsuite::mediatools
