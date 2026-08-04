#pragma once

#include "DashboardController.h"
#include "EpgArtworkController.h"
#include "VdrEventQuery.h"

#include <map>
#include <string>
#include <vector>

class EpgArtworkPublicJsonSerializer;
class EpgArtworkRepository;
class EpgCacheService;
class EpgCacheServiceRegistry;
class IEpgScraperMetadataResolver;
class IEpgSeriesArtworkFallbackDeliveryProvider;

class IEpgCacheController
{
public:
    virtual ~IEpgCacheController() = default;

    virtual ApiResponse refreshBackendWindow(
        const std::string& backendId,
        const VdrEventQuery& query) = 0;

    virtual ApiResponse getStatus(
        const std::string& backendId) const = 0;

    virtual ApiResponse getNowNext(
        const std::string& backendId,
        const std::string& channelId,
        const std::string& fromTime,
        int eventLimit) const = 0;

    virtual ApiResponse getWindow(
        const std::string& backendId,
        const std::string& channelId,
        const std::string& fromTime,
        const std::string& untilTime,
        int eventLimit) const = 0;

    virtual ApiResponse getArtwork(
        const std::string&,
        const std::string&,
        const std::string&) const
    {
        ApiResponse response;
        response.statusCode = 503;
        response.contentType = "application/json";
        response.body = "{\"error\":\"epg artwork unavailable\"}";
        return response;
    }

    virtual ApiResponse getMetadata(
        const std::string&,
        const std::string&,
        const std::string&) const
    {
        ApiResponse response;
        response.statusCode = 503;
        response.contentType = "application/json";
        response.body = "{\"error\":\"epg scraper metadata unavailable\"}";
        return response;
    }

    virtual ApiResponse getMetadataImage(
        const std::string&,
        const std::string&,
        const std::string&,
        const std::string&,
        int) const
    {
        ApiResponse response;
        response.statusCode = 503;
        response.contentType = "application/json";
        response.body = "{\"error\":\"epg scraper metadata image unavailable\"}";
        return response;
    }
};

class EpgCacheController : public IEpgCacheController
{
public:
    explicit EpgCacheController(EpgCacheService& service);
    explicit EpgCacheController(EpgCacheServiceRegistry& registry);
    EpgCacheController(
        EpgCacheService& service,
        EpgArtworkRepository& artworkRepository,
        EpgArtworkPublicJsonSerializer& artworkJsonSerializer);
    EpgCacheController(
        EpgCacheServiceRegistry& registry,
        EpgArtworkRepository& artworkRepository,
        EpgArtworkPublicJsonSerializer& artworkJsonSerializer);

    // Compatibility boundary for existing runtime wiring. Metadata GETs do
    // not retain or call the resolver itself; only its optional, read-only
    // managed fallback-delivery capability is registered.
    void registerScraperMetadataResolver(
        const std::string& backendId,
        IEpgScraperMetadataResolver& resolver);

    void setScraperMetadataAllowedRoots(
        std::vector<std::string> allowedRoots);

    ApiResponse refreshBackendWindow(
        const std::string& backendId,
        const VdrEventQuery& query) override;

    ApiResponse getStatus(
        const std::string& backendId) const override;

    ApiResponse getNowNext(
        const std::string& backendId,
        const std::string& channelId,
        const std::string& fromTime,
        int eventLimit) const override;

    ApiResponse getWindow(
        const std::string& backendId,
        const std::string& channelId,
        const std::string& fromTime,
        const std::string& untilTime,
        int eventLimit) const override;

    ApiResponse getArtwork(
        const std::string& backendId,
        const std::string& channelId,
        const std::string& eventId) const override
    {
        if (artworkRepository_ == nullptr)
        {
            ApiResponse response;
            response.statusCode = 503;
            response.contentType = "application/json";
            response.body = "{\"error\":\"epg artwork unavailable\"}";
            return response;
        }

        return EpgArtworkController(*artworkRepository_).getArtwork(
            backendId,
            channelId,
            eventId);
    }

    ApiResponse getMetadata(
        const std::string& backendId,
        const std::string& channelId,
        const std::string& eventId) const override;

    ApiResponse getMetadataImage(
        const std::string& backendId,
        const std::string& channelId,
        const std::string& eventId,
        const std::string& kind,
        int index) const override;

private:
    EpgCacheService* directService_;
    EpgCacheServiceRegistry* registry_;
    EpgArtworkRepository* artworkRepository_;
    EpgArtworkPublicJsonSerializer* artworkJsonSerializer_;
    std::vector<std::string> scraperMetadataAllowedRoots_;
    std::map<std::string, IEpgSeriesArtworkFallbackDeliveryProvider*>
        fallbackDeliveryProviders_;

    EpgCacheService* findService(
        const std::string& backendId) const;
};
