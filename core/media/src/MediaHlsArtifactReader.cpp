#include "MediaHlsArtifactReader.h"

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <filesystem>
#include <fcntl.h>
#include <regex>
#include <sys/stat.h>
#include <unistd.h>
#include <utility>

#ifndef O_CLOEXEC
#define O_CLOEXEC 0
#endif

#ifndef O_NOFOLLOW
#define O_NOFOLLOW 0
#endif

namespace
{

bool safeWorkspaceId(const std::string& value)
{
    if (value.empty() || value.size() > 96) return false;
    return std::all_of(
        value.begin(), value.end(),
        [](unsigned char character) {
            return std::isalnum(character) || character == '-' || character == '_';
        });
}

bool hasSuffix(const std::string& value, const std::string& suffix)
{
    return value.size() >= suffix.size() &&
        value.compare(value.size() - suffix.size(), suffix.size(), suffix) == 0;
}

std::string contentTypeFor(const std::string& artifactName)
{
    if (artifactName == "master.m3u8") return "application/vnd.apple.mpegurl";
    if (artifactName == "init.mp4") return "video/mp4";
    if (hasSuffix(artifactName, ".m4s")) return "video/iso.segment";
    if (hasSuffix(artifactName, ".ts")) return "video/mp2t";
    return "application/octet-stream";
}

} // namespace

MediaHlsArtifactReader::MediaHlsArtifactReader(std::string workspaceRoot)
    : workspaceRoot_(std::move(workspaceRoot))
{
}

bool MediaHlsArtifactReader::allowedArtifactName(const std::string& artifactName)
{
    if (artifactName == "master.m3u8" || artifactName == "init.mp4") {
        return true;
    }

    static const std::regex SegmentPattern(
        R"(^segment-[0-9]{6}\.(m4s|ts)$)",
        std::regex::ECMAScript);
    return std::regex_match(artifactName, SegmentPattern);
}

MediaHlsArtifact MediaHlsArtifactReader::read(
    const std::string& workspaceId,
    const std::string& artifactName) const
{
    MediaHlsArtifact result;

    const std::filesystem::path root(workspaceRoot_);
    if (!root.is_absolute() || !safeWorkspaceId(workspaceId) ||
        !allowedArtifactName(artifactName)) {
        result.reasonCode = "invalid_media_artifact_request";
        return result;
    }

    const std::filesystem::path workspace = root / workspaceId;
    const std::filesystem::path artifact = workspace / artifactName;
    if (workspace.parent_path() != root || artifact.parent_path() != workspace) {
        result.reasonCode = "invalid_media_artifact_path";
        return result;
    }

    struct stat metadata{};
    if (lstat(artifact.c_str(), &metadata) != 0) {
        result.reasonCode = errno == ENOENT
            ? "media_artifact_not_ready"
            : "media_artifact_stat_failed";
        return result;
    }
    if (!S_ISREG(metadata.st_mode) || metadata.st_size < 0) {
        result.reasonCode = "media_artifact_not_regular";
        return result;
    }
    const std::size_t size = static_cast<std::size_t>(metadata.st_size);
    if (size == 0) {
        result.reasonCode = "media_artifact_not_ready";
        return result;
    }
    if (size > MaximumArtifactBytes) {
        result.reasonCode = "media_artifact_too_large";
        return result;
    }

    const int fd = open(artifact.c_str(), O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
    if (fd < 0) {
        result.reasonCode = "media_artifact_open_failed";
        return result;
    }

    result.bytes.resize(size);
    std::size_t offset = 0;
    while (offset < size) {
        const ssize_t count = ::read(fd, result.bytes.data() + offset, size - offset);
        if (count < 0 && errno == EINTR) continue;
        if (count <= 0) break;
        offset += static_cast<std::size_t>(count);
    }
    const int closeResult = close(fd);

    if (offset != size || closeResult != 0) {
        result.bytes.clear();
        result.reasonCode = "media_artifact_read_failed";
        return result;
    }

    result.found = true;
    result.contentType = contentTypeFor(artifactName);
    return result;
}