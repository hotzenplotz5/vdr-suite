#include "ImageGallerySourceSettings.h"

#include <cerrno>
#include <fcntl.h>
#include <set>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>
#include <utility>

namespace vdrsuite::imagegallery {
namespace {
bool validField(const std::string& value)
{
    return !value.empty() && value.find_first_of("\t\r\n") == std::string::npos &&
        value.find('\0') == std::string::npos;
}

bool validSources(const std::vector<Source>& sources)
{
    std::set<std::string> ids;
    for (const auto& source : sources) {
        const std::filesystem::path root(source.root);
        if (!validField(source.id) || !validField(source.title) ||
            !validField(source.root) || !root.is_absolute() ||
            root.lexically_normal() == root.root_path() ||
            !ids.insert(source.id).second) return false;
    }
    return true;
}

// Same simple tab-separated configuration style as the existing addon
// source inventory, without Media Tools' video-root/Agent/import contracts.
bool decode(const std::string& content, std::vector<Source>& sources)
{
    std::istringstream input(content);
    std::string row;
    while (std::getline(input, row)) {
        const auto first = row.find('\t');
        if (first == std::string::npos) return false;
        const auto second = row.find('\t', first + 1);
        if (second == std::string::npos ||
            row.find('\t', second + 1) != std::string::npos) return false;
        sources.push_back({row.substr(0, first),
            row.substr(first + 1, second - first - 1), row.substr(second + 1)});
    }
    return validSources(sources);
}

struct File {
    int fd;
    explicit File(int descriptor) : fd(descriptor) {}
    ~File() { if (fd >= 0) ::close(fd); }
    File(const File&) = delete;
    File& operator=(const File&) = delete;
};

bool writeAll(int fd, const std::string& content)
{
    std::size_t offset = 0;
    while (offset < content.size()) {
        const auto n = ::write(fd, content.data() + offset, content.size() - offset);
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) return false;
        offset += static_cast<std::size_t>(n);
    }
    return true;
}
} // namespace

ImageGallerySourceSettings::ImageGallerySourceSettings(
    std::filesystem::path file, std::vector<Source> defaults)
    : file_(std::move(file)), defaults_(std::move(defaults)) {}

SourceSettingsSnapshot ImageGallerySourceSettings::load() const
{
    if (!file_.is_absolute() || file_.filename().empty()) return {};
    File file(::open(file_.c_str(), O_RDONLY | O_NOFOLLOW | O_NONBLOCK | O_CLOEXEC));
    if (file.fd < 0) {
        if (errno == ENOENT && validSources(defaults_))
            return {SourceSettingsState::defaults, defaults_};
        return {};
    }
    struct stat st {};
    if (::fstat(file.fd, &st) != 0 || !S_ISREG(st.st_mode)) return {};
    std::string content;
    char buffer[4096];
    for (;;) {
        const auto n = ::read(file.fd, buffer, sizeof(buffer));
        if (n < 0 && errno == EINTR) continue;
        if (n < 0) return {};
        if (n == 0) break;
        content.append(buffer, static_cast<std::size_t>(n));
    }
    std::vector<Source> sources;
    if (!decode(content, sources)) return {};
    return {SourceSettingsState::configured, std::move(sources)};
}

bool ImageGallerySourceSettings::save(const std::vector<Source>& sources) const
{
    if (!file_.is_absolute() || file_.filename().empty() ||
        !validSources(sources)) return false;
    // Missing or temporarily offline picture roots are valid settings.
    // Only the protected, daemon-owned configuration directory is written.
    File directory(::open(file_.parent_path().c_str(),
        O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW));
    if (directory.fd < 0) return false;
    struct stat previous {};
    if (::fstatat(directory.fd, file_.filename().c_str(), &previous,
            AT_SYMLINK_NOFOLLOW) == 0) {
        if (!S_ISREG(previous.st_mode)) return false;
    } else if (errno != ENOENT) return false;

    std::string content;
    for (const auto& source : sources)
        content += source.id + '\t' + source.title + '\t' + source.root + '\n';
    auto temporary = (file_.parent_path() / ".image-sources-XXXXXX").string();
    File output(::mkstemp(temporary.data()));
    if (output.fd < 0) return false;
    const auto name = std::filesystem::path(temporary).filename();
    const bool published = writeAll(output.fd, content) && ::fsync(output.fd) == 0 &&
        ::renameat(directory.fd, name.c_str(), directory.fd, file_.filename().c_str()) == 0;
    if (!published) {
        ::unlink(temporary.c_str());
        return false;
    }
    return ::fsync(directory.fd) == 0;
}

} // namespace vdrsuite::imagegallery
