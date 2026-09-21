#include "ArtworkPreviewCache.h"
#include "MediaProcessRunner.h"

#include <algorithm>
#include <cerrno>
#include <cstdint>
#include <dirent.h>
#include <fcntl.h>
#include <openssl/evp.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>
#include <utility>
#include <vector>

namespace {
constexpr std::size_t MaxInput = 16 * 1024 * 1024;
constexpr std::size_t MaxOutput = 512 * 1024;
constexpr std::size_t MaxCache = 32 * 1024 * 1024;
constexpr std::size_t MaxEntries = 256;
struct Fd {
    int value;
    explicit Fd(int fd) : value(fd) {}
    ~Fd() { if (value >= 0) ::close(value); }
    Fd(const Fd&) = delete;
    Fd& operator=(const Fd&) = delete;
};
struct Dimensions { unsigned width = 0, height = 0; };
unsigned byte(const std::string& s, std::size_t i) {
    return static_cast<unsigned char>(s[i]);
}
unsigned be16(const std::string& s, std::size_t i) {
    return (byte(s, i) << 8) | byte(s, i + 1);
}
unsigned be32(const std::string& s, std::size_t i) {
    return (be16(s, i) << 16) | be16(s, i + 2);
}
unsigned le24(const std::string& s, std::size_t i) {
    return byte(s, i) | (byte(s, i + 1) << 8) | (byte(s, i + 2) << 16);
}
// Header inspection never decodes pixels. The subprocess limits remain the
// backstop for malformed files, including inconsistent later image headers.
Dimensions dimensions(const std::string& s, const std::string& type) {
    if (type == "image/png" && s.size() >= 24 &&
        s.compare(0, 8, "\x89PNG\r\n\x1a\n", 8) == 0 &&
        be32(s, 8) == 13 && s.compare(12, 4, "IHDR") == 0)
        return {be32(s, 16), be32(s, 20)};
    if (type == "image/jpeg" && s.size() >= 4 && byte(s, 0) == 255 && byte(s, 1) == 216) {
        std::size_t i = 2;
        while (i + 4 <= s.size()) {
            if (byte(s, i++) != 255) return {};
            while (i < s.size() && byte(s, i) == 255) ++i;
            if (i + 3 > s.size()) return {};
            const unsigned marker = byte(s, i++);
            if (marker == 218 || marker == 217) return {};
            const unsigned length = be16(s, i);
            if (length < 2 || length > s.size() - i) return {};
            if (marker == 192 || marker == 193 || marker == 194) {
                if (length < 8) return {};
                return {be16(s, i + 5), be16(s, i + 3)};
            }
            i += length;
        }
    }
    if (type == "image/webp" && s.size() >= 30 &&
        s.compare(0, 4, "RIFF") == 0 && s.compare(8, 4, "WEBP") == 0) {
        if (s.compare(12, 4, "VP8X") == 0) {
            if (byte(s, 20) & 2) return {}; // Animated WebP stays original.
            return {le24(s, 24) + 1, le24(s, 27) + 1};
        }
        if (s.compare(12, 4, "VP8 ") == 0 && s.compare(23, 3, "\x9d\x01\x2a", 3) == 0)
            return {(byte(s, 26) | (byte(s, 27) << 8)) & 16383,
                    (byte(s, 28) | (byte(s, 29) << 8)) & 16383};
        if (s.compare(12, 4, "VP8L") == 0 && byte(s, 20) == 47)
            return {1 + (byte(s, 21) | ((byte(s, 22) & 63) << 8)),
                    1 + ((byte(s, 22) >> 6) | (byte(s, 23) << 2) | ((byte(s, 24) & 15) << 10))};
    }
    return {};
}
std::string cacheKey(const std::string& identity, const std::string& type, const std::string& source) {
    EVP_MD_CTX* context = EVP_MD_CTX_new();
    if (!context) return {};
    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned length = 0;
    const char separator = 0;
    const bool ok = EVP_DigestInit_ex(context, EVP_sha256(), nullptr) == 1 &&
        EVP_DigestUpdate(context, "home-400x600-jpeg-v1", sizeof("home-400x600-jpeg-v1") - 1) == 1 &&
        EVP_DigestUpdate(context, identity.data(), identity.size()) == 1 &&
        EVP_DigestUpdate(context, &separator, 1) == 1 &&
        EVP_DigestUpdate(context, type.data(), type.size()) == 1 &&
        EVP_DigestUpdate(context, &separator, 1) == 1 &&
        EVP_DigestUpdate(context, source.data(), source.size()) == 1 &&
        EVP_DigestFinal_ex(context, digest, &length) == 1;
    EVP_MD_CTX_free(context);
    if (!ok || length != 32) return {};
    std::string key;
    for (unsigned i = 0; i < length; ++i) {
        key += "0123456789abcdef"[digest[i] >> 4];
        key += "0123456789abcdef"[digest[i] & 15];
    }
    return key + ".jpg";
}
bool regular(const struct stat& st) {
    return S_ISREG(st.st_mode) && st.st_uid == ::geteuid() && st.st_nlink == 1;
}
std::string readCache(int directory, const std::string& key) {
    Fd fd(::openat(directory, key.c_str(), O_RDONLY | O_NONBLOCK | O_NOFOLLOW | O_CLOEXEC));
    struct stat st{};
    if (fd.value < 0 || ::fstat(fd.value, &st) != 0 || !regular(st) ||
        st.st_size <= 0 || st.st_size > static_cast<off_t>(MaxOutput)) return {};
    std::string data(static_cast<std::size_t>(st.st_size), '\0');
    std::size_t offset = 0;
    while (offset < data.size()) {
        const ssize_t n = ::read(fd.value, data.data() + offset, data.size() - offset);
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) return {};
        offset += static_cast<std::size_t>(n);
    }
    const auto size = dimensions(data, "image/jpeg");
    return size.width && size.height && size.width <= 400 && size.height <= 600 ? data : std::string{};
}
bool writeAll(int fd, const std::string& data) {
    std::size_t offset = 0;
    while (offset < data.size()) {
        const ssize_t n = ::write(fd, data.data() + offset, data.size() - offset);
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) return false;
        offset += static_cast<std::size_t>(n);
    }
    return true;
}
bool makeRoom(int directory, std::size_t incoming) {
    // A separate open file description keeps repeated scans independent.
    const int scan = ::openat(directory, ".", O_RDONLY | O_DIRECTORY | O_CLOEXEC);
    DIR* dir = scan < 0 ? nullptr : ::fdopendir(scan);
    if (!dir) { if (scan >= 0) ::close(scan); return false; }
    struct Entry { std::string name; std::size_t bytes; time_t modified; };
    std::vector<Entry> entries;
    std::size_t total = incoming, scanned = 0;
    bool safe = true;
    while (const dirent* entry = ::readdir(dir)) {
        if (++scanned > MaxEntries + 8) { safe = false; break; }
        const std::string name(entry->d_name);
        if (name.size() != 68 || name.substr(64) != ".jpg" ||
            name.substr(0, 64).find_first_not_of("0123456789abcdef") != std::string::npos) continue;
        struct stat st{};
        if (::fstatat(directory, name.c_str(), &st, AT_SYMLINK_NOFOLLOW) != 0 || !regular(st) || st.st_size < 0) {
            safe = false; break;
        }
        total += static_cast<std::size_t>(st.st_size);
        entries.push_back({name, static_cast<std::size_t>(st.st_size), st.st_mtime});
    }
    ::closedir(dir);
    if (!safe) return false;
    std::sort(entries.begin(), entries.end(), [](const Entry& a, const Entry& b) { return a.modified < b.modified; });
    std::size_t count = entries.size();
    for (const auto& entry : entries) {
        if (total <= MaxCache && count < MaxEntries) break;
        if (::unlinkat(directory, entry.name.c_str(), 0) != 0) return false;
        total -= entry.bytes;
        --count;
    }
    return total <= MaxCache && count < MaxEntries;
}
} // namespace

