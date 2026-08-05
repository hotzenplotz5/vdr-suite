#include "VdrRecordingNativePersonSearchService.h"

#include "Person.h"
#include "VdrRecordingCacheRepository.h"
#include "VdrRecordingNativeMetadataRepository.h"

#include <algorithm>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace
{
std::string roleToString(PersonRole role)
{
    switch (role)
    {
    case PersonRole::Actor: return "actor";
    case PersonRole::Director: return "director";
    case PersonRole::Writer: return "writer";
    case PersonRole::Producer: return "producer";
    case PersonRole::Moderator: return "moderator";
    case PersonRole::Guest: return "guest";
    case PersonRole::Composer: return "composer";
    case PersonRole::Other: return "other";
    case PersonRole::Unknown:
    default: return "unknown";
    }
}

PersonRole roleFromString(const std::string& role)
{
    if (role == "actor") return PersonRole::Actor;
    if (role == "director") return PersonRole::Director;
    if (role == "writer") return PersonRole::Writer;
    if (role == "producer") return PersonRole::Producer;
    if (role == "moderator") return PersonRole::Moderator;
    if (role == "guest") return PersonRole::Guest;
    if (role == "composer") return PersonRole::Composer;
    if (role == "other") return PersonRole::Other;
    return PersonRole::Unknown;
}

std::string sourceToString(ContentClassificationSource source)
{
    if (source == ContentClassificationSource::Tvscraper) return "tvscraper";
    if (source == ContentClassificationSource::Tmdb) return "tmdb";
    return {};
}

ContentClassificationSource sourceFromString(const std::string& source)
{
    return source == "tmdb"
        ? ContentClassificationSource::Tmdb
        : ContentClassificationSource::Tvscraper;
}
}

VdrRecordingNativePersonSearchService::
VdrRecordingNativePersonSearchService(
    VdrRecordingNativeMetadataRepository& metadataRepository,
    VdrRecordingCacheRepository& recordingCacheRepository)
    : metadataRepository_(metadataRepository),
      recordingCacheRepository_(recordingCacheRepository)
{
}

RecordingPersonSearchResult
VdrRecordingNativePersonSearchService::search(
    const std::string& backendId,
    const PersonQuery& query,
    int limit,
    int offset) const
{
    const int normalizedLimit = std::max(0, limit);
    const int normalizedOffset = std::max(0, offset);

    if (query.hasSource() && sourceToString(query.source()).empty())
    {
        return RecordingPersonSearchResult::empty(
            normalizedLimit,
            normalizedOffset);
    }

    VdrRecordingNativePersonSearchQuery nativeQuery;
    if (query.hasName()) nativeQuery.name = query.name();
    if (query.hasNormalizedName())
        nativeQuery.normalizedName = query.normalizedName();
    if (query.hasCharacterName())
        nativeQuery.characterName = query.characterName();
    if (query.hasRole()) nativeQuery.role = roleToString(query.role());
    if (query.hasSource()) nativeQuery.source = sourceToString(query.source());
    if (query.hasProviderReference())
        nativeQuery.providerReference = query.providerReference();
    nativeQuery.limit = normalizedLimit;
    nativeQuery.offset = normalizedOffset;

    const VdrRecordingNativePersonSearchResult nativeResult =
        metadataRepository_.searchPeople(backendId, nativeQuery);
    const std::vector<VdrRecording> recordings =
        recordingCacheRepository_.findAllForBackend(backendId);

    std::unordered_map<std::string, VdrRecording> recordingsByNativeId;
    recordingsByNativeId.reserve(recordings.size());
    for (const VdrRecording& recording : recordings)
    {
        if (!recording.backendNativeId.empty())
            recordingsByNativeId.emplace(recording.backendNativeId, recording);
    }

    std::vector<RecordingPersonSearchMatch> matches;
    matches.reserve(nativeResult.entries.size());
    for (const VdrRecordingNativePersonIndexEntry& entry : nativeResult.entries)
    {
        const auto recording = recordingsByNativeId.find(entry.backendNativeId);
        if (recording == recordingsByNativeId.end()) continue;

        const ContentClassificationSource source =
            sourceFromString(entry.source);
        const Person person = entry.providerReference.empty()
            ? Person::withCharacterName(
                source,
                roleFromString(entry.role),
                entry.name,
                entry.normalizedName,
                entry.characterName)
            : Person::withProviderReference(
                source,
                roleFromString(entry.role),
                entry.name,
                entry.normalizedName,
                entry.characterName,
                0,
                entry.providerReference);
        matches.emplace_back(recording->second, person);
    }

    return RecordingPersonSearchResult::from(
        std::move(matches),
        nativeResult.totalCount,
        nativeResult.limit,
        nativeResult.offset);
}
