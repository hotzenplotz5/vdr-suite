#include "EpgCacheService.h"

#include "EpgArtworkEnrichmentService.h"

#include <chrono>

namespace
{
long long currentEpochSeconds()
{
    return std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

long long elapsedMilliseconds(
    const std::chrono::steady_clock::time_point& startedAt)
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - startedAt).count();
}
}

EpgCacheService::EpgCacheService(
    EpgEventRepository& repository,
    VdrService& vdrService,
    EpgArtworkEnrichmentService* artworkEnrichmentService)
    : repository_(repository),
      vdrService_(vdrService),
      artworkEnrichmentService_(artworkEnrichmentService)
{
}

EpgCacheRefreshResult EpgCacheService::refreshBackendWindow(
    const std::string& backendId,
    const VdrEventQuery& query)
{
    const std::string normalizedBackendId = normalizeBackendId(backendId);
    const long long startedAt = currentEpochSeconds();
    const std::chrono::steady_clock::time_point steadyStartedAt =
        std::chrono::steady_clock::now();

    EpgCacheRefreshResult result;
    result.accepted = isBoundedRefreshQuery(query);

    if (!result.accepted)
    {
        updateStatusForBackend(
            normalizedBackendId,
            result,
            startedAt,
            currentEpochSeconds(),
            elapsedMilliseconds(steadyStartedAt),
            "refresh-not-accepted");
        return result;
    }

    const std::vector<VdrEvent> events = vdrService_.getEvents(query);

    result.fetched = true;
    result.eventCount = events.size();
    result.stored = repository_.upsertEventsForBackend(
        normalizedBackendId,
        events);

    if (result.stored && artworkEnrichmentService_ != nullptr)
    {
        artworkEnrichmentService_->enrich(normalizedBackendId, events);
    }

    updateStatusForBackend(
        normalizedBackendId,
        result,
        startedAt,
        currentEpochSeconds(),
        elapsedMilliseconds(steadyStartedAt),
        result.stored ? "" : "store-failed");

    return result;
}

EpgCacheStatus EpgCacheService::getStatusForBackend(
    const std::string& backendId) const
{
    const std::string normalizedBackendId = normalizeBackendId(backendId);

    EpgCacheStatus status;
    status.backendId = normalizedBackendId;
    status.eventCount = repository_.countForBackend(normalizedBackendId);
    status.ready = status.eventCount > 0;

    std::lock_guard<std::mutex> lock(statusMutex_);

    const auto found = refreshMetadataByBackend_.find(normalizedBackendId);

    if (found == refreshMetadataByBackend_.end())
    {
        return status;
    }

    const RefreshMetadata& metadata = found->second;
    status.lastRefreshKnown = metadata.known;
    status.lastRefreshAccepted = metadata.accepted;
    status.lastRefreshFetched = metadata.fetched;
    status.lastRefreshStored = metadata.stored;
    status.lastRefreshEventCount = metadata.eventCount;
    status.lastRefreshStartedAt = metadata.startedAt;
    status.lastRefreshFinishedAt = metadata.finishedAt;
    status.lastRefreshDurationMs = metadata.durationMs;
    status.lastError = metadata.lastError;

    return status;
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

void EpgCacheService::updateStatusForBackend(
    const std::string& backendId,
    const EpgCacheRefreshResult& result,
    long long startedAt,
    long long finishedAt,
    long long durationMs,
    const std::string& lastError)
{
    const std::string normalizedBackendId = normalizeBackendId(backendId);

    RefreshMetadata metadata;
    metadata.known = true;
    metadata.accepted = result.accepted;
    metadata.fetched = result.fetched;
    metadata.stored = result.stored;
    metadata.eventCount = result.eventCount;
    metadata.startedAt = startedAt;
    metadata.finishedAt = finishedAt;
    metadata.durationMs = durationMs;
    metadata.lastError = lastError;

    std::lock_guard<std::mutex> lock(statusMutex_);
    refreshMetadataByBackend_[normalizedBackendId] = metadata;
}

std::string EpgCacheService::normalizeBackendId(
    const std::string& backendId)
{
    if (backendId.empty())
    {
        return "default";
    }

    return backendId;
}
