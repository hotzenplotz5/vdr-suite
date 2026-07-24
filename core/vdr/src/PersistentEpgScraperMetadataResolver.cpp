#include "PersistentEpgScraperMetadataResolver.h"

#include "EpgArtworkReference.h"
#include "EpgArtworkRepository.h"

#include <chrono>

namespace
{
long long epochSeconds()
{
    return std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

std::string normalizedBackendId(const std::string& backendId)
{
    return backendId.empty() ? "default" : backendId;
}
}

PersistentEpgScraperMetadataResolver::PersistentEpgScraperMetadataResolver(
    IEpgScraperMetadataResolver& delegate,
    EpgArtworkRepository& artworkRepository)
    : delegate_(delegate),
      artworkRepository_(artworkRepository)
{
}

EpgScraperMetadataResolution PersistentEpgScraperMetadataResolver::resolve(
    const std::string& backendId,
    const VdrEvent& event)
{
    EpgScraperMetadataResolution resolution = delegate_.resolve(
        backendId,
        event);

    if (!resolution.attempted ||
        !resolution.found ||
        !resolution.metadata.valid() ||
        !resolution.metadata.preferredArtwork.valid())
    {
        return resolution;
    }

    const EpgScraperArtwork& preferred =
        resolution.metadata.preferredArtwork;

    EpgArtworkReference artwork;
    artwork.backendId = normalizedBackendId(backendId);
    artwork.channelId = resolution.metadata.channelId.empty()
        ? event.channelId
        : resolution.metadata.channelId;
    artwork.eventId = resolution.metadata.eventId.empty()
        ? event.id
        : resolution.metadata.eventId;
    artwork.provider = preferred.provider;
    artwork.path = preferred.path;
    artwork.width = preferred.width;
    artwork.height = preferred.height;
    artwork.resolvedAt = epochSeconds();

    // Persistence is best effort for the metadata response itself. A failed
    // write must not hide otherwise valid provider metadata, while a later
    // successful resolution can repair the cache.
    artworkRepository_.upsert(artwork);
    return resolution;
}
