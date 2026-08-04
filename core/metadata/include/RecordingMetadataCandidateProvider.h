#pragma once

#include <string>
#include <vector>

enum class RecordingMetadataCandidateKind
{
    Movie,
    Series,
    Season,
    Episode
};

const char* recordingMetadataCandidateKindName(
    RecordingMetadataCandidateKind kind);

struct RecordingMetadataCandidate
{
    RecordingMetadataCandidateKind kind =
        RecordingMetadataCandidateKind::Movie;
    std::string providerId;
    std::string externalNamespace;
    std::string externalId;
    std::string parentExternalId;
    std::string title;
    std::string originalTitle;
    std::string overview;
    std::string releaseDate;
    std::string posterReference;
    int seasonNumber = 0;
    int episodeNumber = 0;
    double rating = 0.0;

    bool valid() const;
};

struct RecordingMetadataCandidatePage
{
    bool attempted = false;
    bool providerAvailable = false;
    bool truncated = false;
    std::string providerId;
    std::string error;
    std::vector<RecordingMetadataCandidate> candidates;
};

class IRecordingMetadataCandidateProvider
{
public:
    virtual ~IRecordingMetadataCandidateProvider() = default;

    virtual RecordingMetadataCandidatePage search(
        const std::string& query,
        RecordingMetadataCandidateKind kind,
        int limit) = 0;

    virtual RecordingMetadataCandidatePage seasons(
        const std::string& seriesExternalId,
        int limit) = 0;

    virtual RecordingMetadataCandidatePage episodes(
        const std::string& seriesExternalId,
        int seasonNumber,
        int limit) = 0;
};
