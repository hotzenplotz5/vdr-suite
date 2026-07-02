#include "EpgCacheService.h"

EpgCacheService::EpgCacheService(
    EpgEventRepository& repository,
    VdrService& vdrService)
    : repository_(repository),
      vdrService_(vdrService)
{
}

EpgCacheRefreshResult EpgCacheService::refreshBackendWindow(
    const std::string& backendId,
    const VdrEventQuery& query)
{
    EpgCacheRefreshResult result;
    result.accepted = isBoundedRefreshQuery(query);

    if (!result.accepted)
    {
        return result;
    }

    const std::vector<VdrEvent> events = vdrService_.getEvents(query);

    result.fetched = true;
    result.eventCount = events.size();
    result.stored = repository_.upsertEventsForBackend(backendId, events);

    return result;
}

std::vector<VdrEvent> EpgCacheService::findNowNextForBackend(
    const std::string& backendId,
    const std::string& channelId,
    const std::string& fromTime,
    int eventLimit) const
{
    return repository_.findNowNextForBackend(
        backendId,
        channelId,
        fromTime,
        eventLimit);
}

std::vector<VdrEvent> EpgCacheService::findWindowForBackend(
    const std::string& backendId,
    const std::string& channelId,
    const std::string& fromTime,
    const std::string& untilTime,
    int eventLimit) const
{
    return repository_.findWindowForBackend(
        backendId,
        channelId,
        fromTime,
        untilTime,
        eventLimit);
}

bool EpgCacheService::deleteExpiredForBackend(
    const std::string& backendId,
    const std::string& beforeEndTime)
{
    return repository_.deleteExpiredForBackend(
        backendId,
        beforeEndTime);
}

int EpgCacheService::countForBackend(
    const std::string& backendId) const
{
    return repository_.countForBackend(backendId);
}

bool EpgCacheService::isBoundedRefreshQuery(
    const VdrEventQuery& query)
{
    if (query.onlyCount)
    {
        return false;
    }

    if (!query.eventId.empty())
    {
        return true;
    }

    if (query.limit > 0 || query.channelEventLimit > 0)
    {
        return true;
    }

    if (query.timespan > 0)
    {
        return true;
    }

    if (!query.channelId.empty() && query.from >= 0)
    {
        return true;
    }

    return false;
}
