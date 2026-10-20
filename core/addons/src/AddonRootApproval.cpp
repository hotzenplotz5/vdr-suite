#include "AddonRootApproval.h"

#include <cerrno>
#include <fcntl.h>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <utility>
#include <vector>

namespace vdrsuite::addons {
namespace {
struct Fd {
    int value = -1;
    explicit Fd(int v = -1) : value(v) {}
    ~Fd() { if (value >= 0) ::close(value); }
    Fd(const Fd&) = delete;
    Fd& operator=(const Fd&) = delete;
    Fd(Fd&& other) noexcept : value(other.value) { other.value = -1; }
    Fd& operator=(Fd&& other) noexcept {
        if (this != &other) {
            if (value >= 0) ::close(value);
            value = other.value;
            other.value = -1;
        }
        return *this;
    }
};

bool components(const std::string& path, std::vector<std::string>& out)
{
    if (path.empty() || path.front() != '/' || path.size() > 4096 ||
        path.find('\0') != std::string::npos)
        return false;
    if (path == "/") return true;
    if (path.back() == '/') return false;
    std::size_t at = 1;
    while (at < path.size()) {
        const auto end = path.find('/', at);
        const auto item = path.substr(
            at, end == std::string::npos ? end : end - at);
        if (item.empty() || item == "." || item == "..") return false;
        out.push_back(item);
        if (end == std::string::npos) break;
        at = end + 1;
    }
    return true;
}

bool safeDirectory(int fd)
{
    struct stat st{};
    return ::fstat(fd, &st) == 0 &&
        S_ISDIR(st.st_mode) && st.st_uid == 0 &&
        (st.st_mode & 0022) == 0;
}

bool stable(const struct stat& a, const struct stat& b)
{
    return a.st_dev == b.st_dev && a.st_ino == b.st_ino &&
        a.st_size == b.st_size && a.st_uid == b.st_uid &&
        a.st_mode == b.st_mode && a.st_nlink == b.st_nlink &&
        a.st_mtim.tv_sec == b.st_mtim.tv_sec &&
        a.st_mtim.tv_nsec == b.st_mtim.tv_nsec &&
        a.st_ctim.tv_sec == b.st_ctim.tv_sec &&
        a.st_ctim.tv_nsec == b.st_ctim.tv_nsec;
}

bool take(const std::string& data, std::size_t& index,
          const std::string& exact)
{
    if (data.compare(index, exact.size(), exact) != 0) return false;
    index += exact.size();
    return true;
}

bool untilQuote(const std::string& data, std::size_t& index,
                std::string& output, std::size_t maximum)
{
    const auto end = data.find('"', index);
    if (end == std::string::npos || end <= index || end - index > maximum)
        return false;
    output = data.substr(index, end - index);
    index = end;
    return true;
}

bool validVersion(const std::string& value)
{
    if (value.size() > 40) return false;
    unsigned parts = 0;
    std::size_t start = 0;
    for (std::size_t i = 0; i <= value.size(); ++i) {
        if (i == value.size() || value[i] == '.') {
            if (i == start || i - start > 12 ||
                (i - start > 1 && value[start] == '0')) return false;
            for (std::size_t j = start; j < i; ++j)
                if (value[j] < '0' || value[j] > '9') return false;
            ++parts;
            start = i + 1;
        } else if (value[i] < '0' || value[i] > '9') return false;
    }
    return parts == 3;
}

bool validDigest(const std::string& digest)
{
    if (digest.size() != 64) return false;
    for (const auto ch : digest)
        if (!((ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'f')))
            return false;
    return true;
}

bool parseCanonical(const std::string& data, RootApprovalResult& result)
{
    std::size_t pos = 0;
    std::string origin, sha256, state, version;
    if (!take(data, pos, "{\"id\":\"rectools\",\"origin\":\"") ||
        !untilQuote(data, pos, origin, 16) ||
        !take(data, pos,
            "\",\"package\":\"vdr-suite-addon-media-tools\",\"schemaVersion\":1,\"sha256\":\"") ||
        !untilQuote(data, pos, sha256, 64) ||
        !take(data, pos, "\",\"state\":\"") ||
        !untilQuote(data, pos, state, 16) ||
        !take(data, pos, "\",\"version\":\"") ||
        !untilQuote(data, pos, version, 40) ||
        !take(data, pos, "\"}\n") || pos != data.size())
        return false;
    if ((origin != "manual" && origin != "debian") ||
        !validDigest(sha256) || !validVersion(version) ||
        (state != "approved" && state != "revoked"))
        return false;
    if (state == "revoked") {
        result.status = RootApprovalStatus::revoked;
        return true;
    }
    result.status = RootApprovalStatus::approved;
    result.origin = origin == "manual" ? InstallOrigin::manual : InstallOrigin::debian;
    result.version = std::move(version);
    result.sha256 = std::move(sha256);
    return true;
}
} // namespace

RootApprovalResult readRootMediaToolsApproval(const std::string& root)
{
    std::vector<std::string> parts;
    if (!components(root, parts))
        return {RootApprovalStatus::invalidRoot, InstallOrigin::absent, {}, {}};
    for (const auto* part : {"etc", "vdr-suite", "addon-approvals"})
        parts.emplace_back(part);
    Fd current(::open("/", O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC));
    if (current.value < 0)
        return {RootApprovalStatus::ioError, InstallOrigin::absent, {}, {}};
    for (const auto& part : parts) {
        if (!safeDirectory(current.value))
            return {RootApprovalStatus::unsafeDirectory, InstallOrigin::absent, {}, {}};
        Fd next(::openat(current.value, part.c_str(),
                         O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC));
        if (next.value < 0)
            return {RootApprovalStatus::unsafeDirectory, InstallOrigin::absent, {}, {}};
        current = std::move(next);
    }
    if (!safeDirectory(current.value))
        return {RootApprovalStatus::unsafeDirectory, InstallOrigin::absent, {}, {}};
    Fd record(::openat(current.value, "rectools.json",
                       O_RDONLY | O_NOFOLLOW | O_NONBLOCK | O_CLOEXEC));
    if (record.value < 0)
        return {RootApprovalStatus::missing, InstallOrigin::absent, {}, {}};
    struct stat first{};
    if (::fstat(record.value, &first) != 0)
        return {RootApprovalStatus::ioError, InstallOrigin::absent, {}, {}};
    if (!S_ISREG(first.st_mode) || first.st_uid != 0 ||
        (first.st_mode & 0022) != 0 || first.st_nlink != 1 ||
        first.st_size <= 0 || first.st_size > 4096)
        return {RootApprovalStatus::unsafeRecord, InstallOrigin::absent, {}, {}};
    std::string raw(static_cast<std::size_t>(first.st_size), '\0');
    std::size_t readBytes = 0;
    while (readBytes < raw.size()) {
        const ssize_t n = ::read(record.value, &raw[readBytes],
                                 raw.size() - readBytes);
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0)
            return {RootApprovalStatus::changedDuringRead, InstallOrigin::absent, {}, {}};
        readBytes += static_cast<std::size_t>(n);
    }
    struct stat last{};
    if (::fstat(record.value, &last) != 0 || !stable(first, last))
        return {RootApprovalStatus::changedDuringRead, InstallOrigin::absent, {}, {}};

    RootApprovalResult result;
    if (!parseCanonical(raw, result))
        return {RootApprovalStatus::invalidRecord, InstallOrigin::absent, {}, {}};
    return result;
}
} // namespace vdrsuite::addons
