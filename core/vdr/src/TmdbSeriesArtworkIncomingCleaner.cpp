#include "TmdbSeriesArtworkIncomingCleaner.h"

#include <algorithm>
#include <cerrno>
#include <climits>
#include <cstdlib>
#include <dirent.h>
#include <fcntl.h>
#include <filesystem>
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
    explicit FileDescriptor(int descriptor) : descriptor_(descriptor) {}
    ~FileDescriptor() { reset(); }

    FileDescriptor(const FileDescriptor&) = delete;
    FileDescriptor& operator=(const FileDescriptor&) = delete;

    FileDescriptor(FileDescriptor&& other) noexcept
        : descriptor_(other.descriptor_)
    {
        other.descriptor_ = -1;
    }

    FileDescriptor& operator=(FileDescriptor&& other) noexcept
    {
        if (this != &other)
        {
            reset();
            descriptor_ = other.descriptor_;
            other.descriptor_ = -1;
        }
        return *this;
    }

    bool valid() const { return descriptor_ >= 0; }
    int get() const { return descriptor_; }

    void reset(int descriptor = -1)
    {
        if (descriptor_ >= 0)
        {
            ::close(descriptor_);
        }
        descriptor_ = descriptor;
    }

private:
    int descriptor_ = -1;
};

enum class RootOpenStatus
{
    Opened,
    Missing,
    Error
};

enum class IncomingFileKind
{
    None,
    Candidate,
    Temporary
};

bool decimalDigits(const std::string& value)
{
    return !value.empty() &&
        std::all_of(
            value.begin(),
            value.end(),
            [](unsigned char character)
            {
                return character >= '0' && character <= '9';
            });
}

bool positiveSeriesId(const std::string& value)
{
    if (value.empty() || value.size() > 10U || value.front() == '0' ||
        !decimalDigits(value))
    {
        return false;
    }

    errno = 0;
    char* end = nullptr;
    const long parsed = std::strtol(value.c_str(), &end, 10);
    return errno == 0 && end != value.c_str() && end != nullptr &&
        *end == '\0' && parsed > 0 && parsed <= INT_MAX;
}

bool lowercaseHex16(const std::string& value)
{
    return value.size() == 16U &&
        std::all_of(
            value.begin(),
            value.end(),
            [](unsigned char character)
            {
                return (character >= '0' && character <= '9') ||
                    (character >= 'a' && character <= 'f');
            });
}

bool finalCandidateName(const std::string& name)
{
    static const std::string Prefix = "tmdb-";
    static const std::string Suffix = ".candidate";

    if (name.size() <= Prefix.size() + Suffix.size() ||
        name.compare(0, Prefix.size(), Prefix) != 0 ||
        name.compare(name.size() - Suffix.size(), Suffix.size(), Suffix) != 0)
    {
        return false;
    }

    const std::size_t separator = name.find('-', Prefix.size());
    const std::size_t suffixPosition = name.size() - Suffix.size();
    if (separator == std::string::npos || separator >= suffixPosition)
    {
        return false;
    }

    const std::string seriesId = name.substr(
        Prefix.size(),
        separator - Prefix.size());
    const std::string hash = name.substr(
        separator + 1U,
        suffixPosition - separator - 1U);
    return positiveSeriesId(seriesId) && lowercaseHex16(hash);
}

bool positivePid(const std::string& value)
{
    return positiveSeriesId(value);
}

bool sequenceNumber(const std::string& value)
{
    return !value.empty() && value.size() <= 20U && decimalDigits(value);
}

IncomingFileKind incomingFileKind(const std::string& name)
{
    if (finalCandidateName(name))
    {
        return IncomingFileKind::Candidate;
    }

    static const std::string TemporarySuffix = ".tmp";
    if (name.size() <= 1U + TemporarySuffix.size() || name.front() != '.' ||
        name.compare(
            name.size() - TemporarySuffix.size(),
            TemporarySuffix.size(),
            TemporarySuffix) != 0)
    {
        return IncomingFileKind::None;
    }

    const std::string body = name.substr(
        1U,
        name.size() - 1U - TemporarySuffix.size());
    const std::size_t counterSeparator = body.rfind('.');
    if (counterSeparator == std::string::npos || counterSeparator == 0U)
    {
        return IncomingFileKind::None;
    }

    const std::size_t pidSeparator = body.rfind('.', counterSeparator - 1U);
    if (pidSeparator == std::string::npos || pidSeparator == 0U)
    {
        return IncomingFileKind::None;
    }

    const std::string candidate = body.substr(0U, pidSeparator);
    const std::string pid = body.substr(
        pidSeparator + 1U,
        counterSeparator - pidSeparator - 1U);
    const std::string sequence = body.substr(counterSeparator + 1U);
    return finalCandidateName(candidate) && positivePid(pid) &&
            sequenceNumber(sequence)
        ? IncomingFileKind::Temporary
        : IncomingFileKind::None;
}

