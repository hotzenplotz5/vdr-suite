#include "EpgSeriesArtworkFallbackOrphanCleaner.h"

#include "EpgSeriesArtworkFallbackRepository.h"

#include <algorithm>
#include <cerrno>
#include <dirent.h>
#include <filesystem>
#include <fcntl.h>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <utility>
#include <vector>

namespace
{
class FileDescriptor
{
public:
    FileDescriptor() = default;

    explicit FileDescriptor(int value)
        : value_(value)
    {
    }

    FileDescriptor(const FileDescriptor&) = delete;
    FileDescriptor& operator=(const FileDescriptor&) = delete;

    FileDescriptor(FileDescriptor&& other) noexcept
        : value_(other.release())
    {
    }

    FileDescriptor& operator=(FileDescriptor&& other) noexcept
    {
        if (this != &other)
        {
            reset(other.release());
        }
        return *this;
    }

    ~FileDescriptor()
    {
        reset();
    }

    int get() const
    {
        return value_;
    }

    bool valid() const
    {
        return value_ >= 0;
    }

    int release()
    {
        const int value = value_;
        value_ = -1;
        return value;
    }

    void reset(int value = -1)
    {
        if (value_ >= 0)
        {
            ::close(value_);
        }
        value_ = value;
    }

private:
    int value_ = -1;
};

bool safeManagedRoot(
    const std::string& configured,
    std::filesystem::path& root)
{
    const std::filesystem::path candidate(configured);
    const std::filesystem::path normalized = candidate.lexically_normal();
    if (candidate.empty() ||
        !candidate.is_absolute() ||
        candidate != normalized ||
        normalized == normalized.root_path())
    {
        return false;
    }

    for (const auto& component : candidate)
    {
        if (component == "." || component == "..")
        {
            return false;
        }
    }

    root = normalized;
    return true;
}

bool validHexComponent(
    const std::string& value,
    std::size_t maximumLength)
{
    if (value.empty() ||
        value.size() > maximumLength ||
        value.size() % 2U != 0U)
    {
        return false;
    }

    return std::all_of(
        value.begin(),
        value.end(),
        [](unsigned char character)
        {
            return (character >= '0' && character <= '9') ||
                (character >= 'a' && character <= 'f');
        });
}

FileDescriptor openDirectoryAt(
    int parent,
    const std::string& name)
{
    if (name.empty() || name == "." || name == "..")
    {
        return FileDescriptor();
    }

    return FileDescriptor(::openat(
        parent,
        name.c_str(),
        O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW));
}

bool listDirectoryNames(
    int directoryFd,
    std::vector<std::string>& names)
{
    names.clear();
    FileDescriptor duplicate(::dup(directoryFd));
    if (!duplicate.valid())
    {
        return false;
    }

    DIR* directory = ::fdopendir(duplicate.release());
    if (directory == nullptr)
    {
        return false;
    }

    errno = 0;
    while (dirent* entry = ::readdir(directory))
    {
        const std::string name(entry->d_name);
        if (name == "." || name == "..")
        {
            continue;
        }
        names.push_back(name);
    }
    const bool listed = errno == 0;
    ::closedir(directory);
    std::sort(names.begin(), names.end());
    return listed;
}

bool oldEnough(
    const struct stat& status,
    std::int64_t nowEpochSeconds,
    std::int64_t minimumAgeSeconds)
{
    const std::int64_t modifiedAt =
        static_cast<std::int64_t>(status.st_mtime);
    return modifiedAt >= 0 &&
        modifiedAt <= nowEpochSeconds &&
        nowEpochSeconds - modifiedAt >= minimumAgeSeconds;
}

bool sameFileIdentity(
    const struct stat& first,
    const struct stat& second)
{
    return first.st_dev == second.st_dev &&
        first.st_ino == second.st_ino &&
        first.st_mode == second.st_mode &&
        first.st_size == second.st_size &&
        first.st_mtim.tv_sec == second.st_mtim.tv_sec &&
        first.st_mtim.tv_nsec == second.st_mtim.tv_nsec;
}

struct CleanupTraversal
{
    EpgSeriesArtworkFallbackRepository& repository;
    const EpgSeriesArtworkFallbackOrphanCleanupConfig& config;
    const std::filesystem::path& root;
    std::int64_t nowEpochSeconds;
    EpgSeriesArtworkFallbackOrphanCleanupResult& result;

    bool limitReached() const
    {
        return result.removedFiles >= config.maximumFilesPerRun;
    }

