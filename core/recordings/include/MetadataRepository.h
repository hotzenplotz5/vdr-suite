#pragma once

#include "ManualRecordingMetadataAssignmentRepository.h"
#include "Metadata.h"

#include <optional>
#include <string>
#include <vector>

class Database;

class MetadataRepository
{
public:
    explicit MetadataRepository(Database& database);

    std::vector<Metadata> getAllMetadata();
    std::optional<Metadata> getMetadataForRecording(int recordingId);

    bool assignManualRecordingMetadata(
        const ManualRecordingMetadataSelection& selection,
        ManualRecordingMetadataAssignment& assigned);

    bool withdrawManualRecordingMetadata(
        const std::string& backendId,
        const std::string& resourceKey,
        const std::string& actorRef,
        int expectedRevision,
        ManualRecordingMetadataAssignment& withdrawn);

    ManualRecordingMetadataAssignment getManualRecordingMetadata(
        const std::string& backendId,
        const std::string& resourceKey);

private:
    Database& database_;
};
