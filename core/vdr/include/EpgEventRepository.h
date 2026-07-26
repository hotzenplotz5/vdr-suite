#pragma once

#include "VdrEvent.h"

#include <cstddef>
#include <mutex>
#include <string>
#include <vector>

class Database;

struct EpgEventCacheKey
{
    std::string channelId;
    std::string eventId;
};

struct EpgAuthoritativeWindowResult
{
    bool stored = false;
    std::vector<EpgEventCacheKey> removedEvents;
};

class EpgEventRepository
{
public:
    explicit EpgEventRepository(Database& database);

    bool ensureSchema();

    bool upsertEventsForBackend(
        const std::string& backendId,
        const std::vector<VdrEvent>& events);

    EpgAuthoritativeWindowResult replaceAuthoritativeWindowForBackend(
        const std::string& backendId,
        const std::string& fromTime,
        const std::string& untilTime,
        const std::vector<std::string>& authoritativeChannelIds,
        const std::vector<VdrEvent>& events);

    bool containsEventForBackend(
        const std::string& backendId,
        const std::string& channelId,
        const std::string& eventId) const;

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

    int countForBackend(const std::string& backendId) const;

private:
    Database& database_;
    mutable std::recursive_mutex mutex_;

    static std::string normalizeBackendId(const std::string& backendId);
};