    void cleanupCandidate(
        int eventDirectory,
        const std::string& backendComponent,
        const std::string& channelComponent,
        const std::string& eventComponent,
        const std::string& filename)
    {
        if (limitReached())
        {
            result.limitReached = true;
            return;
        }

        errno = 0;
        FileDescriptor candidate(::openat(
            eventDirectory,
            filename.c_str(),
            O_RDONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK));
        if (!candidate.valid())
        {
            if (errno != ENOENT)
            {
                if (errno == ELOOP || errno == ENOTDIR)
                {
                    ++result.skippedUnsafeEntries;
                }
                else
                {
                    ++result.errors;
                }
            }
            return;
        }

        struct stat openedStatus {};
        if (::fstat(candidate.get(), &openedStatus) != 0 ||
            !S_ISREG(openedStatus.st_mode))
        {
            ++result.skippedUnsafeEntries;
            return;
        }

        ++result.examinedFiles;
        if (!oldEnough(
                openedStatus,
                nowEpochSeconds,
                config.minimumAgeSeconds))
        {
            ++result.youngFiles;
            return;
        }

        const std::string path = (
            root /
            backendComponent /
            channelComponent /
            eventComponent /
            filename).string();
        const EpgSeriesArtworkFallbackPathReferenceState referenceState =
            repository.referenceStateForPath(path);
        if (referenceState ==
            EpgSeriesArtworkFallbackPathReferenceState::Error)
        {
            ++result.errors;
            return;
        }
        if (referenceState ==
            EpgSeriesArtworkFallbackPathReferenceState::Referenced)
        {
            ++result.referencedFiles;
            return;
        }

        struct stat currentStatus {};
        if (::fstatat(
                eventDirectory,
                filename.c_str(),
                &currentStatus,
                AT_SYMLINK_NOFOLLOW) != 0 ||
            !S_ISREG(currentStatus.st_mode) ||
            !sameFileIdentity(openedStatus, currentStatus))
        {
            ++result.skippedUnsafeEntries;
            return;
        }

        if (::unlinkat(eventDirectory, filename.c_str(), 0) != 0)
        {
            ++result.errors;
            return;
        }

        ++result.removedFiles;
        if (limitReached())
        {
            result.limitReached = true;
        }
    }
};
}

EpgSeriesArtworkFallbackOrphanCleaner::
EpgSeriesArtworkFallbackOrphanCleaner(
    EpgSeriesArtworkFallbackRepository& repository,
    EpgSeriesArtworkFallbackOrphanCleanupConfig config)
    : repository_(repository),
      config_(std::move(config))
{
}

EpgSeriesArtworkFallbackOrphanCleanupResult
EpgSeriesArtworkFallbackOrphanCleaner::cleanup(
    std::int64_t nowEpochSeconds)
{
    EpgSeriesArtworkFallbackOrphanCleanupResult result;
    if (!config_.enabled)
    {
        return result;
    }

    result.attempted = true;
    if (!config_.valid() || nowEpochSeconds <= 0)
    {
        ++result.errors;
        return result;
    }

    std::filesystem::path root;
    if (!safeManagedRoot(config_.cacheRoot, root))
    {
        ++result.errors;
        return result;
    }

    errno = 0;
    FileDescriptor rootDirectory(::open(
        root.c_str(),
        O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW));
    if (!rootDirectory.valid())
    {
        if (errno != ENOENT)
        {
            ++result.errors;
        }
        return result;
    }
    result.rootAvailable = true;

    CleanupTraversal traversal{
        repository_,
        config_,
        root,
        nowEpochSeconds,
        result};

    std::vector<std::string> backendNames;
    if (!listDirectoryNames(rootDirectory.get(), backendNames))
    {
        ++result.errors;
        return result;
    }

    for (const std::string& backendName : backendNames)
    {
        if (traversal.limitReached())
        {
            break;
        }
        if (!validHexComponent(backendName, 256U))
        {
            continue;
        }

        FileDescriptor backendDirectory = openDirectoryAt(
            rootDirectory.get(),
            backendName);
        if (!backendDirectory.valid())
        {
            ++result.skippedUnsafeEntries;
            continue;
        }

        std::vector<std::string> channelNames;
        if (!listDirectoryNames(backendDirectory.get(), channelNames))
        {
            ++result.errors;
            continue;
        }

        for (const std::string& channelName : channelNames)
        {
            if (traversal.limitReached())
            {
                break;
            }
            if (!validHexComponent(channelName, 512U))
            {
                continue;
            }

            FileDescriptor channelDirectory = openDirectoryAt(
                backendDirectory.get(),
                channelName);
            if (!channelDirectory.valid())
            {
                ++result.skippedUnsafeEntries;
                continue;
            }

            std::vector<std::string> eventNames;
            if (!listDirectoryNames(channelDirectory.get(), eventNames))
            {
                ++result.errors;
                continue;
            }

            for (const std::string& eventName : eventNames)
            {
                if (traversal.limitReached())
                {
                    break;
                }
                if (!validHexComponent(eventName, 512U))
                {
                    continue;
                }

                FileDescriptor eventDirectory = openDirectoryAt(
                    channelDirectory.get(),
                    eventName);
                if (!eventDirectory.valid())
                {
                    ++result.skippedUnsafeEntries;
                    continue;
                }

                traversal.cleanupCandidate(
                    eventDirectory.get(),
                    backendName,
                    channelName,
                    eventName,
                    "series.png");
                traversal.cleanupCandidate(
                    eventDirectory.get(),
                    backendName,
                    channelName,
                    eventName,
                    "series.jpg");

                eventDirectory.reset();
                ::unlinkat(
                    channelDirectory.get(),
                    eventName.c_str(),
                    AT_REMOVEDIR);
            }

            channelDirectory.reset();
            ::unlinkat(
                backendDirectory.get(),
                channelName.c_str(),
                AT_REMOVEDIR);
        }

        backendDirectory.reset();
        ::unlinkat(
            rootDirectory.get(),
            backendName.c_str(),
            AT_REMOVEDIR);
    }

    return result;
}
