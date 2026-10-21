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
    first.path = "/srv/vdr/video/Filme/Film/2026-10-09.20.15.00-0.rec";
    first.id = "backend-native-list-7";
    first.title = "Film";
    first.startTime = "2026-10-09T20:15:00Z";
    first.durationSeconds = 3600;
    first.recordingDurationKnown = true;
    first.sizeMb = 2048;
    first.metadata.native.description = "Beschreibung aus VDR";
    first.metadata.provider.overview = "Beschreibung aus Metadaten";

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
    assert(result.items[0].durationKnown);
    assert(result.items[0].description == "Beschreibung aus Metadaten");
    assert(result.items[0].sizeMb == 2048);
    // Keep the payload bounded and preserve UTF-8 codepoint boundaries.
    first.metadata.provider.overview = std::string(2047, 'a') + "\xc3\xa4" + "end";
    const auto longResult = projection.project("backend-a", {first});
    assert(longResult.valid);
    assert(longResult.items[0].description.size() == 2047);
    assert(longResult.items[0].description == std::string(2047, 'a'));
    first.metadata.provider.overview.clear();
    assert(projection.project("backend-a", {first}).items[0].description ==
        "Beschreibung aus VDR");
    assert(projection.project("backend-a", {first}).items[0].recordingId ==
           result.items[0].recordingId);

    // Web recordings2-shared.js selects the actual title directory,
    // decodes #XX/_ and removes leading % only from the display.
    VdrRecording original = first;
    original.backendNativeId = "/srv/vdr/video/Filme/Der_Hobbit/2026-10-09.rec";
    original.path = original.backendNativeId;
    original.title = "Filme~Der_Hobbit";
    VdrRecording cut = original;
    cut.backendNativeId = "/srv/vdr/video/Filme/%Der_Hobbit/2026-10-09.rec";
    cut.path = cut.backendNativeId;
    cut.title = "Filme~%Der_Hobbit";
    const auto variants = projection.project("backend-a", {original, cut});
    assert(variants.valid);
    assert(variants.items.size() == 2U);
    assert(variants.items[0].title == "Der Hobbit");
    assert(variants.items[1].title == "Der Hobbit");
    assert(variants.items[0].recordingId != variants.items[1].recordingId);
    assert(variants.items[0].edited != variants.items[1].edited);
    assert(!variants.items[0].edited || !variants.items[1].edited);
    assert(variants.items[0].edited || variants.items[1].edited);

    VdrRecording escaped = first;
    escaped.path = "/srv/vdr/video/Serien/%Der#20Hobbit#2FTeil_2/"
        "2026-10-09.20.15.00-0.rec";
    escaped.backendNativeId = escaped.path;
    const auto encoded = projection.project("backend-a", {escaped});
    assert(encoded.valid);
    assert(encoded.items[0].title == "Der Hobbit/Teil 2");
    assert(encoded.items[0].edited);
    escaped.path = "/srv/vdr/video/Serien/#2525Der_Hobbit/"
        "2026-10-09.20.15.00-0.rec";
    escaped.backendNativeId = escaped.path;
    assert(projection.project("backend-a", {escaped}).items[0].edited);
    escaped.path = "/srv/vdr/video/Serien/100%_Abenteuer/"
        "2026-10-09.20.15.00-0.rec";
    escaped.backendNativeId = escaped.path;
    const auto percent = projection.project("backend-a", {escaped});
    assert(percent.items[0].title == "100% Abenteuer");
    assert(!percent.items[0].edited);
    escaped.path = "/srv/vdr/video/Serien/Der#E4_Hobbit/"
        "2026-10-09.20.15.00-0.rec";
    escaped.backendNativeId = escaped.path;
    assert(projection.project("backend-a", {escaped}).items[0].title ==
           "Der\xc3\xa4 Hobbit");

    // Some recordings have no usable native path: Web falls back to a
    // last-segment title, never presenting the folder prefix.
    escaped.path.clear();
    escaped.backendNativeId.clear();
    escaped.backendNativeId = "recording-id";
    escaped.title = "Serien~%Hobbit_Spezial";
    const auto byTitle = projection.project("backend-a", {escaped});
    assert(byTitle.valid && byTitle.items.size() == 1U);
    assert(byTitle.items[0].title == "Hobbit Spezial");
    assert(byTitle.items[0].edited);

    assert(!projection.project("backend-b", {first}).valid);
    assert(!projection.project("*", {first}).valid);
    first.backendNativeId.clear();
    assert(!projection.project("backend-a", {first}).valid);
    assert(projection.project("backend-a", {}).valid);

    return 0;
}
