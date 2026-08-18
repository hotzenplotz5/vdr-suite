#include "VdrRecordingArtworkService.h"

#include "VdrRecordingArtworkIdentity.h"
#include "VdrRecordingCacheRepository.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <map>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

namespace
{

const std::string artworkPrefix =
    "/recording-artwork/";

constexpr auto ArtworkIndexMissRefreshInterval =
    std::chrono::seconds(1);

int hexValue(
    const char character)
{
    if (character >= '0' && character <= '9')
    {
        return character - '0';
    }

    if (character >= 'a' && character <= 'f')
    {
        return 10 + character - 'a';
    }

    if (character >= 'A' && character <= 'F')
    {
        return 10 + character - 'A';
    }

    return -1;
}

bool percentDecodePathSegment(
    const std::string& value,
    std::string& decoded)
{
    decoded.clear();
    decoded.reserve(value.size());

    for (std::size_t index = 0; index < value.size(); ++index)
    {
        const char character = value[index];

        if (character == '%')
        {
            if (index + 2 >= value.size())
            {
                return false;
            }

            const int high = hexValue(value[index + 1]);
            const int low = hexValue(value[index + 2]);

            if (high < 0 || low < 0)
            {
                return false;
            }

            const char decodedCharacter =
                static_cast<char>((high * 16) + low);

            if (decodedCharacter == '/' ||
                decodedCharacter == '\\' ||
                decodedCharacter == '\0')
            {
                return false;
            }

            decoded.push_back(decodedCharacter);
            index += 2;
            continue;
        }

        if (character == '/' ||
            character == '\\' ||
            character == '\0')
        {
            return false;
        }

        decoded.push_back(character);
    }

    return !decoded.empty() && decoded.size() <= 128;
}

bool parseRequestPath(
    const std::string& requestPath,
    std::string& backendId,
    std::string& assetId)
{
    if (requestPath.compare(
            0,
            artworkPrefix.size(),
            artworkPrefix) != 0)
    {
        return false;
    }

    const std::string remainder =
        requestPath.substr(artworkPrefix.size());
    const std::size_t separator = remainder.find('/');

    if (separator == std::string::npos ||
        separator == 0 ||
        separator + 1 >= remainder.size() ||
        remainder.find('/', separator + 1) != std::string::npos)
    {
        return false;
    }

    if (!percentDecodePathSegment(
            remainder.substr(0, separator),
            backendId))
    {
        return false;
    }

    assetId = remainder.substr(separator + 1);
    return VdrRecordingArtworkIdentity::isValidAssetId(assetId);
}

bool isSafeRelativeReference(
    const std::string& reference)
{
    if (reference.empty() ||
        reference.front() == '/' ||
        reference.find('\\') != std::string::npos ||
        reference.find('%') != std::string::npos ||
        reference.find('\0') != std::string::npos)
    {
        return false;
    }

    std::size_t start = 0;

    while (start <= reference.size())
    {
        const std::size_t end = reference.find('/', start);
        const std::string segment = reference.substr(
            start,
            end == std::string::npos
                ? std::string::npos
                : end - start);

        if (segment.empty() || segment == "." || segment == "..")
        {
            return false;
        }

        if (end == std::string::npos)
        {
            break;
        }

        start = end + 1;
    }

    return true;
}

std::string lowerExtension(
    const std::filesystem::path& path)
{
    std::string extension = path.extension().string();

    std::transform(
        extension.begin(),
        extension.end(),
        extension.begin(),
        [](const unsigned char character)
        {
            return static_cast<char>(std::tolower(character));
        });

    return extension;
}

std::string contentTypeForPath(
    const std::filesystem::path& path)
{
    const std::string extension = lowerExtension(path);

    if (extension == ".jpg" || extension == ".jpeg")
    {
        return "image/jpeg";
    }

    if (extension == ".png")
    {
        return "image/png";
    }

    if (extension == ".webp")
    {
        return "image/webp";
    }

    return {};
}

unsigned char byteAt(
    const std::string& content,
    const std::size_t index)
{
    return static_cast<unsigned char>(content.at(index));
}

bool contentMatchesType(
    const std::string& contentType,
    const std::string& content)
{
    if (contentType == "image/jpeg")
    {
        return content.size() >= 3 &&
               byteAt(content, 0) == 0xff &&
               byteAt(content, 1) == 0xd8 &&
               byteAt(content, 2) == 0xff;
    }

    if (contentType == "image/png")
    {
        return content.size() >= 8 &&
               byteAt(content, 0) == 0x89 &&
               byteAt(content, 1) == 0x50 &&
               byteAt(content, 2) == 0x4e &&
               byteAt(content, 3) == 0x47 &&
               byteAt(content, 4) == 0x0d &&
               byteAt(content, 5) == 0x0a &&
               byteAt(content, 6) == 0x1a &&
               byteAt(content, 7) == 0x0a;
    }

    if (contentType == "image/webp")
    {
        return content.size() >= 12 &&
               content.compare(0, 4, "RIFF") == 0 &&
               content.compare(8, 4, "WEBP") == 0;
    }

    return false;
}

bool pathIsWithinRoot(
    const std::filesystem::path& root,
    const std::filesystem::path& candidate)
{
    auto rootIterator = root.begin();
    auto candidateIterator = candidate.begin();

    for (;
         rootIterator != root.end();
         ++rootIterator, ++candidateIterator)
    {
        if (candidateIterator == candidate.end() ||
            *candidateIterator != *rootIterator)
        {
            return false;
        }
    }

    return candidateIterator != candidate.end();
}

std::string referenceRelativeToConfiguredRoot(
    const std::string& configuredRoot,
    const std::string& reference)
{
    const std::string configuredRootReference =
        std::filesystem::path(configuredRoot)
            .lexically_normal()
            .relative_path()
            .generic_string();

    if (configuredRootReference.empty())
    {
        return reference;
    }

    const std::string repeatedRootPrefix =
        configuredRootReference + "/";

    if (reference.compare(
            0,
            repeatedRootPrefix.size(),
            repeatedRootPrefix) != 0)
    {
        return reference;
    }

    return reference.substr(repeatedRootPrefix.size());
}

VdrRecordingArtworkAsset readAllowedAsset(
    const std::string& configuredRoot,
    const std::string& reference,
    const std::size_t maximumFileSizeBytes)
{
    VdrRecordingArtworkAsset result;

    if (configuredRoot.empty() ||
        !std::filesystem::path(configuredRoot).is_absolute() ||
        !isSafeRelativeReference(reference))
    {
        return result;
    }

    std::error_code error;
    const std::filesystem::path canonicalRoot =
        std::filesystem::canonical(configuredRoot, error);

    if (error ||
        !std::filesystem::is_directory(canonicalRoot, error) ||
        error)
    {
        return result;
    }

    const std::string rootRelativeReference =
        referenceRelativeToConfiguredRoot(
            configuredRoot,
            reference);

    if (!isSafeRelativeReference(rootRelativeReference))
    {
        return result;
    }

    const std::filesystem::path requestedPath =
        canonicalRoot /
        std::filesystem::path(rootRelativeReference);
    const std::filesystem::path canonicalCandidate =
        std::filesystem::canonical(requestedPath, error);

    if (error ||
        !pathIsWithinRoot(canonicalRoot, canonicalCandidate) ||
        !std::filesystem::is_regular_file(canonicalCandidate, error) ||
        error)
    {
        return result;
    }

    const std::string contentType =
        contentTypeForPath(canonicalCandidate);

    if (contentType.empty())
    {
        return result;
    }

    const std::uintmax_t fileSize =
        std::filesystem::file_size(canonicalCandidate, error);

    if (error ||
        fileSize == 0 ||
        fileSize > maximumFileSizeBytes)
    {
        return result;
    }

    std::ifstream file(canonicalCandidate, std::ios::binary);

    if (!file)
    {
        return result;
    }

    std::string content{
        std::istreambuf_iterator<char>(file),
        std::istreambuf_iterator<char>()};

    if (file.bad() ||
        content.size() != static_cast<std::size_t>(fileSize) ||
        !contentMatchesType(contentType, content))
    {
        return result;
    }

    result.statusCode = 200;
    result.contentType = contentType;
    result.content = std::move(content);
    return result;
}

std::map<std::string, std::string> buildArtworkIndex(
    const std::vector<VdrRecording>& recordings)
{
    std::map<std::string, std::string> referencesByAssetId;

    for (const VdrRecording& recording : recordings)
    {
        for (const VdrRecordingArtworkRef& artwork : recording.metadata.artwork)
        {
            if (!artwork.isValid())
            {
                continue;
            }

            const std::string assetId =
                VdrRecordingArtworkIdentity::assetId(
                    recording,
                    artwork);

            if (!assetId.empty())
            {
                referencesByAssetId.emplace(
                    assetId,
                    artwork.reference);
            }
        }
    }

    return referencesByAssetId;
}

}

