#include "PersistentSeriesArtworkFallbackResolver.h"

#include "EpgArtworkReference.h"
#include "EpgSeriesArtworkFallbackRepository.h"

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

bool validFallbackArtwork(const EpgScraperArtwork& artwork)
{
    return artwork.available &&
        artwork.origin == EpgScraperArtworkOrigin::ExternalFallback &&
        !artwork.provider.empty() &&
        artwork.provider != "none" &&
        artwork.provider != "tvscraper" &&
        !artwork.path.empty() &&
        artwork.width > 0 &&
        artwork.height > 0;
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
    reference.origin = EpgArtworkReferenceOrigin::ExternalFallback;
    reference.path = artwork.path;
    reference.width = artwork.width;
    reference.height = artwork.height;
    reference.resolvedAt = resolvedAt;
    return reference;
}

EpgScraperArtwork fallbackArtwork(const EpgArtworkReference& reference)
{
    EpgScraperArtwork artwork;
    if (!reference.valid() ||
        reference.origin != EpgArtworkReferenceOrigin::ExternalFallback ||
        reference.provider == "none" ||
        reference.provider == "tvscraper")
    {
        return artwork;
    }

    artwork.available = true;
    artwork.provider = reference.provider;
    artwork.origin = EpgScraperArtworkOrigin::ExternalFallback;
    artwork.managed = true;
    artwork.path = reference.path;
    artwork.width = reference.width;
    artwork.height = reference.height;
    return artwork;
}

EpgSeriesArtworkFallbackDeliveryConfig deliveryConfig(
    std::vector<std::string> managedRoots)
{
    EpgSeriesArtworkFallbackDeliveryConfig config;
    config.managedRoots = std::move(managedRoots);
    return config;
}
}

PersistentSeriesArtworkFallbackResolver::
PersistentSeriesArtworkFallbackResolver(
    IEpgScraperMetadataResolver& delegate,
    EpgSeriesArtworkFallbackRepository& repository,
    std::vector<std::string> allowedRoots)
    : delegate_(delegate),
      repository_(repository),
      allowedRoots_(allowedRoots),
      deliveryService_(
          repository,
          deliveryConfig(std::move(allowedRoots)))
{
}

EpgScraperMetadataResolution PersistentSeriesArtworkFallbackResolver::resolve(
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
        resolution.metadata.seriesArtworkFallback = EpgScraperArtwork{};
        return resolution;
    }

    const std::string normalizedBackend = normalizedBackendId(
        resolution.metadata.backendId.empty()
            ? backendId
            : resolution.metadata.backendId);
    const std::string channelId = resolution.metadata.channelId.empty()
        ? event.channelId
        : resolution.metadata.channelId;
    const std::string eventId = resolution.metadata.eventId.empty()
        ? event.id
        : resolution.metadata.eventId;
    EpgScraperArtwork& fallback =
        resolution.metadata.seriesArtworkFallback;

    // Provider and materializer output is never public-delivery eligible by
    // itself. Only this persisted boundary may assert managed eligibility.
    fallback.managed = false;

    if (validFallbackArtwork(fallback) &&
        EpgArtworkPathPolicy::isAllowedPath(fallback.path, allowedRoots_))
    {
        const EpgArtworkReference reference = referenceFor(
            normalizedBackend,
            channelId,
            eventId,
            fallback,
            epochSeconds());
        if (repository_.upsert(reference))
        {
            fallback.managed = true;
            return resolution;
        }
    }

    const EpgArtworkReference retained = repository_.find(
        normalizedBackend,
        channelId,
        eventId);
    if (retained.valid() &&
        retained.origin == EpgArtworkReferenceOrigin::ExternalFallback &&
        EpgArtworkPathPolicy::isAllowedPath(retained.path, allowedRoots_))
    {
        fallback = fallbackArtwork(retained);
        return resolution;
    }

    fallback = EpgScraperArtwork{};
    return resolution;
}

EpgSeriesArtworkFallbackAsset
PersistentSeriesArtworkFallbackResolver::loadSeriesArtworkFallback(
    const std::string& backendId,
    const std::string& channelId,
    const std::string& eventId) const
{
    return deliveryService_.loadSeriesArtworkFallback(
        backendId,
        channelId,
        eventId);
}
