#pragma once

#include "EpgEventRepository.h"
#include "VdrEvent.h"
#include "VdrEventQuery.h"
#include "VdrService.h"

#include <cstddef>
#include <string>
#include <vector>

struct EpgCacheRefreshResult
{
    bool accepted = false;
    bool fetched = false;
    bool stored = false;
    std::size_t eventCount = 0;
};

class EpgCacheService
{
public:
    EpgCacheService(
        EpgEventRepository& repository,
        VdrService& vdrService);

    EpgCacheRefreshResult refreshBackendWindow(
        const std::string& backendId,
        const VdrEventQuery& query);

    std::vector<VdrEvent> findNowNextForBackend(
        const std::string& backendId,
        const std::string& channelId,
        const std::string& fromTime,
        int eventLimit) const;

    std::vector<VdrEvent> findWindowForBackend(
        const std::string& backendId,
        const std::string& channelId,
        const std::string& fromTime,
        const std::string& untilTime,
        int eventLimit) const;

    bool deleteExpiredForBackend(
        const std::string& backendId,
        const std::string& beforeEndTime);

    int countForBackend(
        const std::string& backendId) const;

    static bool isBoundedRefreshQuery(
        const VdrEventQuery& query);

private:
    EpgEventRepository& repository_;
    VdrService& vdrService_;
};
