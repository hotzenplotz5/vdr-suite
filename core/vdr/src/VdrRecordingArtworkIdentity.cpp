#include "VdrRecordingArtworkIdentity.h"

#include <cctype>
#include <cstdint>
#include <iomanip>
#include <sstream>
#include <string>

namespace
{

std::string artworkKindName(
    const VdrRecordingArtworkKind kind)
{
    switch (kind)
    {
    case VdrRecordingArtworkKind::Poster:
        return "poster";
    case VdrRecordingArtworkKind::Fanart:
        return "fanart";
    case VdrRecordingArtworkKind::Banner:
        return "banner";
    case VdrRecordingArtworkKind::Still:
        return "still";
    }

    return "unknown";
}

std::string normalizedBackendId(
    const VdrRecording& recording)
{
    return recording.backendId.empty()
        ? "default"
        : recording.backendId;
}

std::uint64_t fnv1a64(
    const std::string& value,
    std::uint64_t hash)
{
    for (const unsigned char character : value)
    {
        hash ^= static_cast<std::uint64_t>(character);
        hash *= 1099511628211ULL;
    }

    return hash;
}

std::string stableArtworkInput(
    const VdrRecording& recording,
    const VdrRecordingArtworkRef& artwork)
{
    std::ostringstream value;
    value
        << "vdr-suite-recording-artwork-v1\n"
        << normalizedBackendId(recording) << '\n'
        << recording.id << '\n'
        << recording.backendNativeId << '\n'
        << recording.path << '\n'
        << artworkKindName(artwork.kind) << '\n'
        << vdrRecordingMetadataSourceName(artwork.source) << '\n'
        << artwork.reference << '\n'
        << artwork.width << 'x' << artwork.height;
    return value.str();
}

std::string percentEncodePathSegment(
    const std::string& value)
{
    std::ostringstream encoded;
    encoded << std::uppercase << std::hex;

    for (const unsigned char character : value)
    {
        if (std::isalnum(character) ||
            character == '-' ||
            character == '_' ||
            character == '.' ||
            character == '~')
        {
            encoded << static_cast<char>(character);
        }
        else
        {
            encoded
                << '%'
                << std::setw(2)
                << std::setfill('0')
                << static_cast<int>(character);
        }
    }

    return encoded.str();
}

int artworkPreference(
    const VdrRecordingArtworkKind kind)
{
    switch (kind)
    {
    case VdrRecordingArtworkKind::Poster:
        return 0;
    case VdrRecordingArtworkKind::Still:
        return 1;
    case VdrRecordingArtworkKind::Fanart:
        return 2;
    case VdrRecordingArtworkKind::Banner:
        return 3;
    }

    return 4;
}

}

std::string VdrRecordingArtworkIdentity::assetId(
    const VdrRecording& recording,
    const VdrRecordingArtworkRef& artwork)
{
    if (!artwork.isValid())
    {
        return {};
    }

    const std::string input =
        stableArtworkInput(recording, artwork);

    const std::uint64_t first =
        fnv1a64(input, 14695981039346656037ULL);
    const std::uint64_t second =
        fnv1a64(input, 7809847782465536322ULL);

    std::ostringstream id;
    id
        << std::hex
        << std::setfill('0')
        << std::setw(16)
        << first
        << std::setw(16)
        << second;

    return id.str();
}

const VdrRecordingArtworkRef*
VdrRecordingArtworkIdentity::preferredArtwork(
    const VdrRecording& recording)
{
    const VdrRecordingArtworkRef* preferred = nullptr;
    int preferredRank = 5;

    for (const VdrRecordingArtworkRef& artwork :
         recording.metadata.artwork)
    {
        if (!artwork.isValid())
        {
            continue;
        }

        const int rank = artworkPreference(artwork.kind);

        if (rank < preferredRank)
        {
            preferred = &artwork;
            preferredRank = rank;
        }
    }

    return preferred;
}

std::string VdrRecordingArtworkIdentity::publicUrl(
    const VdrRecording& recording,
    const VdrRecordingArtworkRef& artwork)
{
    const std::string id = assetId(recording, artwork);

    if (id.empty())
    {
        return {};
    }

    return
        "/recording-artwork/" +
        percentEncodePathSegment(normalizedBackendId(recording)) +
        "/" +
        id;
}

bool VdrRecordingArtworkIdentity::isValidAssetId(
    const std::string& assetId)
{
    if (assetId.size() != 32)
    {
        return false;
    }

    for (const unsigned char character : assetId)
    {
        if (!std::isxdigit(character))
        {
            return false;
        }
    }

    return true;
}
