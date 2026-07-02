#pragma once

#include "DashboardController.h"
#include "VdrEventQuery.h"

#include <string>

class EpgCacheService;

class IEpgCacheController
{
public:
    virtual ~IEpgCacheController() = default;

    virtual ApiResponse refreshBackendWindow(
        const std::string& backendId,
        const VdrEventQuery& query) = 0;

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

    ApiResponse refreshBackendWindow(
        const std::string& backendId,
        const VdrEventQuery& query) override;

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
    EpgCacheService& service_;
};
