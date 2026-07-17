#include "VdrRecordingArtworkService.h"

#include "VdrRecordingArtworkIdentity.h"
#include "VdrRecordingCacheRepository.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <system_error>
#include <vector>

namespace
{

const std::string artworkPrefix =
    "/recording-artwork/";

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

VdrRecordingArtworkAsset readAllowedAsset(
    const std::vector<std::string>& roots,
    const std::string& reference,
    const std::size_t maximumFileSizeBytes)
{
    VdrRecordingArtworkAsset result;

    if (!isSafeRelativeReference(reference))
    {
        return result;
    }

    for (const std::string& configuredRoot : roots)
    {
        if (configuredRoot.empty())
        {
            continue;
        }

        std::error_code error;
        const std::filesystem::path canonicalRoot =
            std::filesystem::canonical(configuredRoot, error);

        if (error ||
            !std::filesystem::is_directory(canonicalRoot, error) ||
            error)
        {
            continue;
        }

        const std::filesystem::path requestedPath =
            canonicalRoot / std::filesystem::path(reference);
        const std::filesystem::path canonicalCandidate =
            std::filesystem::canonical(requestedPath, error);

        if (error ||
            !pathIsWithinRoot(canonicalRoot, canonicalCandidate) ||
            !std::filesystem::is_regular_file(canonicalCandidate, error) ||
            error)
        {
            continue;
        }

        const std::string contentType =
            contentTypeForPath(canonicalCandidate);

        if (contentType.empty())
        {
            continue;
        }

        const std::uintmax_t fileSize =
            std::filesystem::file_size(canonicalCandidate, error);

        if (error ||
            fileSize == 0 ||
            fileSize > maximumFileSizeBytes)
        {
            continue;
        }

        std::ifstream file(canonicalCandidate, std::ios::binary);

        if (!file)
        {
            continue;
        }

        std::string content(
            std::istreambuf_iterator<char>(file),
            std::istreambuf_iterator<char>());

        if (!file.eof() || content.size() != fileSize)
        {
            continue;
        }

        result.statusCode = 200;
        result.contentType = contentType;
        result.content = std::move(content);
        return result;
    }

    return result;
}

}

VdrRecordingArtworkService::VdrRecordingArtworkService(
    VdrRecordingCacheRepository& repository,
    std::vector<std::string> roots,
    const std::size_t maximumFileSizeBytes)
    : repository_(repository),
      roots_(std::move(roots)),
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

    const std::vector<VdrRecording> recordings =
        repository_.findAllForBackend(backendId);

    for (const VdrRecording& recording : recordings)
    {
        for (const VdrRecordingArtworkRef& artwork :
             recording.metadata.artwork)
        {
            if (!artwork.isValid() ||
                VdrRecordingArtworkIdentity::assetId(
                    recording,
                    artwork) != requestedAssetId)
            {
                continue;
            }

            return readAllowedAsset(
                roots_,
                artwork.reference,
                maximumFileSizeBytes_);
        }
    }

    return {};
}
