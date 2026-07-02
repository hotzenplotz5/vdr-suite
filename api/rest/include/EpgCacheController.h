#pragma once

#include "DashboardController.h"
#include "VdrEventQuery.h"

#include <string>

class EpgCacheService;

class EpgCacheController
{
public:
    explicit EpgCacheController(EpgCacheService& service);

    ApiResponse refreshBackendWindow(
        const std::string& backendId,
        const VdrEventQuery& query);

    ApiResponse getNowNext(
        const std::string& backendId,
        const std::string& channelId,
        const std::string& fromTime,
        int eventLimit) const;

    ApiResponse getWindow(
        const std::string& backendId,
        const std::string& channelId,
        const std::string& fromTime,
        const std::string& untilTime,
        int eventLimit) const;

private:
    EpgCacheService& service_;
};
