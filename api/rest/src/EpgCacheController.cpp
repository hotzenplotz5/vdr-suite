#include "EpgCacheController.h"

#include "EpgArtworkPublicJsonSerializer.h"
#include "EpgArtworkReference.h"
#include "EpgArtworkRepository.h"
#include "EpgCacheService.h"
#include "EpgCacheServiceRegistry.h"
#include "EpgMetadataMaterializationQueue.h"
#include "IEpgScraperMetadataResolver.h"
#include "IEpgSeriesArtworkFallbackDeliveryProvider.h"

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
         << "\"authoritative\":" << boolJson(result.authoritative) << ','
         << "\"eventCount\":" << result.eventCount << ','
         << "\"removedEventCount\":" << result.removedEventCount << '}';
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
    const std::string normalizedBackend = normalizeBackendId(backendId);
    IEpgSeriesArtworkFallbackDeliveryProvider* provider =
        dynamic_cast<IEpgSeriesArtworkFallbackDeliveryProvider*>(&resolver);
    if (provider == nullptr)
    {
        fallbackDeliveryProviders_.erase(normalizedBackend);
        return;
    }

    fallbackDeliveryProviders_[normalizedBackend] = provider;
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

    if (artworkRepository_ == nullptr)
    {
        return jsonError(503, "epg scraper metadata cache unavailable");
    }

    const std::string normalizedBackendId = normalizeBackendId(backendId);
    EpgCacheService* service = findService(normalizedBackendId);
    if (service != nullptr &&
        !service->containsEventForBackend(
            normalizedBackendId,
            channelId,
            eventId))
    {
        return jsonResponse(
            200,
            "{\"available\":false,\"status\":\"stale-event\"}");
    }

    const std::string cached = artworkRepository_->findMetadataJson(
        normalizedBackendId,
        channelId,
        eventId);
    if (!cached.empty())
    {
        return jsonResponse(200, cached);
    }

    EpgMetadataMaterializationQueue::instance().request(
        normalizedBackendId,
        channelId,
        eventId);

    return jsonResponse(
        200,
        "{\"available\":false,\"status\":\"pending\"}");
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
    if (artworkRepository_ == nullptr)
    {
        return jsonError(503, "epg scraper metadata cache unavailable");
    }

    const std::string normalizedBackendId = normalizeBackendId(backendId);
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
        const ApiResponse primary = EpgArtworkController::serveValidatedPath(
            cached.path,
            scraperMetadataAllowedRoots_);
        if (primary.statusCode == 200)
        {
            return primary;
        }
    }

    if (kind == "preferred" && index == 0)
    {
        const auto provider = fallbackDeliveryProviders_.find(
            normalizedBackendId);
        if (provider != fallbackDeliveryProviders_.end() &&
            provider->second != nullptr)
        {
            const EpgSeriesArtworkFallbackAsset fallback =
                provider->second->loadSeriesArtworkFallback(
                    normalizedBackendId,
                    channelId,
                    eventId);
            if (fallback.valid())
            {
                ApiResponse response;
                response.statusCode = 200;
                response.contentType = fallback.contentType;
                response.body = fallback.content;
                return response;
            }
        }
    }

    return jsonError(404, "epg scraper metadata image not found");
}
