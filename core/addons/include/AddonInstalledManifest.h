#pragma once

#include "AddonHandlerRegistry.h"

#include <cerrno>
#include <cctype>
#include <cstddef>
#include <fcntl.h>
#include <map>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <utility>
#include <vector>

namespace vdrsuite::addons {
namespace installed_manifest_detail {

struct Fd {
    int fd = -1;
    explicit Fd(int value = -1) : fd(value) {}
    ~Fd() { if (fd >= 0) ::close(fd); }
    Fd(const Fd&) = delete;
    Fd& operator=(const Fd&) = delete;
    Fd(Fd&& other) noexcept : fd(other.fd) { other.fd = -1; }
    Fd& operator=(Fd&& other) noexcept {
        if (this != &other) {
            if (fd >= 0) ::close(fd);
            fd = other.fd;
            other.fd = -1;
        }
        return *this;
    }
};

inline bool safeDirectory(int fd)
{
    struct stat st{};
    return ::fstat(fd, &st) == 0 && S_ISDIR(st.st_mode) &&
           st.st_uid == 0 && (st.st_mode & 0022) == 0;
}

inline bool components(const std::string& root, std::vector<std::string>& out)
{
    if (root.empty() || root.size() > 4096 || root[0] != '/' ||
        root.find('\0') != std::string::npos ||
        (root.size() > 1 && root.back() == '/')) return false;
    std::size_t offset = 1;
    while (offset < root.size()) {
        const auto end = root.find('/', offset);
        const std::string part = root.substr(
            offset, end == std::string::npos ? end : end - offset);
        if (part.empty() || part == "." || part == "..") return false;
        out.push_back(part);
        if (end == std::string::npos) break;
        offset = end + 1;
    }
    return true;
}

inline bool unchanged(const struct stat& before, const struct stat& after)
{
    return before.st_dev == after.st_dev &&
           before.st_ino == after.st_ino &&
           before.st_mode == after.st_mode &&
           before.st_uid == after.st_uid &&
           before.st_size == after.st_size &&
           before.st_nlink == after.st_nlink &&
           before.st_mtim.tv_sec == after.st_mtim.tv_sec &&
           before.st_mtim.tv_nsec == after.st_mtim.tv_nsec &&
           before.st_ctim.tv_sec == after.st_ctim.tv_sec &&
           before.st_ctim.tv_nsec == after.st_ctim.tv_nsec;
}

// This reads the fixed, root-owned manifest companion beside the pinned
// handler. The /usr/share copy is descriptive inventory and may be hidden by
// a pre-existing private Suite web directory. Neither manifest nor an HTTP
// caller chooses a handler path, grant, version or executable capability.
inline bool readManifest(const std::string& root, std::string& data)
{
    std::vector<std::string> parts;
    if (!components(root, parts)) return false;
    for (const char* part : {"usr", "libexec", "vdr-suite", "addons", "rectools"})
        parts.emplace_back(part);
    Fd directory(::open("/", O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC));
    if (directory.fd < 0) return false;
    for (const auto& part : parts) {
        if (!safeDirectory(directory.fd)) return false;
        Fd next(::openat(directory.fd, part.c_str(),
                         O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC));
        if (next.fd < 0) return false;
        directory = std::move(next);
    }
    if (!safeDirectory(directory.fd)) return false;
    Fd file(::openat(directory.fd, "addon.json",
                     O_RDONLY | O_NOFOLLOW | O_NONBLOCK | O_CLOEXEC));
    if (file.fd < 0) return false;
    struct stat before{}, after{};
    if (::fstat(file.fd, &before) != 0 || !S_ISREG(before.st_mode) ||
        before.st_uid != 0 || (before.st_mode & 0022) != 0 ||
        before.st_nlink != 1 || before.st_size <= 0 || before.st_size > 65536)
        return false;
    data.assign(static_cast<std::size_t>(before.st_size), '\0');
    std::size_t total = 0;
    while (total < data.size()) {
        const ssize_t n = ::read(file.fd, &data[total], data.size() - total);
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) return false;
        total += static_cast<std::size_t>(n);
    }
    return ::fstat(file.fd, &after) == 0 && unchanged(before, after);
}

struct Field {
    enum class Kind { string, number, emptyArray } kind = Kind::string;
    std::string text;
};

// Bounded, strict reader for the *inert* schema-v1 manifest, not a general
// JSON decoder. Unknown keys, duplicates, nonempty arrays, nested values,
// executable declarations and malformed escapes all fail closed.
class Parser {
public:
    explicit Parser(const std::string& input) : input_(input) {}

