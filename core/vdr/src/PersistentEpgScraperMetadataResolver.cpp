#include "PersistentEpgScraperMetadataResolver.h"

#include "EpgArtworkReference.h"
#include "EpgArtworkRepository.h"

#include <chrono>
#include <utility>

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

EpgScraperArtwork scraperArtwork(const EpgArtworkReference& reference)
{
    EpgScraperArtwork artwork;
    if (!reference.valid()) return artwork;
    artwork.available = true;
    artwork.provider = reference.provider;
    artwork.origin = EpgScraperArtworkOrigin::PrimaryMetadata;
    artwork.path = reference.path;
    artwork.width = reference.width;
    artwork.height = reference.height;
    return artwork;
}

EpgArtworkReference referenceFor(
    const std::string& backendId,
    const std::string& channelId,
    const std::string& eventId,
    const EpgScraperArtwork& artwork,
    long long resolvedAt)
{
    EpgArtworkReference reference;
    reference.backendId = backendId;
    reference.channelId = channelId;
    reference.eventId = eventId;
    reference.provider = artwork.provider;
    reference.path = artwork.path;
    reference.width = artwork.width;
    reference.height = artwork.height;
    reference.resolvedAt = resolvedAt;
    return reference;
}
}

PersistentEpgScraperMetadataResolver::PersistentEpgScraperMetadataResolver(
    IEpgScraperMetadataResolver& delegate,
    EpgArtworkRepository& artworkRepository,
    std::vector<std::string> allowedRoots)
    : delegate_(delegate),
      artworkRepository_(artworkRepository),
      allowedRoots_(std::move(allowedRoots))
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
        !resolution.metadata.valid())
    {
        return resolution;
    }

    const std::string normalizedBackend = normalizedBackendId(backendId);
    const std::string channelId = resolution.metadata.channelId.empty()
        ? event.channelId
        : resolution.metadata.channelId;
    const std::string eventId = resolution.metadata.eventId.empty()
        ? event.id
        : resolution.metadata.eventId;
    const long long resolvedAt = epochSeconds();

    EpgScraperMetadataResolution persisted = resolution;
    persisted.metadata.backendId = normalizedBackend;
    persisted.metadata.channelId = channelId;
    persisted.metadata.eventId = eventId;

    const auto persistArtwork = [this, &normalizedBackend, &channelId, &eventId, resolvedAt](
        EpgScraperArtwork& artwork,
        const std::string& kind,
        int imageIndex,
        bool preferred)
    {
        if (artwork.valid() &&
            EpgArtworkPathPolicy::isAllowedPath(artwork.path, allowedRoots_))
        {
            const EpgArtworkReference reference = referenceFor(
                normalizedBackend,
                channelId,
                eventId,
                artwork,
                resolvedAt);
            artworkRepository_.upsertMetadataImage(
                normalizedBackend,
                channelId,
                eventId,
                kind,
                imageIndex,
                reference);
            if (preferred) artworkRepository_.upsert(reference);
            return;
        }

        EpgArtworkReference retained = artworkRepository_.findMetadataImage(
            normalizedBackend,
            channelId,
            eventId,
            kind,
            imageIndex);
        if (preferred && !retained.valid())
        {
            retained = artworkRepository_.find(
                normalizedBackend,
                channelId,
                eventId);
        }

        if (retained.valid() &&
            EpgArtworkPathPolicy::isAllowedPath(retained.path, allowedRoots_))
        {
            artwork = scraperArtwork(retained);
            return;
        }

        artwork = EpgScraperArtwork{};
    };

    persistArtwork(
        persisted.metadata.preferredArtwork,
        "preferred",
        0,
        true);

    for (std::size_t index = 0;
         index < persisted.metadata.people.size();
         ++index)
    {
        persistArtwork(
            persisted.metadata.people[index].image,
            "person",
            static_cast<int>(index),
            false);
    }

    for (std::size_t index = 0;
         index < persisted.metadata.images.size();
         ++index)
    {
        persistArtwork(
            persisted.metadata.images[index].artwork,
            "gallery",
            static_cast<int>(index),
            false);
    }

    const std::string publicJson = serializer_.serialize(persisted);
    if (artworkRepository_.upsertMetadataJson(
            normalizedBackend,
            channelId,
            eventId,
            publicJson,
            resolvedAt))
    {
        artworkRepository_.replaceMetadataPeople(
            normalizedBackend,
            channelId,
            eventId,
            persisted.metadata.people);
    }

    return persisted;
}
