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
class GenreIndexRepository;

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

    std::string getManualRecordingGenre(
        const std::string& backendId,
        const std::string& resourceKey);

    bool setManualRecordingGenre(
        const std::string& backendId,
        const std::string& resourceKey,
        const std::string& genreId);

    bool clearManualRecordingGenre(
        const std::string& backendId,
        const std::string& resourceKey);

private:
    ManualRecordingMetadataAssignmentRepository& manualRepository();
    GenreIndexRepository& genreRepository();
    bool ensureManualPersonProfileSchema();

    Database& database_;
    std::unique_ptr<ManualRecordingMetadataAssignmentRepository>
        manualMetadataRepository_;
    std::mutex manualMetadataRepositoryMutex_;
    std::unique_ptr<GenreIndexRepository>
        genreIndexRepository_;
    std::mutex genreIndexRepositoryMutex_;
    std::mutex manualPersonProfileSchemaMutex_;
    bool manualPersonProfileSchemaAttempted_ = false;
    bool manualPersonProfileSchemaReady_ = false;
};