    bool parse(std::map<std::string, Field>& values) {
        skip();
        if (!consume('{')) return false;
        skip();
        while (offset_ < input_.size() && input_[offset_] != '}') {
            std::string key;
            Field field;
            if (!string(key) || key.empty() || key.size() > 32) return false;
            skip();
            if (!consume(':')) return false;
            skip();
            if (offset_ >= input_.size()) return false;
            if (input_[offset_] == '"') {
                field.kind = Field::Kind::string;
                if (!string(field.text)) return false;
            } else if (input_[offset_] == '[') {
                field.kind = Field::Kind::emptyArray;
                ++offset_;
                skip();
                if (!consume(']')) return false;
            } else {
                field.kind = Field::Kind::number;
                const auto start = offset_;
                while (offset_ < input_.size() &&
                       std::isdigit(static_cast<unsigned char>(input_[offset_])))
                    ++offset_;
                if (start == offset_ || offset_ - start > 10) return false;
                field.text = input_.substr(start, offset_ - start);
            }
            if (!values.emplace(std::move(key), std::move(field)).second ||
                values.size() > 11) return false;
            skip();
            if (offset_ < input_.size() && input_[offset_] == '}') break;
            if (!consume(',')) return false;
            skip();
            if (offset_ >= input_.size() || input_[offset_] == '}') return false;
        }
        if (!consume('}')) return false;
        skip();
        return offset_ == input_.size();
    }
private:
    void skip() {
        while (offset_ < input_.size() &&
               std::isspace(static_cast<unsigned char>(input_[offset_])))
            ++offset_;
    }
    bool consume(char c) {
        if (offset_ >= input_.size() || input_[offset_] != c) return false;
        ++offset_;
        return true;
    }
    bool string(std::string& out) {
        if (!consume('"')) return false;
        while (offset_ < input_.size()) {
            const unsigned char c = static_cast<unsigned char>(input_[offset_++]);
            if (c == '"') return true;
            if (c < 32 || c == 127) return false;
            if (c == '\\') {
                if (offset_ >= input_.size()) return false;
                const char escaped = input_[offset_++];
                if (escaped == '"' || escaped == '\\' || escaped == '/')
                    out.push_back(escaped);
                else if (escaped == 'n') out.push_back('\n');
                else if (escaped == 't') out.push_back('\t');
                else return false;
            } else out.push_back(static_cast<char>(c));
            if (out.size() > 512) return false;
        }
        return false;
    }
    const std::string& input_;
    std::size_t offset_ = 0;
};

inline bool validVersion(const std::string& version)
{
    if (version.empty() || version.size() > 40) return false;
    unsigned parts = 0;
    std::size_t start = 0;
    for (std::size_t i = 0; i <= version.size(); ++i) {
        if (i == version.size() || version[i] == '.') {
            if (i == start || i - start > 12 ||
                (i - start > 1 && version[start] == '0')) return false;
            for (std::size_t j = start; j < i; ++j)
                if (version[j] < '0' || version[j] > '9') return false;
            ++parts;
            start = i + 1;
        } else if (version[i] < '0' || version[i] > '9') return false;
    }
    return parts == 3;
}
} // namespace installed_manifest_detail

// Same read for explicitly staged manual and eventual Debian installs.
// This only verifies stable, root-owned *descriptive* metadata tied to a
// separately approved version. It cannot confer execution trust or rights.
inline bool installedMediaToolsManifestMatches(
    const std::string& trustedRoot, const std::string& approvedVersion)
{
    using namespace installed_manifest_detail;
    if (!validVersion(approvedVersion) ||
        approvedVersion != kMediaToolsHandlerVersion) return false;
    std::string raw;
    if (!readManifest(trustedRoot, raw)) return false;
    std::map<std::string, Field> fields;
    if (!Parser(raw).parse(fields) || fields.size() != 11) return false;
    const auto equals = [&](const char* key, Field::Kind kind,
                            const char* value) {
        const auto it = fields.find(key);
        return it != fields.end() && it->second.kind == kind &&
               it->second.text == value;
    };
    const auto textField = [&](const char* key) {
        const auto it = fields.find(key);
        return it != fields.end() && it->second.kind == Field::Kind::string &&
               !it->second.text.empty() && it->second.text.size() <= 300 &&
               it->second.text.find_first_of("\r\n\t") == std::string::npos;
    };
    if (!equals("schemaVersion", Field::Kind::number, "1") ||
        !equals("id", Field::Kind::string, "rectools") ||
        !equals("package", Field::Kind::string,
                "vdr-suite-addon-media-tools") ||
        !equals("state", Field::Kind::string, "scaffold") ||
        !equals("version", Field::Kind::string, approvedVersion.c_str()) ||
        !textField("displayName") || !textField("description"))
        return false;
    for (const char* key :
         {"entrypoints", "capabilities", "permissions", "dependencies"})
        if (!equals(key, Field::Kind::emptyArray, "")) return false;
    return true;
}
} // namespace vdrsuite::addons
