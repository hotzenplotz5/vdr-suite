#include "PublicRecordingCollectionProjection.h"
#include "PublicRecordingIdentityRepository.h"
#include "Database.h"

#include <cassert>
#include <string>
#include <vector>

int main()
{
    Database db;
    assert(db.open(":memory:"));
    PublicRecordingIdentityRepository identities(db);
    assert(identities.ensureSchema());
    PublicRecordingCollectionProjection projection(identities);

    VdrRecording first;
    first.backendId = "backend-a";
    first.backendNativeId = "/private/source/2026.rec";
    first.path = "/srv/vdr/video/private-path";
    first.id = "backend-native-list-7";
    first.title = "Film";
    first.startTime = "2026-10-09T20:15:00Z";
    first.durationSeconds = 3600;

    const auto result = projection.project("backend-a", {first});
    assert(result.valid);
    assert(result.items.size() == 1);
    assert(result.items[0].recordingId.rfind("rec_", 0) == 0);
    assert(result.items[0].recordingId != first.id);
    assert(result.items[0].recordingId != first.backendNativeId);
    assert(result.items[0].backendId == "backend-a");
    assert(result.items[0].title == "Film");
    assert(result.items[0].recordedAt == first.startTime);
    assert(result.items[0].durationSeconds == 3600);
    assert(projection.project("backend-a", {first}).items[0].recordingId ==
           result.items[0].recordingId);

    assert(!projection.project("backend-b", {first}).valid);
    assert(!projection.project("*", {first}).valid);
    first.backendNativeId.clear();
    assert(!projection.project("backend-a", {first}).valid);
    assert(projection.project("backend-a", {}).valid);

    return 0;
}
