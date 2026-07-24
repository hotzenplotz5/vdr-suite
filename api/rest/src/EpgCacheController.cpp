#include "EpgCacheController.h"

#include "EpgArtworkPublicJsonSerializer.h"
#include "EpgArtworkReference.h"
#include "EpgArtworkRepository.h"
#include "EpgCacheService.h"
#include "EpgCacheServiceRegistry.h"
#include "IEpgScraperMetadataResolver.h"
#include "VdrEvent.h"

#include <chrono>
#include <iomanip>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace
{
std::string normalizeBackendId(const std::string& backendId)
{
    return backendId.empty() ? "default" : backendId;
}

long long epochSeconds()
{
    return std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

std::string escapeJsonString(const std::string& value)
{
    std::ostringstream escaped;
    for (unsigned char character : value)
    {
        switch (character)
        {
        case '"': escaped << "\\\""; break;
        case '\\': escaped << "\\\\"; break;
        case '\n': escaped << "\\n"; break;
        case '\r': escaped << "\\r"; break;
        case '\t': escaped << "\\t"; break;
        default:
            if (character < 0x20)
            {
                escaped << "\\u" << std::hex << std::setw(4)
                        << std::setfill('0') << static_cast<int>(character)
                        << std::dec << std::setfill(' ');
            }
            else
            {
                escaped << static_cast<char>(character);
            }
            break;
        }
    }
    return escaped.str();
}

const char* boolJson(bool value)
{
    return value ? "true" : "false";
}

ApiResponse jsonResponse(int statusCode, const std::string& body)
{
    ApiResponse response;
    response.statusCode = statusCode;
    response.contentType = "application/json";
    response.body = body;
    return response;
}

ApiResponse jsonError(int statusCode, const std::string& message)
{
    return jsonResponse(
        statusCode,
        "{\"error\":\"" + escapeJsonString(message) + "\"}");
}

std::string serializeArtwork(
    const std::string& backendId,
    const VdrEvent& event,
    EpgArtworkRepository* repository,
    EpgArtworkPublicJsonSerializer* serializer)
{
    if (repository == nullptr || serializer == nullptr)
    {
        return "{\"available\":false}";
    }
    return serializer->serialize(repository->find(
        backendId,
        event.channelId,
        event.id));
}

std::string serializeEvent(
    const std::string& backendId,
    const VdrEvent& event,
    EpgArtworkRepository* repository,
    EpgArtworkPublicJsonSerializer* serializer)
{
    std::ostringstream json;
    json << "{"
         << "\"id\":\"" << escapeJsonString(event.id) << "\","
         << "\"channelId\":\"" << escapeJsonString(event.channelId) << "\","
         << "\"title\":\"" << escapeJsonString(event.title) << "\","
         << "\"subtitle\":\"" << escapeJsonString(event.subtitle) << "\","
         << "\"description\":\"" << escapeJsonString(event.description) << "\","
         << "\"startTime\":\"" << escapeJsonString(event.startTime) << "\","
         << "\"endTime\":\"" << escapeJsonString(event.endTime) << "\","
         << "\"durationSeconds\":" << event.durationSeconds << ','
         << "\"parentalRating\":" << event.parentalRating << ','
         << "\"artwork\":"
         << serializeArtwork(backendId, event, repository, serializer)
         << '}';
    return json.str();
}

std::string serializeEvents(
    const std::string& backendId,
    const std::vector<VdrEvent>& events,
    EpgArtworkRepository* repository,
    EpgArtworkPublicJsonSerializer* serializer)
{
    std::ostringstream json;
    json << "{\"backendId\":\"" << escapeJsonString(backendId)
         << "\",\"eventCount\":" << events.size() << ",\"events\":[";
    for (std::size_t index = 0; index < events.size(); ++index)
    {
        if (index > 0) json << ',';
        json << serializeEvent(backendId, events[index], repository, serializer);
    }
    json << "]}";
    return json.str();
}

std::string serializeRefreshResult(
    const std::string& backendId,
    const EpgCacheRefreshResult& result)
{
    std::ostringstream json;
    json << "{\"backendId\":\"" << escapeJsonString(backendId) << "\","
         << "\"accepted\":" << boolJson(result.accepted) << ','
         << "\"fetched\":" << boolJson(result.fetched) << ','
         << "\"stored\":" << boolJson(result.stored) << ','
         << "\"eventCount\":" << result.eventCount << '}';
    return json.str();
}

std::string serializeStatus(const EpgCacheStatus& status)
{
    std::ostringstream json;
    json << "{\"backendId\":\"" << escapeJsonString(status.backendId) << "\","
         << "\"ready\":" << boolJson(status.ready) << ','
         << "\"eventCount\":" << status.eventCount << ','
         << "\"lastRefreshKnown\":" << boolJson(status.lastRefreshKnown) << ','
         << "\"lastRefreshAccepted\":" << boolJson(status.lastRefreshAccepted) << ','
         << "\"lastRefreshFetched\":" << boolJson(status.lastRefreshFetched) << ','
         << "\"lastRefreshStored\":" << boolJson(status.lastRefreshStored) << ','
         << "\"lastRefreshEventCount\":" << status.lastRefreshEventCount << ','
         << "\"lastRefreshStartedAt\":" << status.lastRefreshStartedAt << ','
         << "\"lastRefreshFinishedAt\":" << status.lastRefreshFinishedAt << ','
         << "\"lastRefreshDurationMs\":" << status.lastRefreshDurationMs << ','
         << "\"lastError\":\"" << escapeJsonString(status.lastError) << "\"}";
    return json.str();
}

std::string serializeBackendNotFound(const std::string& backendId)
{
    return "{\"backendId\":\"" + escapeJsonString(backendId) +
        "\",\"status\":\"backend-not-found\",\"eventCount\":0,\"events\":[]}";
}

VdrEvent metadataEvent(const std::string& channelId, const std::string& eventId)
{
    VdrEvent event;
    event.channelId = channelId;
    event.id = eventId;
    return event;
}

const EpgScraperArtwork* selectedMetadataArtwork(
    const EpgScraperMetadata& metadata,
    const std::string& kind,
    int index)
{
    if (kind == "preferred")
    {
        return index == 0 ? &metadata.preferredArtwork : nullptr;
    }
    if (kind == "person")
    {
        return index >= 0 && static_cast<std::size_t>(index) < metadata.people.size()
            ? &metadata.people[static_cast<std::size_t>(index)].image
            : nullptr;
    }
    if (kind == "gallery")
    {
        return index >= 0 && static_cast<std::size_t>(index) < metadata.images.size()
            ? &metadata.images[static_cast<std::size_t>(index)].artwork
            : nullptr;
    }
    return nullptr;
}

EpgArtworkReference persistentArtwork(
    const std::string& backendId,
    const std::string& channelId,
    const std::string& eventId,
    const EpgScraperArtwork& source,
    long long resolvedAt)
{
    EpgArtworkReference artwork;
    artwork.backendId = backendId;
    artwork.channelId = channelId;
    artwork.eventId = eventId;
    artwork.provider = source.provider;
    artwork.path = source.path;
    artwork.width = source.width;
    artwork.height = source.height;
    artwork.resolvedAt = resolvedAt;
    return artwork;
}

void persistMetadataResolution(
    EpgArtworkRepository* repository,
    const EpgScraperMetadataPublicJsonSerializer& serializer,
    const EpgScraperMetadataResolution& resolution)
{
    if (repository == nullptr || !resolution.attempted || !resolution.found ||
        !resolution.metadata.valid())
    {
        return;
    }

    const EpgScraperMetadata& metadata = resolution.metadata;
    const long long resolvedAt = epochSeconds();
    const std::string publicJson = serializer.serialize(resolution);
    repository->upsertMetadataJson(
        metadata.backendId,
        metadata.channelId,
        metadata.eventId,
        publicJson,
        resolvedAt);

    if (metadata.preferredArtwork.valid())
    {
        const EpgArtworkReference artwork = persistentArtwork(
            metadata.backendId,
            metadata.channelId,
            metadata.eventId,
            metadata.preferredArtwork,
            resolvedAt);
        repository->upsert(artwork);
        repository->upsertMetadataImage(
            metadata.backendId,
            metadata.channelId,
            metadata.eventId,
            "preferred",
            0,
            artwork);
    }

    for (std::size_t index = 0; index < metadata.people.size(); ++index)
    {
        if (!metadata.people[index].image.valid()) continue;
        repository->upsertMetadataImage(
            metadata.backendId,
            metadata.channelId,
            metadata.eventId,
            "person",
            static_cast<int>(index),
            persistentArtwork(
                metadata.backendId,
                metadata.channelId,
                metadata.eventId,
                metadata.people[index].image,
                resolvedAt));
    }

    for (std::size_t index = 0; index < metadata.images.size(); ++index)
    {
        if (!metadata.images[index].artwork.valid()) continue;
        repository->upsertMetadataImage(
            metadata.backendId,
            metadata.channelId,
            metadata.eventId,
            "gallery",
            static_cast<int>(index),
            persistentArtwork(
                metadata.backendId,
                metadata.channelId,
                metadata.eventId,
                metadata.images[index].artwork,
                resolvedAt));
    }
}
}

EpgCacheController::EpgCacheController(EpgCacheService& service)
    : directService_(&service), registry_(nullptr), artworkRepository_(nullptr),
      artworkJsonSerializer_(nullptr),
      scraperMetadataAllowedRoots_(EpgArtworkController::defaultAllowedRoots())
{
}

EpgCacheController::EpgCacheController(EpgCacheServiceRegistry& registry)
    : directService_(nullptr), registry_(&registry), artworkRepository_(nullptr),
      artworkJsonSerializer_(nullptr),
      scraperMetadataAllowedRoots_(EpgArtworkController::defaultAllowedRoots())
{
}

EpgCacheController::EpgCacheController(
    EpgCacheService& service,
    EpgArtworkRepository& artworkRepository,
    EpgArtworkPublicJsonSerializer& artworkJsonSerializer)
    : directService_(&service), registry_(nullptr),
      artworkRepository_(&artworkRepository),
      artworkJsonSerializer_(&artworkJsonSerializer),
      scraperMetadataAllowedRoots_(EpgArtworkController::defaultAllowedRoots())
{
}

EpgCacheController::EpgCacheController(
    EpgCacheServiceRegistry& registry,
    EpgArtworkRepository& artworkRepository,
    EpgArtworkPublicJsonSerializer& artworkJsonSerializer)
    : directService_(nullptr), registry_(&registry),
      artworkRepository_(&artworkRepository),
      artworkJsonSerializer_(&artworkJsonSerializer),
      scraperMetadataAllowedRoots_(EpgArtworkController::defaultAllowedRoots())
{
}

void EpgCacheController::registerScraperMetadataResolver(
    const std::string& backendId,
    IEpgScraperMetadataResolver& resolver)
{
    scraperMetadataResolverRegistry_.registerResolver(backendId, resolver);
}

void EpgCacheController::setScraperMetadataAllowedRoots(
    std::vector<std::string> allowedRoots)
{
    scraperMetadataAllowedRoots_ = std::move(allowedRoots);
}

EpgCacheService* EpgCacheController::findService(
    const std::string& backendId) const
{
    return registry_ != nullptr ? registry_->findService(backendId) : directService_;
}

ApiResponse EpgCacheController::refreshBackendWindow(
    const std::string& backendId,
    const VdrEventQuery& query)
{
    const std::string normalizedBackendId = normalizeBackendId(backendId);
    EpgCacheService* service = findService(normalizedBackendId);
    if (service == nullptr)
    {
        return jsonResponse(404, serializeBackendNotFound(normalizedBackendId));
    }
    const EpgCacheRefreshResult result =
        service->refreshBackendWindow(normalizedBackendId, query);
    if (!result.accepted)
    {
        return jsonResponse(400, serializeRefreshResult(normalizedBackendId, result));
    }
    if (!result.stored)
    {
        return jsonResponse(503, serializeRefreshResult(normalizedBackendId, result));
    }
    return jsonResponse(200, serializeRefreshResult(normalizedBackendId, result));
}

ApiResponse EpgCacheController::getStatus(const std::string& backendId) const
{
    const std::string normalizedBackendId = normalizeBackendId(backendId);
    EpgCacheService* service = findService(normalizedBackendId);
    if (service == nullptr)
    {
        return jsonResponse(404, serializeBackendNotFound(normalizedBackendId));
    }
    return jsonResponse(
        200,
        serializeStatus(service->getStatusForBackend(normalizedBackendId)));
}

ApiResponse EpgCacheController::getNowNext(
    const std::string& backendId,
    const std::string& channelId,
    const std::string& fromTime,
    int eventLimit) const
{
    const std::string normalizedBackendId = normalizeBackendId(backendId);
    EpgCacheService* service = findService(normalizedBackendId);
    if (service == nullptr)
    {
        return jsonResponse(404, serializeBackendNotFound(normalizedBackendId));
    }
    return jsonResponse(
        200,
        serializeEvents(
            normalizedBackendId,
            service->findNowNextForBackend(
                normalizedBackendId,
                channelId,
                fromTime,
                eventLimit),
            artworkRepository_,
            artworkJsonSerializer_));
}

ApiResponse EpgCacheController::getWindow(
    const std::string& backendId,
    const std::string& channelId,
    const std::string& fromTime,
    const std::string& untilTime,
    int eventLimit) const
{
    const std::string normalizedBackendId = normalizeBackendId(backendId);
    EpgCacheService* service = findService(normalizedBackendId);
    if (service == nullptr)
    {
        return jsonResponse(404, serializeBackendNotFound(normalizedBackendId));
    }
    return jsonResponse(
        200,
        serializeEvents(
            normalizedBackendId,
            service->findWindowForBackend(
                normalizedBackendId,
                channelId,
                fromTime,
                untilTime,
                eventLimit),
            artworkRepository_,
            artworkJsonSerializer_));
}

ApiResponse EpgCacheController::getMetadata(
    const std::string& backendId,
    const std::string& channelId,
    const std::string& eventId) const
{
    if (channelId.empty() || eventId.empty())
    {
        return jsonError(400, "channelId and eventId are required");
    }

    const std::string normalizedBackendId = normalizeBackendId(backendId);
    if (artworkRepository_ != nullptr)
    {
        const std::string cached = artworkRepository_->findMetadataJson(
            normalizedBackendId,
            channelId,
            eventId);
        if (!cached.empty()) return jsonResponse(200, cached);
    }

    IEpgScraperMetadataResolver* resolver =
        scraperMetadataResolverRegistry_.findResolver(normalizedBackendId);
    if (resolver == nullptr)
    {
        return jsonError(503, "epg scraper metadata backend unavailable");
    }

    const EpgScraperMetadataResolution resolution = resolver->resolve(
        normalizedBackendId,
        metadataEvent(channelId, eventId));
    if (!resolution.attempted)
    {
        return jsonError(502, "epg scraper metadata lookup failed");
    }

    persistMetadataResolution(
        artworkRepository_,
        scraperMetadataJsonSerializer_,
        resolution);
    return jsonResponse(200, scraperMetadataJsonSerializer_.serialize(resolution));
}

ApiResponse EpgCacheController::getMetadataImage(
    const std::string& backendId,
    const std::string& channelId,
    const std::string& eventId,
    const std::string& kind,
    int index) const
{
    if (channelId.empty() || eventId.empty() || kind.empty() || index < 0)
    {
        return jsonError(
            400,
            "channelId, eventId, kind and non-negative index are required");
    }
    if (kind != "preferred" && kind != "person" && kind != "gallery")
    {
        return jsonError(400, "unsupported epg scraper metadata image kind");
    }

    const std::string normalizedBackendId = normalizeBackendId(backendId);
    if (artworkRepository_ != nullptr)
    {
        EpgArtworkReference cached = artworkRepository_->findMetadataImage(
            normalizedBackendId,
            channelId,
            eventId,
            kind,
            index);
        if (!cached.valid() && kind == "preferred" && index == 0)
        {
            cached = artworkRepository_->find(
                normalizedBackendId,
                channelId,
                eventId);
        }
        if (cached.valid())
        {
            return EpgArtworkController::serveValidatedPath(
                cached.path,
                scraperMetadataAllowedRoots_);
        }
    }

    IEpgScraperMetadataResolver* resolver =
        scraperMetadataResolverRegistry_.findResolver(normalizedBackendId);
    if (resolver == nullptr)
    {
        return jsonError(503, "epg scraper metadata backend unavailable");
    }

    const EpgScraperMetadataResolution resolution = resolver->resolve(
        normalizedBackendId,
        metadataEvent(channelId, eventId));
    if (!resolution.attempted)
    {
        return jsonError(502, "epg scraper metadata lookup failed");
    }
    if (!resolution.found)
    {
        return jsonError(404, "epg scraper metadata not found");
    }

    persistMetadataResolution(
        artworkRepository_,
        scraperMetadataJsonSerializer_,
        resolution);
    const EpgScraperArtwork* artwork = selectedMetadataArtwork(
        resolution.metadata,
        kind,
        index);
    if (artwork == nullptr || !artwork->valid())
    {
        return jsonError(404, "epg scraper metadata image not found");
    }
    return EpgArtworkController::serveValidatedPath(
        artwork->path,
        scraperMetadataAllowedRoots_);
}
