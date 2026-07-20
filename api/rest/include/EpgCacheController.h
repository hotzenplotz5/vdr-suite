#pragma once

#include "DashboardController.h"
#include "VdrEventQuery.h"

#include <string>

class EpgArtworkPublicJsonSerializer;
class EpgArtworkRepository;
class EpgCacheService;
class EpgCacheServiceRegistry;

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

private:
    EpgCacheService* directService_;
    EpgCacheServiceRegistry* registry_;
    EpgArtworkRepository* artworkRepository_;
    EpgArtworkPublicJsonSerializer* artworkJsonSerializer_;

    EpgCacheService* findService(
        const std::string& backendId) const;
};
