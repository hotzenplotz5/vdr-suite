#include "LocalImageGalleryProvider.h"
#include "EpgArtworkPathPolicy.h"

#include <algorithm>
#include <cerrno>
#include <random>
#include <dirent.h>
#include <fcntl.h>
#include <filesystem>
#include <memory>
#include <cstdint>
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

struct FileCloser {
    int fd;
    ~FileCloser() { if (fd >= 0) ::close(fd); }
    FileCloser(const FileCloser&) = delete;
    FileCloser& operator=(const FileCloser&) = delete;
    explicit FileCloser(int value) : fd(value) {}
};

bool resolveSourcePath(const Source& source, const std::string& requested,
                       fs::path& root, fs::path& relative)
{
    if (!relativeDirectoryPath(requested) ||
        !fs::path(source.root).is_absolute() ||
        source.root.find('\0') != std::string::npos) return false;
    std::error_code error;
    root = fs::canonical(source.root, error);
    if (error || root == root.root_path()) return false;
    std::string resolved;
    if (!EpgArtworkPathPolicy::resolveAllowedPath(
            (root / requested).string(), {root.string()}, resolved)) return false;
    relative = fs::path(resolved).lexically_relative(root);
    return !relative.empty();
}

int openRoot(const fs::path& root)
{
    return descend(::open("/", O_RDONLY | O_DIRECTORY | O_CLOEXEC),
                   root.relative_path());
}

// Identify the transport media type by bytes, never a client-supplied MIME
// value. This is a signature check, not a full decoder or EXIF inspection.
std::string photoContentType(const std::string& bytes)
{
    if (bytes.size() >= 3 && bytes.compare(0, 3, "\xff\xd8\xff", 3) == 0)
        return "image/jpeg";
    if (bytes.size() >= 8 && bytes.compare(0, 8, "\x89PNG\r\n\x1a\n", 8) == 0)
        return "image/png";
    if (bytes.size() >= 12 && bytes.compare(0, 4, "RIFF") == 0 &&
        bytes.compare(8, 4, "WEBP") == 0) return "image/webp";
    if (bytes.size() >= 14 && bytes.compare(0, 2, "BM") == 0)
        return "image/bmp";
    return {};
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
                                  const std::string& relativeDirectory,
                                  std::size_t* scanBudget)
{
    fs::path root, relative;
    if (!resolveSourcePath(source, relativeDirectory, root, relative)) return {};
    std::error_code error;
    const int fd = descend(openRoot(root), relative);
    if (fd < 0) return {};
    DIR* raw = ::fdopendir(fd);
    if (!raw) {
        ::close(fd);
        return {};
    }
    std::unique_ptr<DIR, DirectoryCloser> directory(raw);
    std::vector<Entry> entries;
    while (!scanBudget || *scanBudget > 0) {
        errno = 0;
        const auto* item = ::readdir(directory.get());
        if (!item) {
            if (errno != 0) return {};
            break;
        }
        const std::string name(item->d_name);
        if (name == "." || name == "..") continue;
        if (scanBudget) --*scanBudget;
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
        else if (!scanBudget && regularInside(root / child / "folder.jpg", root.string()))
            entry.cover = (child / "folder.jpg").generic_string();
        entries.push_back(std::move(entry));
    }
    return {true, galleryEntries(std::move(entries))};
}

std::vector<SamplePhoto> sampleLocalPhotos(const std::vector<Source>& sources)
{
    constexpr std::size_t sampleSize = 9, maxEntries = 8192, maxFolders = 64;
    std::mt19937 rng(std::random_device{}());
    struct Pending { std::size_t source; std::string path; unsigned depth; };
    std::vector<Pending> pending;
    for (std::size_t i = 0; i < sources.size(); ++i) pending.push_back({i,{},0});
    std::shuffle(pending.begin(),pending.end(),rng);
    std::vector<SamplePhoto> result;
    std::size_t budget = maxEntries, seen = 0;
    // Breadth-first traversal gives different sources and albums a chance;
    // each folder is bounded, including entries that are not pictures.
    for (std::size_t i = 0; i < pending.size() && i < maxFolders && budget; ++i) {
        const auto folder = pending[i];
        std::size_t allowance = std::min<std::size_t>(1024,budget);
        const auto initial = allowance;
        auto listing = browseLocalDirectory(sources[folder.source],folder.path,&allowance);
        budget -= initial - allowance;
        if (!listing.success) continue;
        std::shuffle(listing.entries.begin(),listing.entries.end(),rng);
        for (const auto& entry : listing.entries) {
            if (entry.directory) {
                if (folder.depth < 12 && pending.size() < maxFolders)
                    pending.push_back({folder.source,entry.relativePath,folder.depth+1});
                continue;
            }
            SamplePhoto photo{sources[folder.source].id,entry.name,entry.relativePath};
            ++seen;
            if (result.size() < sampleSize) result.push_back(std::move(photo));
            else {
                const auto pos = std::uniform_int_distribution<std::size_t>(0,seen-1)(rng);
                if (pos < sampleSize) result[pos] = std::move(photo);
            }
        }
    }
    std::shuffle(result.begin(),result.end(),rng);
    return result;
}

PhotoReadResult readLocalPhoto(const Source& source,
                               const std::string& relativePath,
                               std::size_t byteBudget)
{
    if (relativePath.empty() || !isPhotoFilename(relativePath))
        return {PhotoReadStatus::unsupported, {}, {}};
    fs::path root, relative;
    if (!resolveSourcePath(source, relativePath, root, relative) ||
        relative == "." || relative.filename().empty()) return {};
    FileCloser directory(descend(openRoot(root), relative.parent_path()));
    if (directory.fd < 0) return {};
    FileCloser file(::openat(directory.fd, relative.filename().c_str(),
        O_RDONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK));
    struct stat before {};
    if (file.fd < 0 || ::fstat(file.fd, &before) != 0 ||
        !S_ISREG(before.st_mode) || before.st_size <= 0) return {};
    if (static_cast<std::uintmax_t>(before.st_size) > byteBudget)
        return {PhotoReadStatus::exceedsBudget, {}, {}};
    std::string bytes(static_cast<std::size_t>(before.st_size), '\0');
    std::size_t offset = 0;
    while (offset < bytes.size()) {
        const auto count = ::read(file.fd, bytes.data() + offset, bytes.size() - offset);
        if (count < 0 && errno == EINTR) continue;
        if (count <= 0) return {};
        offset += static_cast<std::size_t>(count);
    }
    struct stat after {};
    if (::fstat(file.fd, &after) != 0 || before.st_size != after.st_size ||
        before.st_mtim.tv_sec != after.st_mtim.tv_sec ||
        before.st_mtim.tv_nsec != after.st_mtim.tv_nsec) return {};
    auto type = photoContentType(bytes);
    if (type.empty()) return {PhotoReadStatus::unsupported, {}, {}};
    return {PhotoReadStatus::ready, std::move(type), std::move(bytes)};
}

} // namespace vdrsuite::imagegallery