VdrRecordingArtworkService::VdrRecordingArtworkService(
    VdrRecordingCacheRepository& repository,
    std::map<std::string, std::string> rootsByBackend,
    const std::size_t maximumFileSizeBytes)
    : repository_(repository),
      rootsByBackend_(std::move(rootsByBackend)),
      maximumFileSizeBytes_(maximumFileSizeBytes)
{
}

bool VdrRecordingArtworkService::handlesPath(
    const std::string& requestPath) const
{
    return requestPath.compare(
        0,
        artworkPrefix.size(),
        artworkPrefix) == 0;
}

bool VdrRecordingArtworkService::resolveArtworkReference(
    const std::string& backendId,
    const std::string& assetId,
    std::string& reference) const
{
    const auto now = std::chrono::steady_clock::now();

    {
        std::lock_guard<std::mutex> lock(artworkIndexMutex_);
        const auto index = artworkIndexes_.find(backendId);

        if (index != artworkIndexes_.end() && index->second.initialized)
        {
            const auto found =
                index->second.referencesByAssetId.find(assetId);

            if (found != index->second.referencesByAssetId.end())
            {
                reference = found->second;
                return true;
            }

            if (now - index->second.rebuiltAt <
                ArtworkIndexMissRefreshInterval)
            {
                return false;
            }
        }
    }

    std::map<std::string, std::string> rebuilt =
        buildArtworkIndex(
            repository_.findAllForBackend(backendId));

    std::lock_guard<std::mutex> lock(artworkIndexMutex_);
    ArtworkLookupIndex& index = artworkIndexes_[backendId];
    index.initialized = true;
    index.rebuiltAt = std::chrono::steady_clock::now();
    index.referencesByAssetId = std::move(rebuilt);

    const auto found =
        index.referencesByAssetId.find(assetId);

    if (found == index.referencesByAssetId.end())
    {
        return false;
    }

    reference = found->second;
    return true;
}

VdrRecordingArtworkAsset VdrRecordingArtworkService::loadPath(
    const std::string& requestPath) const
{
    std::string backendId;
    std::string requestedAssetId;

    if (!parseRequestPath(
            requestPath,
            backendId,
            requestedAssetId))
    {
        return {};
    }

    const auto configuredRoot =
        rootsByBackend_.find(backendId);

    if (configuredRoot == rootsByBackend_.end())
    {
        return {};
    }

    std::string reference;
    if (!resolveArtworkReference(
            backendId,
            requestedAssetId,
            reference))
    {
        return {};
    }

    return readAllowedAsset(
        configuredRoot->second,
        reference,
        maximumFileSizeBytes_);
}
