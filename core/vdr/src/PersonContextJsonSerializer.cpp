#include "PersonContextJsonSerializer.h"

#include "VdrRecordingMetadataJsonSerializer.h"

#include <cctype>
#include <iomanip>
#include <sstream>
#include <string>

namespace
{

void appendQuoted(std::ostringstream& json, const std::string& value)
{
    json << '"';
    for (const unsigned char character : value)
    {
        switch (character)
        {
        case '"': json << "\\\""; break;
        case '\\': json << "\\\\"; break;
        case '\n': json << "\\n"; break;
        case '\r': json << "\\r"; break;
        case '\t': json << "\\t"; break;
        default:
            if (character < 0x20)
            {
                json << "\\u"
                     << std::hex << std::setw(4) << std::setfill('0')
                     << static_cast<int>(character)
                     << std::dec << std::setfill(' ');
            }
            else
            {
                json << static_cast<char>(character);
            }
            break;
        }
    }
    json << '"';
}

void appendKey(std::ostringstream& json, const char* key)
{
    appendQuoted(json, key);
    json << ':';
}

std::string roleName(PersonRole role)
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
    case PersonRole::Unknown: return "unknown";
    }
    return "unknown";
}

std::string percentEncode(const std::string& value)
{
    std::ostringstream encoded;
    encoded << std::uppercase << std::hex;
    for (const unsigned char character : value)
    {
        if (std::isalnum(character) || character == '-' ||
            character == '_' || character == '.' || character == '~')
        {
            encoded << static_cast<char>(character);
        }
        else
        {
            encoded << '%' << std::setw(2) << std::setfill('0')
                    << static_cast<int>(character);
        }
    }
    return encoded.str();
}

std::string metadataImageUrl(
    const std::string& backendId,
    const std::string& channelId,
    const std::string& eventId,
    int imageIndex)
{
    if (backendId.empty() || channelId.empty() ||
        eventId.empty() || imageIndex < 0)
    {
        return {};
    }

    return "/api/epg/cache/metadata/image?backend=" +
        percentEncode(backendId) +
        "&channelId=" + percentEncode(channelId) +
        "&eventId=" + percentEncode(eventId) +
        "&kind=person&index=" + std::to_string(imageIndex);
}

std::string eventArtworkUrl(const EpgPersonIndexEntry& person)
{
    return "/api/epg/cache/artwork?backend=" +
        percentEncode(person.backendId) +
        "&channelId=" + percentEncode(person.channelId) +
        "&eventId=" + percentEncode(person.eventId);
}

void appendPersonEvidence(
    std::ostringstream& json,
    const Person& person)
{
    json << '{';
    appendKey(json, "name");
    appendQuoted(json, person.originalName());
    json << ',';
    appendKey(json, "normalizedName");
    appendQuoted(json, person.normalizedName());
    json << ',';
    appendKey(json, "role");
    appendQuoted(json, roleName(person.role()));
    json << ',';
    appendKey(json, "characterName");
    appendQuoted(json, person.characterName());
    json << ',';
    appendKey(json, "source");
    appendQuoted(json, "recording-metadata");
    json << ',';
    appendKey(json, "identityMatch");
    appendQuoted(json, person.hasProviderReference()
        ? "provider-reference"
        : "normalized-name");
    json << ',';
    appendKey(json, "confidence");
    json << (person.hasConfidence()
        ? static_cast<double>(person.confidence()) / 100.0
        : 0.65);
    json << '}';
}

void appendRecordingMatch(
    std::ostringstream& json,
    const RecordingPersonSearchMatch& match)
{
    const VdrRecording& recording = match.recording();
    json << '{';
    appendKey(json, "recording");
    json << '{';
    appendKey(json, "id");
    appendQuoted(json, recording.id);
    json << ',';
    appendKey(json, "backendId");
    appendQuoted(json, recording.backendId);
    json << ',';
    appendKey(json, "title");
    appendQuoted(json, recording.title);
    json << ',';
    appendKey(json, "startTime");
    appendQuoted(json, recording.startTime);
    json << ',';
    appendKey(json, "durationSeconds");
    json << recording.durationSeconds;
    json << ',';
    appendKey(json, "metadata");
    json << VdrRecordingMetadataJsonSerializer::serialize(recording);
    json << '}';
    json << ',';
    appendKey(json, "person");
    appendPersonEvidence(json, match.person());
    json << '}';
}

