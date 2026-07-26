#include "EpgCacheService.h"

#include "EpgArtworkEnrichmentService.h"

#include <chrono>
#include <map>
#include <set>
#include <sstream>

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

std::vector<std::string> splitChannelIds(const std::string& value)
{
    std::vector<std::string> result;
    std::istringstream input(value);
    std::string channelId;
    while (std::getline(input, channelId, ','))
    {
        if (!channelId.empty())
        {
            result.push_back(channelId);
        }
    }
    return result;
}

std::vector<std::string> authoritativeChannelsFor(
    const VdrEventQuery& query,
    const std::vector<VdrEvent>& events)
{
    std::map<std::string, int> counts;
    for (const VdrEvent& event : events)
    {
        if (!event.channelId.empty())
        {
            ++counts[event.channelId];
        }
    }

    std::set<std::string> candidates;
    for (const auto& entry : counts)
    {
        candidates.insert(entry.first);
    }
    for (const std::string& channelId : splitChannelIds(query.channelId))
    {
        candidates.insert(channelId);
    }

    std::vector<std::string> authoritative;
    for (const std::string& channelId : candidates)
    {
        const int count = counts[channelId];
        if (query.channelEventLimit <= 0 ||
            count < query.channelEventLimit)
        {
            authoritative.push_back(channelId);
        }
    }
    return authoritative;
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
    result.artworkEnrichmentAvailable = artworkEnrichmentService_ != nullptr;

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
    result.authoritative = isAuthoritativeRefreshQuery(query);

    if (result.authoritative)
    {
        const std::vector<std::string> authoritativeChannels =
            authoritativeChannelsFor(query, events);
        if (!authoritativeChannels.empty())
        {
            const long long from = query.from;
            const long long until = from + query.timespan;
            const EpgAuthoritativeWindowResult stored =
                repository_.replaceAuthoritativeWindowForBackend(
                    normalizedBackendId,
                    std::to_string(from),
                    std::to_string(until),
                    authoritativeChannels,
                    events);
            result.stored = stored.stored;
            result.removedEventCount = stored.removedEvents.size();
        }
        else
        {
            result.stored = repository_.upsertEventsForBackend(
                normalizedBackendId,
                events);
        }
    }
    else
    {
        result.stored = repository_.upsertEventsForBackend(
            normalizedBackendId,
            events);
    }

    if (result.stored && artworkEnrichmentService_ != nullptr)
    {
        const EpgArtworkEnrichmentResult artworkResult =
            artworkEnrichmentService_->enrich(normalizedBackendId, events);
        result.artworkQueueAvailable = artworkResult.queueAvailable;
        result.artworkQueued = artworkResult.queued;
        result.artworkDeduplicated = artworkResult.deduplicated;
        result.artworkSuppressed = artworkResult.suppressed;
        result.artworkDropped = artworkResult.dropped;
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

bool EpgCacheService::containsEventForBackend(
    const std::string& backendId,
    const std::string& channelId,
    const std::string& eventId) const
{
    return repository_.containsEventForBackend(
        backendId,
        channelId,
        eventId);
}

bool EpgCacheService::isAuthoritativeRefreshQuery(
    const VdrEventQuery& query)
{
    return !query.onlyCount &&
        query.eventId.empty() &&
        query.from >= 0 &&
        query.timespan > 0 &&
        query.start < 0 &&
        query.limit <= 0;
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
