#include "LocalMediaImportInventory.h"

#include "MediaImport.h"

#include <array>
#include <algorithm>
#include <cerrno>
#include <cctype>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <functional>
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
std::string scannedId(const std::string& prefix, const LocalMediaImportSource& source) {
    std::ostringstream identity;
    identity << prefix << '\n' << source.sourceFile.generic_string() << '\n'
        << source.sourceDevice << ':' << source.sourceInode << ':'
        << source.sourceBytes << ':' << source.sourceMtimeSeconds << ':'
        << source.sourceMtimeNanoseconds;
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

// Marked names are exposed ONLY as observations. Ordinary imports still
// require inspectImport(), which categorically rejects them.
bool inspectMarkedSource(const std::filesystem::path& sourceRoot,
                         const std::filesystem::path& videoRoot,
                         const std::filesystem::path& path,
                         LocalMediaImportSource& out)
{
    namespace fs = std::filesystem;
    out = {};
    if (!validConfiguredRoot(sourceRoot) ||
        !validConfiguredRoot(videoRoot) || !path.is_absolute() ||
        path.lexically_normal() != path || path == sourceRoot ||
        sourceRoot == videoRoot) return false;
    const auto relative = path.lexically_relative(sourceRoot);
    if (relative.empty() || relative.is_absolute()) return false;
    for (const auto& component : relative)
        if (component == ".." || component == ".") return false;
    const auto inside = [](const fs::path& root, const fs::path& child) {
        const auto relative = child.lexically_relative(root);
        if (relative.empty() || relative.is_absolute()) return false;
        for (const auto& item : relative) if (item == "..") return false;
        return true;
    };
    if (inside(sourceRoot, videoRoot) || inside(videoRoot, sourceRoot))
        return false;
    const std::string name = path.filename().string();
    if (name.empty() || name.front() == '.' ||
        name.size() > 255 || name.find_first_of("\r\n\t") != std::string::npos)
        return false;
    std::string ext = path.extension().string();
    std::string stem = path.stem().string();
    std::transform(ext.begin(), ext.end(), ext.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    std::transform(stem.begin(), stem.end(), stem.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    if (ext != ".mkv" && ext != ".mp4" && ext != ".ts" &&
        ext != ".mov" && ext != ".avi" && ext != ".webm" &&
        ext != ".m4v") return false;
    bool marked = false;
    for (const char* value : {".pc_encode", ".skipped", ".duplicate"}) {
        const std::string suffix(value);
        if (stem.size() > suffix.size() &&
            stem.compare(stem.size() - suffix.size(), suffix.size(), suffix) == 0)
            marked = true;
    }
    if (!marked) return false;
    fs::path parent = path.parent_path();
    while (parent != sourceRoot) {
        const auto status = fs::symlink_status(parent);
        if (!fs::is_directory(status) || fs::is_symlink(status) ||
            parent == parent.parent_path()) return false;
        parent = parent.parent_path();
    }
    struct stat st{};
    if (::lstat(path.c_str(), &st) != 0 || !S_ISREG(st.st_mode) ||
        st.st_nlink != 1 || st.st_size <= 0 || st.st_dev == 0 || st.st_ino == 0)
        return false;
    out.sourceRoot = sourceRoot;
    out.videoRoot = videoRoot;
    out.sourceFile = path;
    out.sourceDevice = static_cast<std::uint64_t>(st.st_dev);
    out.sourceInode = static_cast<std::uint64_t>(st.st_ino);
    out.sourceBytes = static_cast<std::uintmax_t>(st.st_size);
    out.sourceMtimeSeconds = st.st_mtim.tv_sec;
    out.sourceMtimeNanoseconds = st.st_mtim.tv_nsec;
    return true;
}

bool scanDirectory(const std::array<std::string, 5>& entry,
                   std::vector<IndexedSource>& found)
{
    namespace fs = std::filesystem;
    const fs::path sourceRoot(entry[2]), videoRoot(entry[3]);
    if (!validConfiguredRoot(sourceRoot) || !validConfiguredRoot(videoRoot))
        return false;
    // Walk every nested directory, not just the first two levels. An
    // iterative queue avoids unbounded recursive call stacks; never follow
    // symlinks and retain the existing global 4096-entry fail-closed cap.
    // An oversized scan must NOT be reported as complete.
    std::vector<fs::path> paths;
    std::vector<fs::path> directories{sourceRoot};
    std::size_t entries = 0;
    for (std::size_t cursor = 0; cursor < directories.size(); ++cursor) {
        for (const auto& item : fs::directory_iterator(directories[cursor])) {
            if (++entries > 4096) return false;
            const auto status = item.symlink_status();
            if (fs::is_symlink(status)) continue;
            if (fs::is_regular_file(status)) paths.push_back(item.path());
            else if (fs::is_directory(status)) directories.push_back(item.path());
        }
    }
    std::sort(paths.begin(), paths.end());
    const auto now = fs::file_time_type::clock::now();
    for (const auto& path : paths) {
        // Do not hand a partially uploaded source to an unattended importer.
        // Every selection is re-inspected at dispatch and actual execution.
        if (now - fs::last_write_time(path) < std::chrono::minutes(2))
            continue;
        try {
            const auto plan = inspectImport({sourceRoot, videoRoot, path});
            const auto local = toSource(plan);
            found.push_back({scannedId(entry[1], local), local});
            if (found.size() > 256) return false;
        } catch (...) {
            LocalMediaImportSource marked;
            if (inspectMarkedSource(sourceRoot, videoRoot, path, marked)) {
                found.push_back({scannedId(entry[1], marked), marked});
                if (found.size() > 256) return false;
            }
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
                    // A marked source is selectable only by an explicit release.
                    LocalMediaImportSource marked;
                    if (inspectMarkedSource(entry[2], entry[3], entry[4], marked))
                        sources.push_back({entry[1], marked});
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
    // The verified Agent observation and Core catalog have a bounded
    // 256-source ceiling. The public endpoint pages these in 32-item reads.
    // A larger filesystem scan still fails closed above its own limit;
    // do not imply unbounded scanner continuation here.
    if (sources.size() > 256U) return false;
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
