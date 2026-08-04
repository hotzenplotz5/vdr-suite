#pragma once

#include "DashboardController.h"
#include "ManualRecordingMetadataAssignmentRepository.h"
#include "MetadataRepository.h"
#include "RecordingMetadataCandidateProvider.h"

#include <map>
#include <string>

class IRecordingMetadataCandidateProvider;

class MetadataController
{
public:
    explicit MetadataController(
        MetadataRepository& metadataRepository,
        IRecordingMetadataCandidateProvider* candidateProvider = nullptr);

    ApiResponse getMetadata();

    ApiResponse searchRecordingMetadataCandidates(
        const std::string& backendId,
        const std::string& query,
        RecordingMetadataCandidateKind kind,
        int limit);

    ApiResponse searchRecordingMetadataCandidates(
        const std::string& query,
        RecordingMetadataCandidateKind kind,
        int limit)
    {
        return searchRecordingMetadataCandidates(
            "default",
            query,
            kind,
            limit);
    }

    ApiResponse getRecordingMetadataSeasons(
        const std::string& backendId,
        const std::string& seriesExternalId,
        int limit);

    ApiResponse getRecordingMetadataSeasons(
        const std::string& seriesExternalId,
        int limit)
    {
        return getRecordingMetadataSeasons(
            "default",
            seriesExternalId,
            limit);
    }

    ApiResponse getRecordingMetadataEpisodes(
        const std::string& backendId,
        const std::string& seriesExternalId,
        int seasonNumber,
        int limit);

    ApiResponse getRecordingMetadataEpisodes(
        const std::string& seriesExternalId,
        int seasonNumber,
        int limit)
    {
        return getRecordingMetadataEpisodes(
            "default",
            seriesExternalId,
            seasonNumber,
            limit);
    }

    ManualRecordingMetadataAssignment findManualRecordingMetadata(
        const std::string& backendId,
        const std::string& resourceKey)
    {
        return metadataRepository_.getManualRecordingMetadata(
            backendId,
            resourceKey);
    }

    std::map<std::string, ManualRecordingMetadataAssignment>
    findManualRecordingMetadataForBackend(
        const std::string& backendId)
    {
        return metadataRepository_.getManualRecordingMetadataForBackend(
            backendId);
    }

    ApiResponse getManualRecordingMetadata(
        const std::string& backendId,
        const std::string& resourceKey);

    ApiResponse assignManualRecordingMetadata(
        ManualRecordingMetadataSelection selection,
        const std::string& actorRef);

    ApiResponse withdrawManualRecordingMetadata(
        const std::string& backendId,
        const std::string& resourceKey,
        int expectedRevision,
        const std::string& actorRef);

private:
    MetadataRepository& metadataRepository_;
    IRecordingMetadataCandidateProvider* candidateProvider_;
};
