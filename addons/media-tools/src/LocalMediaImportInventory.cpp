#include "LocalMediaImportInventory.h"

#include "MediaImport.h"

#include <array>
#include <algorithm>
#include <cerrno>
#include <cctype>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <iomanip>
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

struct IndexedSource {
    std::string id;
    LocalMediaImportSource source;
};

LocalMediaImportSource toSource(const ImportPlan& plan) {
    LocalMediaImportSource source;
    source.sourceRoot = plan.sourceRoot;
    source.videoRoot = plan.videoRoot;
    source.sourceFile = plan.sourceFile;
    source.sourceDevice = plan.sourceDevice;
    source.sourceInode = plan.sourceInode;
    source.sourceBytes = plan.sourceBytes;
    source.sourceMtimeSeconds = plan.sourceMtimeSeconds;
    source.sourceMtimeNanoseconds = plan.sourceMtimeNanoseconds;
    return source;
}

// Deterministic opaque observation ID, not an authorization token. Core still
// checks the actor's grant, approved provider and the exact source snapshot.
std::string scannedId(const std::string& prefix, const ImportPlan& plan) {
    std::ostringstream identity;
    identity << prefix << '\n' << plan.sourceFile.generic_string() << '\n'
        << plan.sourceDevice << ':' << plan.sourceInode << ':'
        << plan.sourceBytes << ':' << plan.sourceMtimeSeconds << ':'
        << plan.sourceMtimeNanoseconds;
    const std::string input = identity.str();
    auto fnv = [](const std::string& value, std::uint64_t seed) {
        std::uint64_t hash = seed;
        for (unsigned char ch : value) {
            hash ^= ch;
            hash *= UINT64_C(1099511628211);
        }
        return hash;
    };
    std::string inverse(input.rbegin(), input.rend());
    std::ostringstream result;
    result << "scan_" << std::hex << std::setfill('0') << std::setw(16)
        << fnv(input, UINT64_C(14695981039346656037)) << std::setw(16)
        << fnv(inverse, UINT64_C(7809847782465536322));
    return result.str();
}

bool validConfiguredRoot(const std::filesystem::path& root) {
    namespace fs = std::filesystem;
    if (!root.is_absolute() || root.lexically_normal() != root) return false;
    fs::path part;
    for (const auto& item : root) {
        part /= item;
        std::error_code error;
        if (fs::is_symlink(fs::symlink_status(part, error)) || error)
            return false;
    }
    return fs::is_directory(root);
}

bool scanDirectory(const std::array<std::string, 5>& entry,
                   std::vector<IndexedSource>& found)
{
    namespace fs = std::filesystem;
    const fs::path sourceRoot(entry[2]), videoRoot(entry[3]);
    if (!validConfiguredRoot(sourceRoot) || !validConfiguredRoot(videoRoot))
        return false;
    // All candidates are scanned read-only. One subdirectory of depth is
    // enough for the prior Rectools import convention, with a hard work cap.
    std::vector<fs::path> paths;
    std::size_t entries = 0;
    const auto addFiles = [&](const fs::path& root, bool descend) {
        for (const auto& item : fs::directory_iterator(root)) {
            if (++entries > 4096) return false;
            if (fs::is_symlink(item.symlink_status())) continue;
            if (item.is_regular_file()) paths.push_back(item.path());
            else if (descend && item.is_directory()) {
                for (const auto& nested : fs::directory_iterator(item.path())) {
                    if (++entries > 4096) return false;
                    if (!fs::is_symlink(nested.symlink_status()) &&
                        nested.is_regular_file())
                        paths.push_back(nested.path());
                }
            }
        }
        return true;
    };
    if (!addFiles(sourceRoot, true)) return false;
    std::sort(paths.begin(), paths.end());
    const auto now = fs::file_time_type::clock::now();
    for (const auto& path : paths) {
        // Do not hand a partially uploaded source to an unattended importer.
        // Every selection is re-inspected at dispatch and actual execution.
        if (now - fs::last_write_time(path) < std::chrono::minutes(2))
            continue;
        try {
            const auto plan = inspectImport({sourceRoot, videoRoot, path});
            found.push_back({scannedId(entry[1], plan), toSource(plan)});
            if (found.size() > 256) return false;
        } catch (...) {
            // Source disappeared, unsupported format, symlinked parent or
            // title already imported: not an executable selection.
        }
    }
    return true;
}

