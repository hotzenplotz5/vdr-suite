#include "EpgArtworkController.h"

#include "EpgArtworkReference.h"
#include "EpgArtworkRepository.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <utility>

namespace
{
constexpr std::uintmax_t kMaximumArtworkBytes = 32U * 1024U * 1024U;

std::string normalizeBackendId(const std::string& backendId)
{
    return backendId.empty() ? "default" : backendId;
}

std::string lowerAscii(std::string value)
{
    std::transform(
        value.begin(),
        value.end(),
        value.begin(),
        [](unsigned char character)
        {
            return static_cast<char>(std::tolower(character));
        });
    return value;
}

std::string contentTypeForPath(const std::filesystem::path& path)
{
    const std::string extension = lowerAscii(path.extension().string());

    if (extension == ".jpg" || extension == ".jpeg")
    {
        return "image/jpeg";
    }

    if (extension == ".png")
    {
        return "image/png";
    }

    return "";
}

bool isPathWithinRoot(
    const std::filesystem::path& path,
    const std::filesystem::path& root)
{
    auto pathIterator = path.begin();
    auto rootIterator = root.begin();

    for (; rootIterator != root.end(); ++rootIterator, ++pathIterator)
    {
        if (pathIterator == path.end() || *pathIterator != *rootIterator)
        {
            return false;
        }
    }

    return true;
}

bool resolveAllowedPath(
    const std::string& candidate,
    const std::vector<std::string>& allowedRoots,
    std::filesystem::path& resolvedPath)
{
    std::error_code error;
    const std::filesystem::path canonicalCandidate =
        std::filesystem::weakly_canonical(candidate, error);

    if (error || canonicalCandidate.empty() || !canonicalCandidate.is_absolute())
    {
        return false;
    }

    for (const std::string& configuredRoot : allowedRoots)
    {
        error.clear();
        const std::filesystem::path canonicalRoot =
            std::filesystem::weakly_canonical(configuredRoot, error);

        if (!error &&
            !canonicalRoot.empty() &&
            canonicalRoot.is_absolute() &&
            isPathWithinRoot(canonicalCandidate, canonicalRoot))
        {
            resolvedPath = canonicalCandidate;
            return true;
        }
    }

    return false;
}

ApiResponse jsonError(int statusCode, const std::string& message)
{
    ApiResponse response;
    response.statusCode = statusCode;
    response.contentType = "application/json";
    response.body = "{\"error\":\"" + message + "\"}";
    return response;
}
}

std::vector<std::string> EpgArtworkController::defaultAllowedRoots()
{
    return {
        "/var/cache/vdr/plugins/tvscraper",
        "/var/cache/vdr-suite/epg-artwork"
    };
}

EpgArtworkController::EpgArtworkController(EpgArtworkRepository& repository)
    : EpgArtworkController(repository, defaultAllowedRoots())
{
}

EpgArtworkController::EpgArtworkController(
    EpgArtworkRepository& repository,
    std::vector<std::string> allowedRoots)
    : repository_(repository),
      allowedRoots_(std::move(allowedRoots))
{
}

ApiResponse EpgArtworkController::serveValidatedPath(
    const std::string& candidate,
    const std::vector<std::string>& allowedRoots)
{
    std::filesystem::path resolvedPath;
    if (!resolveAllowedPath(candidate, allowedRoots, resolvedPath))
    {
        return jsonError(403, "epg artwork path is not allowed");
    }

    const std::string contentType = contentTypeForPath(resolvedPath);
    if (contentType.empty())
    {
        return jsonError(415, "epg artwork type is not supported");
    }

    std::error_code error;
    const std::uintmax_t fileSize = std::filesystem::file_size(resolvedPath, error);
    if (error)
    {
        return jsonError(404, "epg artwork file not found");
    }

    if (fileSize > kMaximumArtworkBytes)
    {
        return jsonError(413, "epg artwork file is too large");
    }

    std::ifstream file(resolvedPath, std::ios::binary);
    if (!file)
    {
        return jsonError(404, "epg artwork file not found");
    }

    std::ostringstream buffer;
    buffer << file.rdbuf();

    ApiResponse response;
    response.statusCode = 200;
    response.contentType = contentType;
    response.body = buffer.str();
    return response;
}

ApiResponse EpgArtworkController::getArtwork(
    const std::string& backendId,
    const std::string& channelId,
    const std::string& eventId) const
{
    if (channelId.empty() || eventId.empty())
    {
        return jsonError(400, "channelId and eventId are required");
    }

    const EpgArtworkReference artwork = repository_.find(
        normalizeBackendId(backendId),
        channelId,
        eventId);

    if (!artwork.valid())
    {
        return jsonError(404, "epg artwork not found");
    }

    return serveValidatedPath(artwork.path, allowedRoots_);
}
