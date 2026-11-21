from .common import replace_once

# ---------------------------------------------------------------------------
# Cache service: distinguish bounded from authoritative fetches.
# ---------------------------------------------------------------------------

replace_once(
    "core/vdr/include/EpgCacheService.h",
    '''    bool stored = false;
    std::size_t eventCount = 0;
''',
    '''    bool stored = false;
    bool authoritative = false;
    std::size_t eventCount = 0;
    std::size_t removedEventCount = 0;
'''
)

replace_once(
    "core/vdr/include/EpgCacheService.h",
    '''    int countForBackend(
        const std::string& backendId) const;

    static bool isBoundedRefreshQuery(
''',
    '''    int countForBackend(
        const std::string& backendId) const;

    bool containsEventForBackend(
        const std::string& backendId,
        const std::string& channelId,
        const std::string& eventId) const;

    static bool isBoundedRefreshQuery(
'''
)

replace_once(
    "core/vdr/include/EpgCacheService.h",
    '''    static bool isBoundedRefreshQuery(
        const VdrEventQuery& query);

private:
''',
    '''    static bool isBoundedRefreshQuery(
        const VdrEventQuery& query);

    static bool isAuthoritativeRefreshQuery(
        const VdrEventQuery& query);

private:
'''
)

replace_once(
    "core/vdr/src/EpgCacheService.cpp",
    '#include <chrono>\n',
    '#include <chrono>\n#include <map>\n#include <set>\n#include <sstream>\n'
)

service_helpers = r'''
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
'''

replace_once(
    "core/vdr/src/EpgCacheService.cpp",
    '''long long elapsedMilliseconds(
    const std::chrono::steady_clock::time_point& startedAt)
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - startedAt).count();
}
}
''',
    '''long long elapsedMilliseconds(
    const std::chrono::steady_clock::time_point& startedAt)
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - startedAt).count();
}
''' + service_helpers + '''
}
'''
)

replace_once(
    "core/vdr/src/EpgCacheService.cpp",
    '''    result.fetched = true;
    result.eventCount = events.size();
    result.stored = repository_.upsertEventsForBackend(
        normalizedBackendId,
        events);

    if (result.stored && artworkEnrichmentService_ != nullptr)
''',
    '''    result.fetched = true;
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
'''
)

replace_once(
    "core/vdr/src/EpgCacheService.cpp",
    '''int EpgCacheService::countForBackend(
    const std::string& backendId) const
{
    return repository_.countForBackend(backendId);
}

bool EpgCacheService::isBoundedRefreshQuery(
''',
    '''int EpgCacheService::countForBackend(
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

bool EpgCacheService::isBoundedRefreshQuery(
'''
)

replace_once(
    "core/vdr/src/EpgCacheService.cpp",
    '''bool EpgCacheService::isBoundedRefreshQuery(
    const VdrEventQuery& query)
{
''',
    '''bool EpgCacheService::isAuthoritativeRefreshQuery(
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
'''
)

# ---------------------------------------------------------------------------
# Controller: stale native IDs are final and are never queued.
# ---------------------------------------------------------------------------

replace_once(
    "api/rest/src/EpgCacheController.cpp",
    '''         << "\\"stored\\":" << boolJson(result.stored) << ','
         << "\\"eventCount\\":" << result.eventCount << '}';
''',
    '''         << "\\"stored\\":" << boolJson(result.stored) << ','
         << "\\"authoritative\\":" << boolJson(result.authoritative) << ','
         << "\\"eventCount\\":" << result.eventCount << ','
         << "\\"removedEventCount\\":" << result.removedEventCount << '}';
'''
)

replace_once(
    "api/rest/src/EpgCacheController.cpp",
    '''    const std::string normalizedBackendId = normalizeBackendId(backendId);
    const std::string cached = artworkRepository_->findMetadataJson(
''',
    '''    const std::string normalizedBackendId = normalizeBackendId(backendId);
    EpgCacheService* service = findService(normalizedBackendId);
    if (service != nullptr &&
        !service->containsEventForBackend(
            normalizedBackendId,
            channelId,
            eventId))
    {
        return jsonResponse(
            200,
            "{\\"available\\":false,\\"status\\":\\"stale-event\\"}");
    }

    const std::string cached = artworkRepository_->findMetadataJson(
'''
)

