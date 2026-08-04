#include "MetadataRepository.h"

#include "Database.h"

bool MetadataRepository::assignManualRecordingMetadata(
    const ManualRecordingMetadataSelection& selection,
    ManualRecordingMetadataAssignment& assigned)
{
    ManualRecordingMetadataAssignmentRepository repository(database_);
    return repository.assign(selection, assigned);
}

bool MetadataRepository::withdrawManualRecordingMetadata(
    const std::string& backendId,
    const std::string& resourceKey,
    const std::string& actorRef,
    int expectedRevision,
    ManualRecordingMetadataAssignment& withdrawn)
{
    ManualRecordingMetadataAssignmentRepository repository(database_);
    return repository.withdraw(
        backendId,
        resourceKey,
        actorRef,
        expectedRevision,
        withdrawn);
}

ManualRecordingMetadataAssignment
MetadataRepository::getManualRecordingMetadata(
    const std::string& backendId,
    const std::string& resourceKey)
{
    ManualRecordingMetadataAssignmentRepository repository(database_);
    return repository.findSelected(backendId, resourceKey);
}