void appendEpgMatch(
    std::ostringstream& json,
    const PersonContextEpgMatch& contextMatch)
{
    const EpgPersonIndexMatch& match = contextMatch.indexedMatch;
    const EpgPersonIndexEntry& person = match.person;
    const std::string personImage = metadataImageUrl(
        person.backendId,
        person.channelId,
        person.eventId,
        person.personImageIndex);

    json << '{';
    appendKey(json, "backendId");
    appendQuoted(json, person.backendId);
    json << ',';
    appendKey(json, "event");
    json << '{';
    appendKey(json, "id");
    appendQuoted(json, match.event.id);
    json << ',';
    appendKey(json, "channelId");
    appendQuoted(json, match.event.channelId);
    json << ',';
    appendKey(json, "title");
    appendQuoted(json, match.event.title);
    json << ',';
    appendKey(json, "subtitle");
    appendQuoted(json, match.event.subtitle);
    json << ',';
    appendKey(json, "startTime");
    appendQuoted(json, match.event.startTime);
    json << ',';
    appendKey(json, "endTime");
    appendQuoted(json, match.event.endTime);
    json << ',';
    appendKey(json, "durationSeconds");
    json << match.event.durationSeconds;
    json << '}';
    json << ',';
    appendKey(json, "channel");
    json << '{';
    appendKey(json, "id");
    appendQuoted(json, match.event.channelId);
    json << ',';
    appendKey(json, "name");
    appendQuoted(json, contextMatch.channelName);
    json << '}';
    json << ',';
    appendKey(json, "person");
    json << '{';
    appendKey(json, "name");
    appendQuoted(json, person.originalName);
    json << ',';
    appendKey(json, "normalizedName");
    appendQuoted(json, person.normalizedName);
    json << ',';
    appendKey(json, "role");
    appendQuoted(json, roleName(person.role));
    json << ',';
    appendKey(json, "characterName");
    appendQuoted(json, person.characterName);
    json << ',';
    appendKey(json, "identityMatch");
    appendQuoted(json, person.identityKind);
    json << ',';
    appendKey(json, "confidence");
    json << person.confidence;
    json << '}';
    json << ',';
    appendKey(json, "artwork");
    json << '{';
    appendKey(json, "available");
    json << "true";
    json << ',';
    appendKey(json, "url");
    appendQuoted(json, eventArtworkUrl(person));
    json << '}';
    json << ',';
    appendKey(json, "personImage");
    json << '{';
    appendKey(json, "available");
    json << (!personImage.empty() ? "true" : "false");
    json << ',';
    appendKey(json, "url");
    appendQuoted(json, personImage);
    json << '}';
    json << ',';
    appendKey(json, "timerStatus");
    appendQuoted(json, "not-evaluated");
    json << ',';
    appendKey(json, "actions");
    json << "{\"recordAvailable\":false,\"searchTimerAvailable\":false}";
    json << '}';
}

}

std::string PersonContextJsonSerializer::serialize(
    const PersonContextResult& result) const
{
    std::ostringstream json;
    const std::string imageUrl = metadataImageUrl(
        result.imageBackendId,
        result.imageChannelId,
        result.imageEventId,
        result.imageIndex);

    json << '{';
    appendKey(json, "person");
    json << '{';
    appendKey(json, "name");
    appendQuoted(json, result.name);
    json << ',';
    appendKey(json, "normalizedName");
    appendQuoted(json, result.normalizedName);
    json << ',';
    appendKey(json, "image");
    json << '{';
    appendKey(json, "available");
    json << (!imageUrl.empty() ? "true" : "false");
    json << ',';
    appendKey(json, "url");
    appendQuoted(json, imageUrl);
    json << '}';
    json << ',';
    appendKey(json, "providerIds");
    json << '{';
    appendKey(json, "tvscraper");
    appendQuoted(json, result.providerPersonId);
    json << ',';
    appendKey(json, "tmdb");
    appendQuoted(json, "");
    json << ',';
    appendKey(json, "imdb");
    appendQuoted(json, "");
    json << '}';
    json << ',';
    appendKey(json, "identity");
    json << '{';
    appendKey(json, "matchMode");
    appendQuoted(json, result.identityMatch);
    json << ',';
    appendKey(json, "confidence");
    json << result.confidence;
    json << ',';
    appendKey(json, "ambiguous");
    json << (result.ambiguous ? "true" : "false");
    json << '}';
    json << '}';

    json << ',';
    appendKey(json, "recordings");
    json << '{';
    appendKey(json, "totalCount");
    json << result.recordings.totalCount();
    json << ',';
    appendKey(json, "returnedCount");
    json << result.recordings.returnedCount();
    json << ',';
    appendKey(json, "limit");
    json << result.recordings.limit();
    json << ',';
    appendKey(json, "offset");
    json << result.recordings.offset();
    json << ',';
    appendKey(json, "matches");
    json << '[';
    for (std::size_t index = 0;
         index < result.recordings.matches().size();
         ++index)
    {
        if (index > 0) json << ',';
        appendRecordingMatch(json, result.recordings.matches().at(index));
    }
    json << ']';
    json << '}';

    json << ',';
    appendKey(json, "epg");
    json << '{';
    appendKey(json, "totalCount");
    json << result.epgTotalCount;
    json << ',';
    appendKey(json, "returnedCount");
    json << result.epgMatches.size();
    json << ',';
    appendKey(json, "limit");
    json << result.epgLimit;
    json << ',';
    appendKey(json, "offset");
    json << result.epgOffset;
    json << ',';
    appendKey(json, "matches");
    json << '[';
    for (std::size_t index = 0; index < result.epgMatches.size(); ++index)
    {
        if (index > 0) json << ',';
        appendEpgMatch(json, result.epgMatches.at(index));
    }
    json << ']';
    json << '}';

    json << ',';
    appendKey(json, "external");
    json << '{';
    appendKey(json, "enabled");
    json << (result.externalEnabled ? "true" : "false");
    json << ',';
    appendKey(json, "provider");
    appendQuoted(json, "");
    json << ',';
    appendKey(json, "matches");
    json << "[]";
    json << '}';
    json << '}';

    return json.str();
}
