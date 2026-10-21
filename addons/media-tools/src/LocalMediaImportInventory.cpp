#include "LocalMediaImportInventory.h"

#include "MediaImport.h"

#include <array>
#include <cerrno>
#include <cctype>
#include <fcntl.h>
#include <set>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <utility>
#include <vector>

namespace vdrsuite::mediatools {
namespace {
class FileDescriptor {
public:
    explicit FileDescriptor(int fd = -1) : fd_(fd) {}
    ~FileDescriptor() { if (fd_ >= 0) ::close(fd_); }
    FileDescriptor(const FileDescriptor&) = delete;
    FileDescriptor& operator=(const FileDescriptor&) = delete;
    int get() const { return fd_; }
private:
    int fd_;
};

bool rootControlled(int fd, bool directory)
{
    struct stat status{};
    return fd >= 0 && ::fstat(fd, &status) == 0 &&
        status.st_uid == 0 && (status.st_mode & 0022) == 0 &&
        (directory ? S_ISDIR(status.st_mode) : S_ISREG(status.st_mode));
}

bool inventoryIdentifier(const std::string& value, std::size_t minimum)
{
    if (value.size() < minimum || value.size() > 64) return false;
    for (unsigned char character : value)
        if (!(std::isalnum(character) || character == '_' || character == '-'))
            return false;
    return true;
}

bool readInventory(const std::filesystem::path& root, std::string& content)
{
    content.clear();
    if (!root.is_absolute() || root.lexically_normal() != root) return false;
    FileDescriptor base(::open(root.c_str(), O_RDONLY | O_DIRECTORY |
                                   O_NOFOLLOW | O_CLOEXEC));
    if (!rootControlled(base.get(), true)) return false;
    int parent = base.get();
    std::vector<int> opened;
    const auto cleanup = [&]() {
        for (auto it = opened.rbegin(); it != opened.rend(); ++it)
            ::close(*it);
    };
    for (const char* component : {"etc", "vdr-suite", "addon-sources"})
    {
        const int next = ::openat(parent, component,
            O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
        if (!rootControlled(next, true))
        {
            if (next >= 0) ::close(next);
            cleanup();
            return false;
        }
        opened.push_back(next);
        parent = next;
    }
    FileDescriptor file(::openat(parent, "media-import.tsv",
                                O_RDONLY | O_NOFOLLOW | O_CLOEXEC | O_NONBLOCK));
    cleanup();
    if (!rootControlled(file.get(), false)) return false;
    std::array<char, 4096> bytes{};
    while (true)
    {
        const ssize_t amount = ::read(file.get(), bytes.data(), bytes.size());
        if (amount < 0 && errno == EINTR) continue;
        if (amount < 0) return false;
        if (amount == 0) break;
        if (content.size() + static_cast<std::size_t>(amount) > 16384)
            return false;
        content.append(bytes.data(), static_cast<std::size_t>(amount));
    }
    return !content.empty() && content.back() == '\n' &&
           content.find('\0') == std::string::npos;
}

bool fields(const std::string& line, std::array<std::string, 5>& columns)
{
    std::size_t offset = 0;
    for (std::size_t i = 0; i < columns.size(); ++i)
    {
        const auto end = line.find('\t', offset);
        if ((i < columns.size() - 1 && end == std::string::npos) ||
            (i == columns.size() - 1 && end != std::string::npos))
            return false;
        columns[i] = line.substr(offset,
            end == std::string::npos ? std::string::npos : end - offset);
        if (columns[i].empty()) return false;
        offset = end + 1;
    }
    return true;
}
} // namespace

RootOwnedMediaImportInventory::RootOwnedMediaImportInventory(
    std::filesystem::path installRoot)
    : installRoot_(std::move(installRoot))
{
}

bool RootOwnedMediaImportInventory::resolve(
    const std::string& backendId, const std::string& sourceId,
    LocalMediaImportSource& output)
{
    output = {};
    if (!inventoryIdentifier(backendId, 1) ||
        !inventoryIdentifier(sourceId, 16))
        return false;
    std::string content;
    if (!readInventory(installRoot_, content)) return false;

    std::istringstream input(content);
    std::set<std::pair<std::string, std::string>> seen;
    LocalMediaImportSource selected;
    bool found = false;
    std::string line;
    std::size_t rows = 0;
    while (std::getline(input, line))
    {
        if (++rows > 128 || line.empty() || line.back() == '\r')
            return false;
        std::array<std::string, 5> values{};
        if (!fields(line, values) ||
            !inventoryIdentifier(values[0], 1) ||
            !inventoryIdentifier(values[1], 16) ||
            !seen.emplace(values[0], values[1]).second)
            return false;
        if (values[0] != backendId || values[1] != sourceId)
            continue;

        // Reuse the existing importer preflight: strict paths, symlink
        // exclusion, source inode/mtime/size, disjoint roots and valid media.
        // No operation on the target and no handler execution occurs here.
        try
        {
            const auto plan = inspectImport({
                values[2], values[3], values[4]});
            selected.sourceRoot = plan.sourceRoot;
            selected.videoRoot = plan.videoRoot;
            selected.sourceFile = plan.sourceFile;
            selected.sourceDevice = plan.sourceDevice;
            selected.sourceInode = plan.sourceInode;
            selected.sourceBytes = plan.sourceBytes;
            selected.sourceMtimeSeconds = plan.sourceMtimeSeconds;
            selected.sourceMtimeNanoseconds = plan.sourceMtimeNanoseconds;
            found = true;
        }
        catch (...) { return false; }
    }
    if (!found) return false;
    output = std::move(selected);
    return true;
}

} // namespace vdrsuite::mediatools

namespace vdrsuite::mediatools {

bool RootOwnedMediaImportInventory::hasTrustedSource(
    const std::string& backendId)
{
    if (!inventoryIdentifier(backendId, 1)) return false;
    std::string content;
    if (!readInventory(installRoot_, content)) return false;
    std::istringstream input(content);
    std::set<std::pair<std::string, std::string>> unique;
    std::string line;
    bool found = false;
    std::size_t count = 0;
    while (std::getline(input, line))
    {
        if (++count > 128 || line.empty() || line.back() == '\r')
            return false;
        std::array<std::string, 5> fieldsOnDisk{};
        if (!fields(line, fieldsOnDisk) ||
            !inventoryIdentifier(fieldsOnDisk[0], 1) ||
            !inventoryIdentifier(fieldsOnDisk[1], 16) ||
            !unique.emplace(fieldsOnDisk[0], fieldsOnDisk[1]).second)
            return false;
        if (fieldsOnDisk[0] != backendId) continue;
        try {
            (void)inspectImport({
                fieldsOnDisk[2], fieldsOnDisk[3], fieldsOnDisk[4]});
            found = true;
        } catch (...) {
            // A stale source must not become executable merely because its
            // mapping remains in the root-owned ledger.
            continue;
        }
    }
    return found;
}

} // namespace vdrsuite::mediatools
