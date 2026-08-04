#pragma once

#include "RecordingMetadataCandidateProvider.h"

#include <cstddef>
#include <string>
#include <vector>

bool parseTmdbRecordingCandidateSearch(
    const std::string& body,
    std::size_t maximumBytes,
    RecordingMetadataCandidateKind kind,
    int limit,
    std::vector<RecordingMetadataCandidate>& candidates,
    bool& truncated);

bool parseTmdbRecordingCandidateSeasons(
    const std::string& body,
    std::size_t maximumBytes,
    const std::string& seriesExternalId,
    int limit,
    std::vector<RecordingMetadataCandidate>& candidates,
    bool& truncated);

bool parseTmdbRecordingCandidateEpisodes(
    const std::string& body,
    std::size_t maximumBytes,
    const std::string& seriesExternalId,
    int seasonNumber,
    int limit,
    std::vector<RecordingMetadataCandidate>& candidates,
    bool& truncated);
