#pragma once

#include "EpgEventRepository.h"
#include "VdrEvent.h"
#include "VdrEventQuery.h"
#include "VdrService.h"

#include <cstddef>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

struct EpgCacheRefreshResult
{
    bool accepted = false;
    bool fetched = false;
    bool stored = false;
    std::size_t eventCount = 0;
};

struct EpgCacheStatus
{
    std::string backendId;
    bool ready = false;
    int eventCount = 0;

    bool lastRefreshKnown = false;
    bool lastRefreshAccepted = false;
    bool lastRefreshFetched = false;
    bool lastRefreshStored = false;
    std::size_t lastRefreshEventCount = 0;
    long long lastRefreshStartedAt = 0;
    long long lastRefreshFinishedAt = 0;
    long long lastRefreshDurationMs = 0;
    std::string lastError;
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

    EpgCacheStatus getStatusForBackend(
        const std::string& backendId) const;

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
    struct RefreshMetadata
    {
        bool known = false;
        bool accepted = false;
        bool fetched = false;
        bool stored = false;
        std::size_t eventCount = 0;
        long long startedAt = 0;
        long long finishedAt = 0;
        long long durationMs = 0;
        std::string lastError;
    };

    EpgEventRepository& repository_;
    VdrService& vdrService_;
    mutable std::mutex statusMutex_;
    std::unordered_map<std::string, RefreshMetadata> refreshMetadataByBackend_;

    void updateStatusForBackend(
        const std::string& backendId,
        const EpgCacheRefreshResult& result,
        long long startedAt,
        long long finishedAt,
        long long durationMs,
        const std::string& lastError);

    static std::string normalizeBackendId(
        const std::string& backendId);
};
