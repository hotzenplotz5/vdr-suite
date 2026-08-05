#include "RecordingMetadataCandidateProvider.h"

#include <algorithm>
#include <cctype>

namespace
{
bool safeText(const std::string& value, std::size_t maximum, bool allowEmpty)
{
    if ((!allowEmpty && value.empty()) || value.size() > maximum) return false;
    return std::none_of(value.begin(), value.end(), [](unsigned char character) {
        return character < 0x20U && character != '\t';
    });
}

bool digits(const std::string& value)
{
    return !value.empty() && value.size() <= 16U &&
        std::all_of(value.begin(), value.end(), [](unsigned char character) {
            return character >= '0' && character <= '9';
        });
}

bool safePoster(const std::string& value)
{
    if (value.empty()) return true;
    if (value.size() < 6U || value.size() > 256U || value.front() != '/' ||
        value.find('/', 1) != std::string::npos ||
        value.find("..") != std::string::npos ||
        value.find("://") != std::string::npos)
        return false;
    return std::all_of(value.begin() + 1, value.end(), [](unsigned char character) {
        return (character >= 'A' && character <= 'Z') ||
            (character >= 'a' && character <= 'z') ||
            (character >= '0' && character <= '9') ||
            character == '_' || character == '-' || character == '.';
    });
}
}

const char* recordingMetadataCandidateKindName(
    RecordingMetadataCandidateKind kind)
{
    switch (kind)
    {
    case RecordingMetadataCandidateKind::Movie: return "movie";
    case RecordingMetadataCandidateKind::Series: return "series";
    case RecordingMetadataCandidateKind::Season: return "season";
    case RecordingMetadataCandidateKind::Episode: return "episode";
    }
    return "movie";
}

bool RecordingMetadataCandidate::valid() const
{
    if (providerId != "tmdb" || !digits(externalId) ||
        !safeText(title, 512U, false) ||
        !safeText(originalTitle, 512U, true) ||
        !safeText(overview, 16384U, true) ||
        !safeText(releaseDate, 64U, true) ||
        !safePoster(posterReference) ||
        seasonNumber < 0 || seasonNumber > 10000 ||
        episodeNumber < 0 || episodeNumber > 100000 ||
        rating < 0.0 || rating > 10.0)
        return false;

    switch (kind)
    {
    case RecordingMetadataCandidateKind::Movie:
        return externalNamespace == "movie" &&
            parentExternalId.empty() && seasonNumber == 0 && episodeNumber == 0;
    case RecordingMetadataCandidateKind::Series:
        return externalNamespace == "tv" &&
            parentExternalId.empty() && seasonNumber == 0 && episodeNumber == 0;
    case RecordingMetadataCandidateKind::Season:
        return externalNamespace == "tv-season" &&
            digits(parentExternalId) && seasonNumber > 0 && episodeNumber == 0;
    case RecordingMetadataCandidateKind::Episode:
        return externalNamespace == "tv-episode" &&
            digits(parentExternalId) && seasonNumber > 0 && episodeNumber > 0;
    }
    return false;
}

bool RecordingMetadataCastMember::valid() const
{
    return providerId == "tmdb" &&
        externalNamespace == "person" &&
        digits(externalId) &&
        safeText(name, 512U, false) &&
        safeText(characterName, 512U, true) &&
        safePoster(profileReference) &&
        order >= 0 && order < 100000;
}