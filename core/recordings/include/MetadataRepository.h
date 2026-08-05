#pragma once

#include "ManualRecordingMetadataAssignmentRepository.h"
#include "Metadata.h"

#include <map>
#include <memory>
#include <mutex>
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

    std::map<std::string, ManualRecordingMetadataAssignment>
    getManualRecordingMetadataForBackend(
        const std::string& backendId);

private:
    ManualRecordingMetadataAssignmentRepository& manualRepository();
    bool ensureManualPersonProfileSchema();

    Database& database_;
    std::unique_ptr<ManualRecordingMetadataAssignmentRepository>
        manualMetadataRepository_;
    std::mutex manualMetadataRepositoryMutex_;
    std::mutex manualPersonProfileSchemaMutex_;
    bool manualPersonProfileSchemaAttempted_ = false;
    bool manualPersonProfileSchemaReady_ = false;
};