ArtworkPreviewCache::ArtworkPreviewCache(std::string directory, std::string executable)
    : directory_(std::move(directory)), executable_(std::move(executable)) {}

std::string ArtworkPreviewCache::home(const std::string& identity, const std::string& type,
                                     const std::string& source) const {
    if (identity.empty() || identity.size() > 8192 || source.empty() || source.size() > MaxInput) return {};
    const Dimensions size = dimensions(source, type);
    if (!size.width || !size.height || size.width > 16384 || size.height > 16384 ||
        static_cast<std::uint64_t>(size.width) * size.height > 40000000 ||
        (size.width <= 400 && size.height <= 600)) return {};
    const std::string key = cacheKey(identity, type, source);
    if (key.empty()) return {};
    if (directory_.empty() || directory_.front() != '/') return {};
    if (::mkdir(directory_.c_str(), 0700) != 0 && errno != EEXIST) return {};
    Fd directory(::open(directory_.c_str(), O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC));
    struct stat st{};
    if (directory.value < 0 || ::fstat(directory.value, &st) != 0 || st.st_uid != ::geteuid() ||
        (st.st_mode & 0077) != 0) return {};

    // Published preview files are immutable and appear atomically via renameat().
    // Serve an already-valid cache hit without contending with an unrelated
    // in-flight conversion.
    std::string cached = readCache(directory.value, key);
    if (!cached.empty()) return cached;

    std::unique_lock<std::mutex> lock(mutex_, std::try_to_lock);
    if (!lock.owns_lock()) return {}; // No request queue or parallel decoder storm.
    if (::flock(directory.value, LOCK_EX | LOCK_NB) != 0) return {};

    // Another process may have published this identity between the optimistic
    // read above and acquisition of the directory conversion lock.
    cached = readCache(directory.value, key);
    if (!cached.empty()) return cached;

    const auto now = std::chrono::steady_clock::now();
    const auto failed = failures_.find(key);
    if (failed != failures_.end() && failed->second > now) return {};
    auto fail = [&]() -> std::string {
        if (failures_.size() >= MaxEntries) failures_.erase(failures_.begin());
        failures_[key] = now + std::chrono::seconds(30);
        return {};
    };
    // Fixed temporary names are safe under the directory lock; no symlinks are
    // followed. Crash leftovers are removed only within this private directory.
    ::unlinkat(directory.value, ".input", 0);
    Fd input(::openat(directory.value, ".input", O_RDWR | O_CREAT | O_EXCL | O_NOFOLLOW | O_CLOEXEC, 0600));
    if (input.value < 0) return fail();
    ::unlinkat(directory.value, ".input", 0);
    if (!writeAll(input.value, source)) return fail();
    const double scale = std::min(400.0 / size.width, 600.0 / size.height);
    const unsigned width = std::max(1u, static_cast<unsigned>(size.width * scale));
    const unsigned height = std::max(1u, static_cast<unsigned>(size.height * scale));
    const std::string format = type == "image/jpeg" ? "jpeg_pipe" : type == "image/png" ? "png_pipe" : "webp_pipe";
    const auto result = MediaProcessRunner{}.runAndCapture({
        executable_, "-nostdin", "-v", "error", "-max_alloc", "134217728",
        "-threads", "1", "-filter_threads", "1", "-protocol_whitelist", "file,pipe",
        "-f", format, "-i",
        "/proc/" + std::to_string(::getpid()) + "/fd/" + std::to_string(input.value),
        "-map", "0:v:0", "-frames:v", "1", "-an", "-sn", "-dn", "-map_metadata", "-1",
        "-vf", "scale=" + std::to_string(width) + ":" + std::to_string(height) + ",setsar=1",
        "-threads", "1", "-c:v", "mjpeg", "-q:v", "4", "-pix_fmt", "yuvj420p",
        "-f", "image2pipe", "pipe:1"}, "/", std::chrono::seconds(3), MaxOutput,
        {768 * 1024 * 1024, 2});
    const auto outputSize = dimensions(result.output, "image/jpeg");
    if (!result.success || outputSize.width != width || outputSize.height != height ||
        result.output.size() >= source.size()) return fail();
    if (!makeRoom(directory.value, result.output.size())) return fail();
    ::unlinkat(directory.value, ".output", 0);
    Fd output(::openat(directory.value, ".output", O_WRONLY | O_CREAT | O_EXCL | O_NOFOLLOW | O_CLOEXEC, 0600));
    if (output.value < 0) return fail();
    const bool stored = writeAll(output.value, result.output) &&
        ::renameat(directory.value, ".output", directory.value, key.c_str()) == 0;
    ::unlinkat(directory.value, ".output", 0);
    if (!stored) return fail();
    failures_.erase(key);
    return result.output;
}
