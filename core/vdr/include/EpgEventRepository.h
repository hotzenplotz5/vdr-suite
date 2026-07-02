#pragma once

#include "VdrEvent.h"

#include <string>
#include <vector>

class Database;

class EpgEventRepository
{
public:
    explicit EpgEventRepository(Database& database);

    bool ensureSchema();

    bool upsertEventsForBackend(
        const std::string& backendId,
        const std::vector<VdrEvent>& events);

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

    static std::string normalizeBackendId(const std::string& backendId);
};