RootOpenStatus openRootNoFollow(
    const std::string& configuredRoot,
    FileDescriptor& root)
{
    root.reset();
    const std::filesystem::path path(configuredRoot);
    if (path.empty() || !path.is_absolute() || path == path.root_path() ||
        path.lexically_normal() != path)
    {
        return RootOpenStatus::Error;
    }

    FileDescriptor current(::open(
        "/",
        O_RDONLY | O_DIRECTORY | O_CLOEXEC));
    if (!current.valid())
    {
        return RootOpenStatus::Error;
    }

    for (const auto& componentPath : path.relative_path())
    {
        const std::string component = componentPath.string();
        if (component.empty() || component == "." || component == "..")
        {
            return RootOpenStatus::Error;
        }

        const int child = ::openat(
            current.get(),
            component.c_str(),
            O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
        if (child < 0)
        {
            return errno == ENOENT
                ? RootOpenStatus::Missing
                : RootOpenStatus::Error;
        }
        current.reset(child);
    }

    root = std::move(current);
    return RootOpenStatus::Opened;
}

bool listNames(
    int root,
    std::vector<std::string>& names)
{
    names.clear();
    const int duplicate = ::dup(root);
    if (duplicate < 0)
    {
        return false;
    }

    DIR* directory = ::fdopendir(duplicate);
    if (directory == nullptr)
    {
        ::close(duplicate);
        return false;
    }

    errno = 0;
    while (dirent* entry = ::readdir(directory))
    {
        const std::string name(entry->d_name);
        if (name != "." && name != "..")
        {
            names.push_back(name);
        }
        errno = 0;
    }
    const bool listed = errno == 0;
    ::closedir(directory);
    if (listed)
    {
        std::sort(names.begin(), names.end());
    }
    return listed;
}

bool oldEnough(
    const struct stat& status,
    std::int64_t nowEpochSeconds,
    std::int64_t minimumAgeSeconds)
{
    if (status.st_mtim.tv_sec < 0 ||
        static_cast<std::int64_t>(status.st_mtim.tv_sec) > nowEpochSeconds)
    {
        return false;
    }

    const std::int64_t age = nowEpochSeconds -
        static_cast<std::int64_t>(status.st_mtim.tv_sec);
    return age > minimumAgeSeconds ||
        (age == minimumAgeSeconds && status.st_mtim.tv_nsec == 0);
}

bool sameIdentity(
    const struct stat& opened,
    const struct stat& current)
{
    return opened.st_dev == current.st_dev &&
        opened.st_ino == current.st_ino &&
        opened.st_mode == current.st_mode &&
        opened.st_size == current.st_size &&
        opened.st_mtim.tv_sec == current.st_mtim.tv_sec &&
        opened.st_mtim.tv_nsec == current.st_mtim.tv_nsec;
}
}

bool TmdbSeriesArtworkIncomingCleanupConfig::valid() const
{
    const std::filesystem::path root(incomingRoot);
    return enabled && !incomingRoot.empty() && incomingRoot.size() <= 4096U &&
        root.is_absolute() && root != root.root_path() &&
        root.lexically_normal() == root &&
        minimumAgeSeconds > 0 && maximumFilesPerRun > 0U &&
        maximumFilesPerRun <= 1024U;
}

TmdbSeriesArtworkIncomingCleaner::TmdbSeriesArtworkIncomingCleaner(
    TmdbSeriesArtworkIncomingCleanupConfig config)
    : config_(std::move(config))
{
}

TmdbSeriesArtworkIncomingCleanupResult
TmdbSeriesArtworkIncomingCleaner::cleanup(
    std::int64_t nowEpochSeconds) const
{
    TmdbSeriesArtworkIncomingCleanupResult result;
    if (!config_.enabled)
    {
        return result;
    }

    result.attempted = true;
    if (!config_.valid() || nowEpochSeconds <= 0)
    {
        result.errors = 1U;
        return result;
    }

    FileDescriptor root;
    const RootOpenStatus rootStatus = openRootNoFollow(
        config_.incomingRoot,
        root);
    if (rootStatus == RootOpenStatus::Missing)
    {
        return result;
    }
    if (rootStatus != RootOpenStatus::Opened)
    {
        result.errors = 1U;
        return result;
    }
    result.rootAvailable = true;

    std::vector<std::string> names;
    if (!listNames(root.get(), names))
    {
        result.errors = 1U;
        return result;
    }

    for (const std::string& name : names)
    {
        if (result.removedFiles() >= config_.maximumFilesPerRun)
        {
            result.limitReached = true;
            break;
        }

        ++result.examinedEntries;
        const IncomingFileKind kind = incomingFileKind(name);
        if (kind == IncomingFileKind::None)
        {
            ++result.skippedForeignEntries;
            continue;
        }
        ++result.recognizedFiles;

        FileDescriptor file(::openat(
            root.get(),
            name.c_str(),
            O_RDONLY | O_NONBLOCK | O_CLOEXEC | O_NOFOLLOW));
        if (!file.valid())
        {
            if (errno == ELOOP || errno == ENOENT)
            {
                ++result.skippedUnsafeEntries;
            }
            else
            {
                ++result.errors;
            }
            continue;
        }

        struct stat opened {};
        if (::fstat(file.get(), &opened) != 0)
        {
            ++result.errors;
            continue;
        }
        if (!S_ISREG(opened.st_mode))
        {
            ++result.skippedUnsafeEntries;
            continue;
        }
        if (!oldEnough(
                opened,
                nowEpochSeconds,
                config_.minimumAgeSeconds))
        {
            ++result.youngFiles;
            continue;
        }

        struct stat current {};
        if (::fstatat(
                root.get(),
                name.c_str(),
                &current,
                AT_SYMLINK_NOFOLLOW) != 0)
        {
            if (errno == ENOENT)
            {
                ++result.skippedUnsafeEntries;
            }
            else
            {
                ++result.errors;
            }
            continue;
        }
        if (!S_ISREG(current.st_mode) || !sameIdentity(opened, current))
        {
            ++result.skippedUnsafeEntries;
            continue;
        }

        if (::unlinkat(root.get(), name.c_str(), 0) != 0)
        {
            ++result.errors;
            continue;
        }

        if (kind == IncomingFileKind::Candidate)
        {
            ++result.removedCandidateFiles;
        }
        else
        {
            ++result.removedTemporaryFiles;
        }
    }

    return result;
}
