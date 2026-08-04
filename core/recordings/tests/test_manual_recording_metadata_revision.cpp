#include "Database.h"
#include "MetadataRepository.h"

#include <cassert>

namespace
{
ManualRecordingMetadataSelection selection(
    const std::string& externalId,
    int expectedRevision)
{
    ManualRecordingMetadataSelection value;
    value.backendId = "default";
    value.resourceKey = "/video/example.rec";
    value.providerId = "tmdb";
    value.externalNamespace = "movie";
    value.externalId = externalId;
    value.mediaType = "movie";
    value.title = "Example";
    value.originalTitle = "Example";
    value.overview = "Manual selection";
    value.releaseDate = "2020-01-01";
    value.actorRef = "user:test-admin";
    value.expectedRevision = expectedRevision;
    return value;
}
}

int main()
{
    Database database;
    assert(database.open(":memory:"));
    MetadataRepository repository(database);

    ManualRecordingMetadataAssignment first;
    assert(repository.assignManualRecordingMetadata(
        selection("100", 0),
        first));
    assert(first.found);
    assert(first.revision == 1);

    ManualRecordingMetadataAssignment withdrawn;
    assert(repository.withdrawManualRecordingMetadata(
        "default",
        "/video/example.rec",
        "user:test-admin",
        first.revision,
        withdrawn));

    ManualRecordingMetadataAssignment second;
    assert(repository.assignManualRecordingMetadata(
        selection("200", 0),
        second));
    assert(second.found);
    assert(second.revision == 2);
    assert(second.externalId == "200");

    ManualRecordingMetadataAssignment stale;
    assert(!repository.assignManualRecordingMetadata(
        selection("300", first.revision),
        stale));
    assert(!stale.found);

    const ManualRecordingMetadataAssignment current =
        repository.getManualRecordingMetadata(
            "default",
            "/video/example.rec");
    assert(current.found);
    assert(current.revision == second.revision);
    assert(current.externalId == "200");

    database.close();
    return 0;
}
