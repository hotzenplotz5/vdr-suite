#include "TmdbRecordingMetadataCredentialResolver.h"

#include <algorithm>
#include <cstdlib>
#include <fcntl.h>
#include <filesystem>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

namespace
{
constexpr const char* ManagedSecretRoot =
    "/var/lib/vdr-suite/secrets/series-artwork";

bool tokenSyntaxValid(const std::string& token)
{
    return !token.empty() && token.size() <= 4096U &&
        std::all_of(
            token.begin(),
            token.end(),
            [](unsigned char character)
            {
                return character > 0x20U && character != 0x7fU;
            });
}

std::string environmentToken()
{
    const char* value = std::getenv("VDR_SUITE_TMDB_READ_ACCESS_TOKEN");
    if (value == nullptr) return {};
    const std::string token(value);
    return tokenSyntaxValid(token) ? token : std::string{};
}

int openDirectoryNoFollow(const std::filesystem::path& path)
{
    const std::filesystem::path normalized = path.lexically_normal();
    if (!normalized.is_absolute() || normalized == normalized.root_path())
        return -1;

    int current = ::open("/", O_RDONLY | O_DIRECTORY | O_CLOEXEC);
    if (current < 0) return -1;

    for (const auto& component : normalized.relative_path())
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

    struct stat metadata{};
    if (::fstat(current, &metadata) != 0 ||
        !S_ISDIR(metadata.st_mode) ||
        (metadata.st_mode & 0077) != 0)
    {
        ::close(current);
        return -1;
    }

    return current;
}

std::string managedToken(const std::string& backendId)
{
    const int directory = openDirectoryNoFollow(ManagedSecretRoot);
    if (directory < 0) return {};

    const std::string filename = backendId + ".tmdb-token";
    const int descriptor = ::openat(
        directory,
        filename.c_str(),
        O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
    ::close(directory);
    if (descriptor < 0) return {};

    struct stat metadata{};
    if (::fstat(descriptor, &metadata) != 0 ||
        !S_ISREG(metadata.st_mode) ||
        (metadata.st_mode & 0077) != 0 ||
        metadata.st_size <= 0 || metadata.st_size > 4096)
    {
        ::close(descriptor);
        return {};
    }

    std::string token(static_cast<std::size_t>(metadata.st_size), '\0');
    std::size_t offset = 0;
    while (offset < token.size())
    {
        const ssize_t count = ::read(
            descriptor,
            &token[offset],
            token.size() - offset);
        if (count <= 0)
        {
            token.clear();
            break;
        }
        offset += static_cast<std::size_t>(count);
    }
    ::close(descriptor);

    return tokenSyntaxValid(token) ? token : std::string{};
}
}

bool TmdbRecordingMetadataCredentialResolver::validBackendId(
    const std::string& backendId)
{
    return !backendId.empty() && backendId.size() <= 128U &&
        std::all_of(
            backendId.begin(),
            backendId.end(),
            [](unsigned char character)
            {
                return (character >= 'a' && character <= 'z') ||
                    (character >= 'A' && character <= 'Z') ||
                    (character >= '0' && character <= '9') ||
                    character == '-' || character == '_' || character == '.';
            });
}

std::string TmdbRecordingMetadataCredentialResolver::resolveReadAccessToken(
    const std::string& backendId)
{
    if (!validBackendId(backendId)) return {};
    const std::string managed = managedToken(backendId);
    return managed.empty() ? environmentToken() : managed;
}
