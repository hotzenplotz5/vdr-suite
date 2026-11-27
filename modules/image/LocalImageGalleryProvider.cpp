#include "LocalImageGalleryProvider.h"
#include "EpgArtworkPathPolicy.h"

#include <cerrno>
#include <dirent.h>
#include <fcntl.h>
#include <filesystem>
#include <memory>
#include <sys/stat.h>
#include <unistd.h>
#include <utility>

namespace vdrsuite::imagegallery {
namespace {
namespace fs = std::filesystem;

bool relativeDirectoryPath(const std::string& value)
{
    if (value.find('\0') != std::string::npos || fs::path(value).is_absolute())
        return false;
    if (value.empty()) return true;
    for (const auto& part : fs::path(value))
        if (part.empty() || part == "." || part == "..") return false;
    return true;
}

// Follow the existing artwork provider's descriptor-relative traversal
// pattern, after canonical resolution. A changed symlink cannot redirect an
// open outside the approved root. Configured roots may themselves be links.
int descend(int fd, const fs::path& path)
{
    if (fd < 0) return -1;
    if (path.empty() || path == ".") return fd;
    for (const auto& part : path) {
        if (part.empty() || part == "." || part == ".." || part == "/") {
            ::close(fd);
            return -1;
        }
        const int next = ::openat(fd, part.c_str(),
            O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
        ::close(fd);
        if (next < 0) return -1;
        fd = next;
    }
    return fd;
}

struct DirectoryCloser {
    void operator()(DIR* value) const { if (value) ::closedir(value); }
};

// Reuse the existing canonical root policy. Only use the resolved path for
// metadata; serving file bytes needs its own descriptor-based open.
bool regularInside(const fs::path& candidate, const std::string& root)
{
    std::string resolved;
    if (!EpgArtworkPathPolicy::resolveAllowedPath(candidate.string(), {root}, resolved))
        return false;
    std::error_code error;
    return fs::is_regular_file(resolved, error) && !error;
}
} // namespace

BrowseResult browseLocalDirectory(const Source& source,
                                  const std::string& relativeDirectory)
{
    if (!relativeDirectoryPath(relativeDirectory) ||
        !fs::path(source.root).is_absolute() ||
        source.root.find('\0') != std::string::npos) return {};
    std::error_code error;
    const auto root = fs::canonical(source.root, error);
    if (error) return {};
    std::string resolved;
    if (!EpgArtworkPathPolicy::resolveAllowedPath(
            (root / relativeDirectory).string(), {root.string()}, resolved)) return {};
    const auto relative = fs::path(resolved).lexically_relative(root);
    if (relative.empty()) return {};
    const int rootFd = descend(::open("/", O_RDONLY | O_DIRECTORY | O_CLOEXEC),
                               root.relative_path());
    const int fd = descend(rootFd, relative);
    if (fd < 0) return {};
    DIR* raw = ::fdopendir(fd);
    if (!raw) {
        ::close(fd);
        return {};
    }
    std::unique_ptr<DIR, DirectoryCloser> directory(raw);
    std::vector<Entry> entries;
    for (;;) {
        errno = 0;
        const auto* item = ::readdir(directory.get());
        if (!item) {
            if (errno != 0) return {};
            break;
        }
        const std::string name(item->d_name);
        if (name == "." || name == "..") continue;
        struct stat st {};
        if (::fstatat(::dirfd(directory.get()), name.c_str(), &st, AT_SYMLINK_NOFOLLOW) != 0)
            continue;
        const auto child = fs::path(relativeDirectory) / name;
        bool folder = S_ISDIR(st.st_mode);
        bool file = S_ISREG(st.st_mode);
        if (S_ISLNK(st.st_mode)) {
            std::string target;
            if (!EpgArtworkPathPolicy::resolveAllowedPath(
                    (root / child).string(), {root.string()}, target)) continue;
            error.clear();
            const auto status = fs::status(target, error);
            if (error) continue;
            folder = fs::is_directory(status);
            file = fs::is_regular_file(status);
        }
        if (!folder && !(file && isPhotoFilename(name))) continue;
        Entry entry{name, child.generic_string(), folder};
        if (!folder) entry.cover = entry.relativePath;
        else if (regularInside(root / child / "folder.jpg", root.string()))
            entry.cover = (child / "folder.jpg").generic_string();
        entries.push_back(std::move(entry));
    }
    return {true, galleryEntries(std::move(entries))};
}

} // namespace vdrsuite::imagegallery
