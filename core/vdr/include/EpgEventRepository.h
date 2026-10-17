#pragma once

#include "VdrEvent.h"

#include <cstddef>
#include <mutex>
#include <optional>
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

    // True only when this exact backend/channel/EPG key was ingested with
    // a nonempty source EventID. Old cache rows remain unproven. This is
    // evidence of a supplied ID, NOT proof of canonical ProgramEvent identity.
    bool hasSourceEventIdForBackend(
        const std::string& backendId,
        const std::string& channelId,
        const std::string& eventId) const;

    // Exact source-provenanced VDR EPG observation; a synthetic/cache-only
    // event ID or legacy row is never considered a native event. The caller
    // must still verify backend generation, freshness and write permission.
    std::optional<VdrEvent> findSourceEventForBackend(
        const std::string& backendId,
        const std::string& channelId,
        const std::string& nativeEventId) const;

    std::vector<VdrEvent> findNowNextForBackend(
        const std::string& backendId,
        const std::string& channelId,
        const std::string& fromTime,
        int eventLimit) const;

    std::vector<VdrEvent> findNowNextPerChannelForBackend(
        const std::string& backendId,
        const std::string& channelId,
        const std::string& fromTime,
        int perChannelLimit) const;

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
