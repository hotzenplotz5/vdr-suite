#pragma once

#include <mutex>
#include <string>
#include <vector>

class Database;

struct ManualRecordingMetadataPerson
{
    std::string metadataEntityId;
    std::string providerId;
    std::string externalNamespace;
    std::string externalId;
    std::string name;
    std::string normalizedName;
    std::string role = "actor";
    std::string characterName;
    int ordinal = 0;
};

struct ManualRecordingMetadataSelection
{
    std::string backendId;
    std::string resourceKey;
    std::string providerId;
    std::string externalNamespace;
    std::string externalId;
    std::string mediaType;
    std::string title;
    std::string originalTitle;
    std::string overview;
    std::string releaseDate;
    std::string posterReference;
    int seasonNumber = 0;
    int episodeNumber = 0;
    std::string actorRef;
    int expectedRevision = 0;
    bool castComplete = false;
    std::vector<ManualRecordingMetadataPerson> people;
};

struct ManualRecordingMetadataAssignment
{
    bool found = false;
    std::string backendId;
    std::string resourceKey;
    std::string metadataTargetId;
    std::string metadataAssignmentId;
    std::string metadataEntityId;
    std::string providerId;
    std::string externalNamespace;
    std::string externalId;
    std::string mediaType;
    std::string title;
    std::string originalTitle;
    std::string overview;
    std::string releaseDate;
    std::string posterReference;
    int seasonNumber = 0;
    int episodeNumber = 0;
    std::string actorRef;
    int revision = 0;
    bool relationshipLocked = false;
    bool castComplete = false;
    std::vector<ManualRecordingMetadataPerson> people;
};

class ManualRecordingMetadataAssignmentRepository
{
public:
    explicit ManualRecordingMetadataAssignmentRepository(Database& database);

    bool ensureSchema();

    bool assign(
        const ManualRecordingMetadataSelection& selection,
        ManualRecordingMetadataAssignment& assigned);

    bool withdraw(
        const std::string& backendId,
        const std::string& resourceKey,
        const std::string& actorRef,
        int expectedRevision,
        ManualRecordingMetadataAssignment& withdrawn);

    ManualRecordingMetadataAssignment findSelected(
        const std::string& backendId,
        const std::string& resourceKey) const;

private:
    Database& database_;
    mutable std::recursive_mutex mutex_;
    mutable bool schemaReady_ = false;

    bool ensureSchemaLocked() const;
};
