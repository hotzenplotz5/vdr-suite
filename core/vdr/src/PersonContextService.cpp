#include "PersonContextService.h"

#include "PersonNameNormalizer.h"
#include "PersonQuery.h"

#include <algorithm>
#include <map>
#include <utility>
#include <vector>

namespace
{

int boundedLimit(int limit)
{
    if (limit <= 0)
    {
        return 50;
    }
    return std::min(limit, 100);
}

int boundedOffset(int offset)
{
    return std::max(0, std::min(offset, 100000));
}

bool allBackends(const std::string& backendId)
{
    return backendId.empty() || backendId == "all";
}

std::string channelName(
    const std::vector<VdrChannel>& channels,
    const std::string& channelId)
{
    for (const VdrChannel& channel : channels)
    {
        if (channel.id == channelId)
        {
            return channel.name;
        }
    }
    return channelId;
}

}

PersonContextService::PersonContextService(
    RecordingPersonSearchService& recordingPersonSearchService,
    EpgPersonIndexRepository& epgPersonIndexRepository,
    VdrSnapshotReadService& snapshotReadService)
    : recordingPersonSearchService_(recordingPersonSearchService),
      epgPersonIndexRepository_(epgPersonIndexRepository),
      snapshotReadService_(snapshotReadService)
{
}

PersonContextResult PersonContextService::getContext(
    const std::string& name,
    const std::string& providerPersonId,
    const std::string& backendId,
    const std::string& channelId,
    const std::string& eventId,
    const std::string& fromTime,
    int limit,
    int offset) const
{
    PersonContextResult result;
    result.name = name;
    result.normalizedName = PersonNameNormalizer::normalize(name);
    result.providerPersonId = providerPersonId;
    result.identityMatch = providerPersonId.empty()
        ? "name-only"
        : "provider-id";
    result.confidence = providerPersonId.empty() ? 0.65 : 0.95;
    result.ambiguous = providerPersonId.empty();
    result.epgLimit = boundedLimit(limit);
    result.epgOffset = boundedOffset(offset);

    if (result.normalizedName.empty())
    {
        result.recordings = RecordingPersonSearchResult::empty(
            result.epgLimit,
            result.epgOffset);
        return result;
    }

    const std::vector<VdrRecording> recordings = allBackends(backendId)
        ? snapshotReadService_.getRecordings()
        : snapshotReadService_.getRecordingsForBackend(backendId);

    result.recordings = recordingPersonSearchService_.search(
        recordings,
        PersonQuery::byNormalizedName(result.normalizedName),
        result.epgLimit,
        result.epgOffset);

    EpgPersonIndexQuery epgQuery;
    epgQuery.backendId = backendId;
    epgQuery.normalizedName = result.normalizedName;
    epgQuery.providerPersonId = providerPersonId;
    epgQuery.fromTime = fromTime;
    epgQuery.limit = result.epgLimit;
    epgQuery.offset = result.epgOffset;

    result.epgTotalCount = epgPersonIndexRepository_.count(epgQuery);
    const std::vector<EpgPersonIndexMatch> indexedMatches =
        epgPersonIndexRepository_.search(epgQuery);

    std::map<std::string, std::vector<VdrChannel>> channelsByBackend;
    for (const EpgPersonIndexMatch& indexedMatch : indexedMatches)
    {
        const std::string& matchBackendId = indexedMatch.person.backendId;
        auto foundChannels = channelsByBackend.find(matchBackendId);
        if (foundChannels == channelsByBackend.end())
        {
            foundChannels = channelsByBackend.emplace(
                matchBackendId,
                snapshotReadService_.getChannelsForBackend(matchBackendId))
                .first;
        }

        PersonContextEpgMatch match;
        match.indexedMatch = indexedMatch;
        match.channelName = channelName(
            foundChannels->second,
            indexedMatch.event.channelId);
        result.epgMatches.push_back(std::move(match));
    }

    const auto preferredImage = std::find_if(
        result.epgMatches.begin(),
        result.epgMatches.end(),
        [&](const PersonContextEpgMatch& match)
        {
            const EpgPersonIndexEntry& person = match.indexedMatch.person;
            const bool selectedEvent =
                !channelId.empty() && !eventId.empty() &&
                person.channelId == channelId && person.eventId == eventId;
            return selectedEvent && person.personImageIndex >= 0;
        });

    const auto fallbackImage = std::find_if(
        result.epgMatches.begin(),
        result.epgMatches.end(),
        [](const PersonContextEpgMatch& match)
        {
            return match.indexedMatch.person.personImageIndex >= 0;
        });

    const auto image = preferredImage != result.epgMatches.end()
        ? preferredImage
        : fallbackImage;

    if (image != result.epgMatches.end())
    {
        const EpgPersonIndexEntry& person = image->indexedMatch.person;
        result.imageBackendId = person.backendId;
        result.imageChannelId = person.channelId;
        result.imageEventId = person.eventId;
        result.imageIndex = person.personImageIndex;
    }

    return result;
}
