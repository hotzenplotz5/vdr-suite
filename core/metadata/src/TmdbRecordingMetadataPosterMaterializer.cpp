#include "TmdbRecordingMetadataCandidateProvider.h"

#include "IExternalArtworkHttpTransport.h"

#include <algorithm>
#include <atomic>
#include <cerrno>
#include <cstdint>
#include <filesystem>
#include <fcntl.h>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

namespace
{
constexpr const char* ImageBase = "https://image.tmdb.org/t/p/w500";
std::atomic<unsigned long long> TemporaryCounter{0};

bool digits(const std::string& value)
{
    return !value.empty() && value.size() <= 16U &&
        std::all_of(value.begin(), value.end(), [](unsigned char character) {
            return character >= '0' && character <= '9';
        });
}

bool validNamespace(const std::string& value)
{
    return value == "movie" || value == "tv" ||
        value == "tv-season" || value == "tv-episode";
}

bool validPosterReference(const std::string& value)
{
    return value.size() >= 6U && value.size() <= 256U &&
        value.front() == '/' && value.find('/', 1U) == std::string::npos &&
        value.find("..") == std::string::npos &&
        std::all_of(value.begin() + 1U, value.end(), [](unsigned char character) {
            return (character >= 'A' && character <= 'Z') ||
                (character >= 'a' && character <= 'z') ||
                (character >= '0' && character <= '9') ||
                character == '_' || character == '-' || character == '.';
        });
}

std::uint64_t fnv1a(const std::string& value)
{
    std::uint64_t hash = 1469598103934665603ULL;
    for (const unsigned char byte : value)
    {
        hash ^= byte;
        hash *= 1099511628211ULL;
    }
    return hash;
}

std::string hex64(std::uint64_t value)
{
    static const char Hex[] = "0123456789abcdef";
    std::string output(16U, '0');
    for (int index = 15; index >= 0; --index)
    {
        output[static_cast<std::size_t>(index)] = Hex[value & 0x0fU];
        value >>= 4U;
    }
    return output;
}

std::string extensionFor(const std::string& contentType)
{
    if (contentType.compare(0, 10, "image/jpeg") == 0) return ".jpg";
    if (contentType.compare(0, 9, "image/png") == 0) return ".png";
    if (contentType.compare(0, 10, "image/webp") == 0) return ".webp";
    return {};
}

int openDirectoryNoFollow(const std::filesystem::path& path)
{
    if (!path.is_absolute() || path == path.root_path()) return -1;
    int current = ::open("/", O_RDONLY | O_DIRECTORY | O_CLOEXEC);
    if (current < 0) return -1;
    for (const auto& component : path.lexically_normal().relative_path())
    {
        const std::string name = component.string();
        if (name.empty() || name == "." || name == "..")
        {
            ::close(current);
            return -1;
        }
        const int next = ::openat(
            current,
            name.c_str(),
            O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
        ::close(current);
        if (next < 0) return -1;
        current = next;
    }
    return current;
}

bool writeAll(int descriptor, const std::string& body)
{
    std::size_t offset = 0;
    while (offset < body.size())
    {
        const ssize_t written = ::write(
            descriptor,
            body.data() + offset,
            body.size() - offset);
        if (written < 0)
        {
            if (errno == EINTR) continue;
            return false;
        }
        if (written == 0) return false;
        offset += static_cast<std::size_t>(written);
    }
    return true;
}

std::string existingPoster(
    const std::filesystem::path& root,
    const std::string& stem)
{
    for (const std::string extension : {".jpg", ".png", ".webp"})
    {
        const std::filesystem::path candidate = root / (stem + extension);
        std::error_code error;
        const auto status = std::filesystem::symlink_status(candidate, error);
        if (!error && std::filesystem::is_regular_file(status))
            return candidate.string();
    }
    return {};
}
}

std::string TmdbRecordingMetadataCandidateProvider::materializePoster(
    const std::string& externalNamespace,
    const std::string& externalId,
    const std::string& posterReference)
{
    const std::filesystem::path root =
        std::filesystem::path(config_.posterCacheRoot).lexically_normal();
    if (!configurationValid(config_) || !validNamespace(externalNamespace) ||
        !digits(externalId) || !validPosterReference(posterReference) ||
        config_.maximumImageBytes < 1024U ||
        config_.maximumImageBytes > 32U * 1024U * 1024U ||
        !root.is_absolute() || root == root.root_path())
        return {};

    std::error_code directoryError;
    std::filesystem::create_directories(root, directoryError);
    if (directoryError) return {};

    const std::string stem =
        "tmdb-" + externalNamespace + "-" + externalId + "-" +
        hex64(fnv1a(posterReference));
    const std::string existing = existingPoster(root, stem);
    if (!existing.empty()) return existing;

    ExternalArtworkHttpRequest request;
    request.url = std::string(ImageBase) + posterReference;
    request.accept = "image/jpeg,image/png,image/webp";
    request.connectTimeoutMs = config_.connectTimeoutMs;
    request.totalTimeoutMs = config_.totalTimeoutMs;
    request.maximumResponseBytes = config_.maximumImageBytes;
    const ExternalArtworkHttpResponse response = transport_.perform(request);
    const std::string extension = extensionFor(response.contentType);
    if (response.transportError || response.statusCode != 200L ||
        extension.empty() || response.body.empty() ||
        response.body.size() > config_.maximumImageBytes)
        return {};

    const int directory = openDirectoryNoFollow(root);
    if (directory < 0) return {};
    const std::string finalName = stem + extension;
    const std::string temporaryName =
        "." + finalName + "." + std::to_string(::getpid()) + "." +
        std::to_string(TemporaryCounter.fetch_add(1U)) + ".tmp";
    const int output = ::openat(
        directory,
        temporaryName.c_str(),
        O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC | O_NOFOLLOW,
        S_IRUSR | S_IWUSR);
    if (output < 0)
    {
        ::close(directory);
        return {};
    }

    bool ok = writeAll(output, response.body) && ::fsync(output) == 0;
    if (::close(output) != 0) ok = false;
    if (ok)
        ok = ::renameat(
            directory,
            temporaryName.c_str(),
            directory,
            finalName.c_str()) == 0;
    if (ok) ok = ::fsync(directory) == 0;
    if (!ok) ::unlinkat(directory, temporaryName.c_str(), 0);
    ::close(directory);
    return ok ? (root / finalName).string() : std::string{};
}
