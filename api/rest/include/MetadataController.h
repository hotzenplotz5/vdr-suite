#pragma once

#include "DashboardController.h"
#include "ManualRecordingMetadataAssignmentRepository.h"
#include "RecordingMetadataCandidateProvider.h"

#include <string>

class IRecordingMetadataCandidateProvider;
class MetadataRepository;

class MetadataController
{
public:
    explicit MetadataController(
        MetadataRepository& metadataRepository,
        IRecordingMetadataCandidateProvider* candidateProvider = nullptr);

    ApiResponse getMetadata();

    ApiResponse searchRecordingMetadataCandidates(
        const std::string& query,
        RecordingMetadataCandidateKind kind,
        int limit);

    ApiResponse getRecordingMetadataSeasons(
        const std::string& seriesExternalId,
        int limit);

    ApiResponse getRecordingMetadataEpisodes(
        const std::string& seriesExternalId,
        int seasonNumber,
        int limit);

    ManualRecordingMetadataAssignment findManualRecordingMetadata(
        const std::string& backendId,
        const std::string& resourceKey);

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
    IRecordingMetadataCandidateProvider& candidateProvider();

    MetadataRepository& metadataRepository_;
    IRecordingMetadataCandidateProvider* candidateProvider_;
};
