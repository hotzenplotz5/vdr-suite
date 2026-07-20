#include "EpgCacheController.h"

#include "EpgArtworkPublicJsonSerializer.h"
#include "EpgArtworkReference.h"
#include "EpgArtworkRepository.h"
#include "EpgCacheService.h"
#include "EpgCacheServiceRegistry.h"
#include "IEpgScraperMetadataResolver.h"
#include "VdrEvent.h"

#include <iomanip>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace
{
std::string normalizeBackendId(const std::string& backendId)
{
    if (backendId.empty())
    {
        return "default";
    }

    return backendId;
}

std::string escapeJsonString(const std::string& value)
{
    std::ostringstream escaped;

    for (unsigned char character : value)
    {
        switch (character)
        {
        case '"':
            escaped << "\\\"";
            break;
        case '\\':
            escaped << "\\\\";
            break;
        case '\n':
            escaped << "\\n";
            break;
        case '\r':
            escaped << "\\r";
            break;
        case '\t':
            escaped << "\\t";
            break;
        default:
            if (character < 0x20)
            {
                escaped
                    << "\\u"
                    << std::hex
                    << std::setw(4)
                    << std::setfill('0')
                    << static_cast<int>(character)
                    << std::dec
                    << std::setfill(' ');
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

std::string serializeArtwork(
    const std::string& backendId,
    const VdrEvent& event,
    EpgArtworkRepository* artworkRepository,
    EpgArtworkPublicJsonSerializer* artworkJsonSerializer)
{
    if (artworkRepository == nullptr || artworkJsonSerializer == nullptr)
    {
        return "{\"available\":false}";
    }

    const EpgArtworkReference artwork = artworkRepository->find(
        backendId,
        event.channelId,
        event.id);

    return artworkJsonSerializer->serialize(artwork);
}

std::string serializeEvent(
    const std::string& backendId,
    const VdrEvent& event,
    EpgArtworkRepository* artworkRepository,
    EpgArtworkPublicJsonSerializer* artworkJsonSerializer)
{
    std::ostringstream json;

    json
        << "{"
        << "\"id\":\"" << escapeJsonString(event.id) << "\","
        << "\"channelId\":\"" << escapeJsonString(event.channelId) << "\","
        << "\"title\":\"" << escapeJsonString(event.title) << "\","
        << "\"subtitle\":\"" << escapeJsonString(event.subtitle) << "\","
        << "\"description\":\"" << escapeJsonString(event.description) << "\","
        << "\"startTime\":\"" << escapeJsonString(event.startTime) << "\","
        << "\"endTime\":\"" << escapeJsonString(event.endTime) << "\","
        << "\"durationSeconds\":" << event.durationSeconds << ","
        << "\"parentalRating\":" << event.parentalRating << ","
        << "\"artwork\":"
        << serializeArtwork(
            backendId,
            event,
            artworkRepository,
            artworkJsonSerializer)
        << "}";

    return json.str();
}

std::string serializeEvents(
    const std::string& backendId,
    const std::vector<VdrEvent>& events,
    EpgArtworkRepository* artworkRepository,
    EpgArtworkPublicJsonSerializer* artworkJsonSerializer)
{
    std::ostringstream json;

    json
        << "{"
        << "\"backendId\":\"" << escapeJsonString(backendId) << "\","
        << "\"eventCount\":" << events.size() << ","
        << "\"events\":[";

    for (std::size_t index = 0; index < events.size(); ++index)
    {
        if (index > 0)
        {
            json << ",";
        }

        json << serializeEvent(
            backendId,
            events.at(index),
            artworkRepository,
            artworkJsonSerializer);
    }

    json << "]}";

    return json.str();
}

std::string serializeRefreshResult(
    const std::string& backendId,
    const EpgCacheRefreshResult& result)
{
    std::ostringstream json;

    json
        << "{"
        << "\"backendId\":\"" << escapeJsonString(backendId) << "\","
        << "\"accepted\":" << boolJson(result.accepted) << ","
        << "\"fetched\":" << boolJson(result.fetched) << ","
        << "\"stored\":" << boolJson(result.stored) << ","
        << "\"eventCount\":" << result.eventCount
        << "}";

    return json.str();
}

std::string serializeStatus(
    const EpgCacheStatus& status)
{
    std::ostringstream json;

    json
        << "{"
        << "\"backendId\":\"" << escapeJsonString(status.backendId) << "\","
        << "\"ready\":" << boolJson(status.ready) << ","
        << "\"eventCount\":" << status.eventCount << ","
        << "\"lastRefreshKnown\":" << boolJson(status.lastRefreshKnown) << ","
        << "\"lastRefreshAccepted\":" << boolJson(status.lastRefreshAccepted) << ","
        << "\"lastRefreshFetched\":" << boolJson(status.lastRefreshFetched) << ","
        << "\"lastRefreshStored\":" << boolJson(status.lastRefreshStored) << ","
        << "\"lastRefreshEventCount\":" << status.lastRefreshEventCount << ","
        << "\"lastRefreshStartedAt\":" << status.lastRefreshStartedAt << ","
        << "\"lastRefreshFinishedAt\":" << status.lastRefreshFinishedAt << ","
        << "\"lastRefreshDurationMs\":" << status.lastRefreshDurationMs << ","
        << "\"lastError\":\"" << escapeJsonString(status.lastError) << "\""
        << "}";

    return json.str();
}

std::string serializeBackendNotFound(
    const std::string& backendId)
{
    std::ostringstream json;

    json
        << "{"
        << "\"backendId\":\"" << escapeJsonString(backendId) << "\","
        << "\"status\":\"backend-not-found\","
        << "\"eventCount\":0,"
        << "\"events\":[]"
        << "}";

    return json.str();
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

VdrEvent metadataEvent(
    const std::string& channelId,
    const std::string& eventId)
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
        if (index < 0 || static_cast<std::size_t>(index) >= metadata.people.size())
        {
            return nullptr;
        }
        return &metadata.people[static_cast<std::size_t>(index)].image;
    }

    if (kind == "gallery")
    {
        if (index < 0 || static_cast<std::size_t>(index) >= metadata.images.size())
        {
            return nullptr;
        }
        return &metadata.images[static_cast<std::size_t>(index)].artwork;
    }

    return nullptr;
}
}

EpgCacheController::EpgCacheController(EpgCacheService& service)
    : directService_(&service),
      registry_(nullptr),
      artworkRepository_(nullptr),
      artworkJsonSerializer_(nullptr),
      scraperMetadataAllowedRoots_(EpgArtworkController::defaultAllowedRoots())
{
}

EpgCacheController::EpgCacheController(EpgCacheServiceRegistry& registry)
    : directService_(nullptr),
      registry_(&registry),
      artworkRepository_(nullptr),
      artworkJsonSerializer_(nullptr),
      scraperMetadataAllowedRoots_(EpgArtworkController::defaultAllowedRoots())
{
}

EpgCacheController::EpgCacheController(
    EpgCacheService& service,
    EpgArtworkRepository& artworkRepository,
    EpgArtworkPublicJsonSerializer& artworkJsonSerializer)
    : directService_(&service),
      registry_(nullptr),
      artworkRepository_(&artworkRepository),
      artworkJsonSerializer_(&artworkJsonSerializer),
      scraperMetadataAllowedRoots_(EpgArtworkController::defaultAllowedRoots())
{
}

EpgCacheController::EpgCacheController(
    EpgCacheServiceRegistry& registry,
    EpgArtworkRepository& artworkRepository,
    EpgArtworkPublicJsonSerializer& artworkJsonSerializer)
    : directService_(nullptr),
      registry_(&registry),
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
    if (registry_ != nullptr)
    {
        return registry_->findService(backendId);
    }

    return directService_;
}

ApiResponse EpgCacheController::refreshBackendWindow(
    const std::string& backendId,
    const VdrEventQuery& query)
{
    const std::string normalizedBackendId = normalizeBackendId(backendId);
    EpgCacheService* service = findService(normalizedBackendId);

    if (service == nullptr)
    {
        return jsonResponse(
            404,
            serializeBackendNotFound(normalizedBackendId));
    }

    const EpgCacheRefreshResult result =
        service->refreshBackendWindow(normalizedBackendId, query);

    if (!result.accepted)
    {
        return jsonResponse(
            400,
            serializeRefreshResult(normalizedBackendId, result));
    }

    if (!result.stored)
    {
        return jsonResponse(
            503,
            serializeRefreshResult(normalizedBackendId, result));
    }

    return jsonResponse(
        200,
        serializeRefreshResult(normalizedBackendId, result));
}

ApiResponse EpgCacheController::getStatus(
    const std::string& backendId) const
{
    const std::string normalizedBackendId = normalizeBackendId(backendId);
    EpgCacheService* service = findService(normalizedBackendId);

    if (service == nullptr)
    {
        return jsonResponse(
            404,
            serializeBackendNotFound(normalizedBackendId));
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
        return jsonResponse(
            404,
            serializeBackendNotFound(normalizedBackendId));
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
        return jsonResponse(
            404,
            serializeBackendNotFound(normalizedBackendId));
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

    return jsonResponse(
        200,
        scraperMetadataJsonSerializer_.serialize(resolution));
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