bool inventorySources(const std::filesystem::path& root, const std::string& backendId,
                      std::vector<IndexedSource>& sources)
{
    sources.clear();
    std::string content;
    if (!readInventory(root, content)) return false;
    std::istringstream input(content);
    std::set<std::pair<std::string, std::string>> seenRows;
    std::string line;
    std::size_t rows = 0;
    try {
        while (std::getline(input, line)) {
            if (++rows > 128 || line.empty() || line.back() == '\r')
                return false;
            std::array<std::string, 5> entry{};
            if (!fields(line, entry) ||
                !inventoryIdentifier(entry[0], 1) ||
                !inventoryIdentifier(entry[1], 16) ||
                !seenRows.emplace(entry[0], entry[1]).second)
                return false;
            if (entry[0] != backendId) continue;
            if (entry[4] == "*") {
                if (!scanDirectory(entry, sources)) return false;
            } else {
                try {
                    const auto plan = inspectImport({
                        entry[2], entry[3], entry[4]});
                    sources.push_back({entry[1], toSource(plan)});
                } catch (...) {
                    // Explicitly listed source is missing or already imported.
                }
            }
        }
    } catch (...) { return false; }
    std::sort(sources.begin(), sources.end(), [](const IndexedSource& a,
                                                  const IndexedSource& b) {
        return a.id < b.id;
    });
    for (std::size_t index = 1; index < sources.size(); ++index)
        if (sources[index].id == sources[index - 1].id) return false;
    // The backend catalog accepts a maximum of 32 currently visible IDs.
    // A larger scanned directory is delivered in bounded future batches as
    // previously imported titles disappear from inspectImport candidates.
    if (sources.size() > 32U) sources.resize(32U);
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
    std::vector<IndexedSource> sources;
    if (!inventorySources(installRoot_, backendId, sources)) return false;
    const auto selected = std::find_if(sources.begin(), sources.end(),
        [&](const IndexedSource& source) { return source.id == sourceId; });
    if (selected == sources.end()) return false;
    output = selected->source;
    return true;
}

bool RootOwnedMediaImportInventory::hasTrustedSource(
    const std::string& backendId)
{
    if (!inventoryIdentifier(backendId, 1)) return false;
    std::vector<IndexedSource> sources;
    return inventorySources(installRoot_, backendId, sources) &&
           !sources.empty();
}

bool RootOwnedMediaImportInventory::listTrustedSourceIds(
    const std::string& backendId, std::vector<std::string>& sourceIds)
{
    sourceIds.clear();
    if (!inventoryIdentifier(backendId, 1)) return false;
    std::vector<IndexedSource> sources;
    if (!inventorySources(installRoot_, backendId, sources)) return false;
    for (const auto& source : sources)
        sourceIds.push_back(source.id);
    return true;
}
bool RootOwnedMediaImportInventory::listTrustedSourceLabels(
    const std::string& backendId, std::vector<MediaImportSourceLabel>& out)
{
    out.clear();
    if (!inventoryIdentifier(backendId, 1)) return false;
    std::vector<IndexedSource> sources;
    if (!inventorySources(installRoot_, backendId, sources)) return false;
    for (const auto& source : sources) {
        const std::string label = source.source.sourceFile.stem().string();
        if (label.empty() || label.size() > 160 ||
            std::any_of(label.begin(), label.end(), [](unsigned char ch) {
                return ch < 0x20 || ch == 0x7f || ch == '/' || ch == '\\';
            })) {
            out.clear();
            return false;
        }
        out.push_back({source.id, label});
    }
    return true;
}
} // namespace vdrsuite::mediatools
