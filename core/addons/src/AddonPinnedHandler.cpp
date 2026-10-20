#include "AddonPinnedHandler.h"

#include <openssl/evp.h>

#include <array>
#include <cerrno>
#include <cstdint>
#include <fcntl.h>
#include <memory>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

namespace vdrsuite::addons {
namespace {
constexpr std::uint64_t MaximumBinaryBytes = 128ULL * 1024ULL * 1024ULL;
constexpr const char* FixedHandlerDirectory =
    "usr/libexec/vdr-suite/addons/rectools";
constexpr const char* FixedHandlerName = "vdr-suite-media-import";

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
    int release() { const int v = value; value = -1; return v; }
};

bool validDigest(const std::string& s)
{
    if (s.size() != 64) return false;
    for (unsigned char c : s)
        if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'))) return false;
    return true;
}

bool pathParts(const std::string& path, std::vector<std::string>& parts)
{
    if (path.empty() || path.size() > 4096 || path.front() != '/' ||
        path.find('\0') != std::string::npos)
        return false;
    if (path == "/") return true;
    if (path.back() == '/') return false;
    std::size_t at = 1;
    while (at < path.size()) {
        const auto end = path.find('/', at);
        const std::string item =
            path.substr(at, end == std::string::npos ? end : end - at);
        if (item.empty() || item == "." || item == "..") return false;
        parts.push_back(item);
        if (end == std::string::npos) break;
        at = end + 1;
    }
    return true;
}

bool safeDirectory(int fd)
{
    struct stat st{};
    return ::fstat(fd, &st) == 0 && S_ISDIR(st.st_mode) &&
        st.st_uid == 0 && (st.st_mode & 0022) == 0;
}

bool sameFile(const struct stat& a, const struct stat& b)
{
    return a.st_dev == b.st_dev && a.st_ino == b.st_ino &&
        a.st_uid == b.st_uid && a.st_mode == b.st_mode &&
        a.st_size == b.st_size && a.st_nlink == b.st_nlink &&
        a.st_mtim.tv_sec == b.st_mtim.tv_sec &&
        a.st_mtim.tv_nsec == b.st_mtim.tv_nsec &&
        a.st_ctim.tv_sec == b.st_ctim.tv_sec &&
        a.st_ctim.tv_nsec == b.st_ctim.tv_nsec;
}
}

HandlerDescriptorResult inspectPinnedMediaToolsHandler(
    const std::string& root, const std::string& approvedSha256)
{
    std::vector<std::string> directories;
    if (!pathParts(root, directories))
        return {HandlerDescriptorReason::invalidRoot, {}};
    if (!validDigest(approvedSha256))
        return {HandlerDescriptorReason::invalidApprovedDigest, {}};

    std::vector<std::string> fixed;
    if (!pathParts(std::string("/") + FixedHandlerDirectory, fixed))
        return {HandlerDescriptorReason::ioError, {}};
    directories.insert(directories.end(), fixed.begin(), fixed.end());

    Fd parent(::open("/", O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC));
    if (parent.value < 0) return {HandlerDescriptorReason::ioError, {}};
    for (const auto& component : directories) {
        if (!safeDirectory(parent.value))
            return {HandlerDescriptorReason::unsafeDirectory, {}};
        Fd next(::openat(parent.value, component.c_str(),
                         O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC));
        if (next.value < 0)
            return {HandlerDescriptorReason::unsafeDirectory, {}};
        parent = std::move(next);
    }
    if (!safeDirectory(parent.value))
        return {HandlerDescriptorReason::unsafeDirectory, {}};

    Fd binary(::openat(parent.value, FixedHandlerName,
                      O_RDONLY | O_NOFOLLOW | O_NONBLOCK | O_CLOEXEC));
    if (binary.value < 0)
        return {HandlerDescriptorReason::missingBinary, {}};
    struct stat before{};
    if (::fstat(binary.value, &before) != 0)
        return {HandlerDescriptorReason::ioError, {}};
    if (!S_ISREG(before.st_mode) || before.st_uid != 0 ||
        (before.st_mode & 0022) != 0 || (before.st_mode & 0100) == 0 ||
        before.st_nlink != 1 || before.st_size <= 0 ||
        static_cast<std::uint64_t>(before.st_size) > MaximumBinaryBytes)
        return {HandlerDescriptorReason::unsafeBinary, {}};

    std::unique_ptr<EVP_MD_CTX, decltype(&EVP_MD_CTX_free)>
        ctx(EVP_MD_CTX_new(), EVP_MD_CTX_free);
    if (!ctx || EVP_DigestInit_ex(ctx.get(), EVP_sha256(), nullptr) != 1)
        return {HandlerDescriptorReason::ioError, {}};
    std::array<unsigned char, 65536> chunk{};
    std::uint64_t count = 0;
    while (true) {
        const ssize_t n = ::read(binary.value, chunk.data(), chunk.size());
        if (n < 0 && errno == EINTR) continue;
        if (n < 0) return {HandlerDescriptorReason::ioError, {}};
        if (n == 0) break;
        count += static_cast<std::uint64_t>(n);
        if (count > MaximumBinaryBytes ||
            EVP_DigestUpdate(ctx.get(), chunk.data(),
                             static_cast<std::size_t>(n)) != 1)
            return {HandlerDescriptorReason::ioError, {}};
    }
    std::array<unsigned char, EVP_MAX_MD_SIZE> digest{};
    unsigned length = 0;
    if (EVP_DigestFinal_ex(ctx.get(), digest.data(), &length) != 1 ||
        length != 32)
        return {HandlerDescriptorReason::ioError, {}};
    struct stat after{};
    if (::fstat(binary.value, &after) != 0 ||
        !sameFile(before, after) ||
        count != static_cast<std::uint64_t>(before.st_size))
        return {HandlerDescriptorReason::changedDuringCheck, {}};
    std::string actual;
    actual.reserve(64);
    constexpr char Hex[] = "0123456789abcdef";
    for (unsigned i = 0; i < length; ++i) {
        actual.push_back(Hex[digest[i] >> 4]);
        actual.push_back(Hex[digest[i] & 0x0f]);
    }
    if (actual != approvedSha256)
        return {HandlerDescriptorReason::digestMismatch, {}};
    if (::lseek(binary.value, 0, SEEK_SET) != 0)
        return {HandlerDescriptorReason::ioError, {}};
    return {HandlerDescriptorReason::verified,
            PinnedAddonHandler(binary.release())};
}
} // namespace vdrsuite::addons
