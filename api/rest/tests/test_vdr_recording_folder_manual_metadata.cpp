#include "Database.h"
#include "VdrRecordingCacheRepository.h"
#include "VdrRecordingFolderController.h"

#include <cassert>
#include <string>
#include <vector>

namespace
{
bool contains(const std::string& value, const std::string& needle)
{
    return value.find(needle) != std::string::npos;
}

VdrRecordingNativeMetadataRecord nativeRecord(
    const std::string& backendId,
    const std::string& backendNativeId)
{
    VdrRecordingNativeMetadataRecord record;
    record.backendId = backendId;
    record.backendNativeId = backendNativeId;
    record.recordingKey = "native-recording";
    record.contentState = "found";
    record.metadata.found = true;
    record.metadata.provider = "tvscraper";
    record.metadata.mediaType = "movie";
    record.metadata.title = "Automatischer Titel";
    record.metadata.overview = "Automatische Beschreibung";
    VdrRecordingNativePerson person;
    person.role = "actor";
    person.name = "Native Person";
    record.metadata.people.push_back(person);
    return record;
}
}

int main()
{
    Database database;
    assert(database.open(":memory:"));
    VdrRecordingCacheRepository repository(database);

    ManualRecordingMetadataAssignment manual;
    manual.found = true;
    manual.backendId = "default";
    manual.resourceKey = "default:/video/example.rec";
    manual.metadataTargetId = "mdtgt_0123456789abcdef0123456789abcdef";
    manual.metadataAssignmentId = "mdasg_0123456789abcdef0123456789abcdef";
    manual.metadataEntityId = "mdent_0123456789abcdef0123456789abcdef";
    manual.providerId = "tmdb";
    manual.externalNamespace = "movie";
    manual.externalId = "13";
    manual.mediaType = "movie";
    manual.title = "Manueller Titel";
    manual.originalTitle = "Manual Title";
    manual.overview = "Manuelle Beschreibung";
    manual.releaseDate = "1994-07-06";
    manual.actorRef = "user:test-admin";
    manual.revision = 2;
    manual.relationshipLocked = true;

    VdrRecordingFolderController controller(
        repository,
        [](
            const std::string& backendId,
            const std::string& backendNativeId)
        {
            return nativeRecord(backendId, backendNativeId);
        },
        [&](const std::string&, const std::string&)
        {
            return manual;
        },
        std::vector<std::string>{});

    const ApiResponse selected = controller.getMetadata(
        "default",
        "/video/example.rec");
    assert(selected.statusCode == 200);
    assert(contains(selected.body, "\"provider\":\"manual\""));
    assert(contains(selected.body, "\"title\":\"Manueller Titel\""));
    assert(contains(selected.body, "\"manualAssignment\":{"));
    assert(contains(selected.body, "\"revision\":2"));
    assert(contains(selected.body, "\"relationshipLocked\":true"));
    assert(!contains(selected.body, "Native Person"));

    manual.relationshipLocked = false;
    const ApiResponse unlocked = controller.getMetadata(
        "default",
        "/video/example.rec");
    assert(unlocked.statusCode == 200);
    assert(contains(unlocked.body, "\"provider\":\"tvscraper\""));
    assert(contains(unlocked.body, "\"title\":\"Automatischer Titel\""));
    assert(contains(unlocked.body, "Native Person"));

    manual.found = false;
    const ApiResponse withdrawn = controller.getMetadata(
        "default",
        "/video/example.rec");
    assert(withdrawn.statusCode == 200);
    assert(contains(withdrawn.body, "\"provider\":\"tvscraper\""));

    database.close();
    return 0;
}
