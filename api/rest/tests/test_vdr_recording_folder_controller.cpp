#include "Database.h"
#include "VdrRecordingCacheRepository.h"
#include "VdrRecordingFolderController.h"

#include <sqlite3.h>

#include <cassert>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <string>

static VdrRecording makeRecording(
    const std::string& id,
    const std::string& title,
    const std::string& path)
{
    VdrRecording recording;

    recording.id = id;
    recording.backendId = "default";
    recording.backendNativeId = "/srv/vdr/video" + path;
    recording.title = title;
    recording.path = path;
    recording.startTime = "1782928800";
    recording.durationSeconds = 3600;
    recording.sizeMb = 1024;

    return recording;
}

static bool contains(
    const std::string& text,
    const std::string& needle)
{
    return text.find(needle) != std::string::npos;
}

struct SqlTraceState
{
    int recordingInventoryReads = 0;
};

static int traceSql(
    unsigned int traceKind,
    void* context,
    void* statement,
    void*)
{
    if (traceKind != SQLITE_TRACE_STMT ||
        context == nullptr || statement == nullptr)
    {
        return 0;
    }

    const char* sql =
        sqlite3_sql(static_cast<sqlite3_stmt*>(statement));

    if (sql != nullptr &&
        std::strstr(sql, "SELECT recording_id, backend_id") != nullptr &&
        std::strstr(sql, "FROM vdr_recording_cache") != nullptr)
    {
        static_cast<SqlTraceState*>(context)
            ->recordingInventoryReads += 1;
    }

    return 0;
}

int main()
{
    std::remove("/tmp/test_vdr_recording_folder_controller.db");

    Database database;
    assert(database.open("/tmp/test_vdr_recording_folder_controller.db"));

    VdrRecordingCacheRepository repository(database);

    assert(repository.replaceRecordingsForBackend(
        "default",
        {
            makeRecording(
                "series-1",
                "Series Episode",
                "/Series/Show/2026-07-01.20.15.1-0.rec"),
            makeRecording(
                "movie-1",
                "Movie",
                "/Movies/Movie/2026-07-02.20.15.1-0.rec"),
            makeRecording(
                "root-1",
                "Root Recording",
                "/2026-07-03.20.15.1-0.rec"),
            makeRecording(
                "drama-1",
                "Drama Recording",
                "/Drama/2026-07-04.20.15.1-0.rec")
        }));

    assert(repository.markRefreshFinished("default", 4));
    assert(repository.warmBrowseSnapshotForBackend("default"));

    VdrRecordingFolderController controller(
        repository,
        [](
            const std::string& backendId,
            const std::string& backendNativeId)
        {
            VdrRecordingNativeMetadataRecord record;
            if (backendId != "default" ||
                backendNativeId.find("Movie") == std::string::npos)
            {
                return record;
            }

            record.backendId = backendId;
            record.backendNativeId = backendNativeId;
            record.recordingKey = "movie-key";
            record.contentState = "found";
            record.metadata.found = true;
            record.metadata.provider = "tvscraper";
            record.metadata.mediaType = "movie";
            record.metadata.title = "Movie";

            VdrRecordingNativePerson person;
            person.role = "actor";
            person.name = "Tom Hanks";
            person.characterName = "Robert Langdon";
            record.metadata.people.push_back(person);

            return record;
        },
        [](
            const std::string& backendId,
            const std::string& backendNativeId)
        {
            ManualRecordingMetadataAssignment assignment;
            if (backendId != "default" ||
                backendNativeId.find("2026-07-03") == std::string::npos)
            {
                return assignment;
            }

            assignment.found = true;
            assignment.backendId = backendId;
            assignment.resourceKey = "root-key";
            assignment.providerId = "tmdb";
            assignment.externalNamespace = "movie";
            assignment.externalId = "754";
            assignment.mediaType = "movie";
            assignment.title = "Face/Off - Im Körper des Feindes";
            assignment.originalTitle = "Face/Off";
            assignment.overview = "Manuell ausgewählte Beschreibung";
            assignment.releaseDate = "1997-06-27";
            assignment.posterReference =
                "/var/cache/vdr-suite/recording-metadata/posters/manual.webp";
            assignment.revision = 3;
            assignment.relationshipLocked = true;
            return assignment;
        },
        {});

    const ApiResponse status =
        controller.getStatus("default");

    assert(status.statusCode == 200);
    assert(contains(status.body, "\"state\":\"ready\""));
    assert(contains(status.body, "\"totalCount\":4"));

    SqlTraceState traceState;
    assert(sqlite3_trace_v2(
        database.handle(),
        SQLITE_TRACE_STMT,
        traceSql,
        &traceState) == SQLITE_OK);

    const ApiResponse root =
        controller.getFolder("default", "", 20, 0);

    assert(root.statusCode == 200);
    assert(contains(root.body, "\"recordingFolder\":true"));
    assert(contains(root.body, "\"Series\""));
    assert(contains(root.body, "\"Movies\""));
    assert(contains(root.body, "\"Drama\""));
    assert(contains(root.body, "\"Root Recording\""));
    assert(contains(root.body, "\"metadata\":{"));
    assert(contains(root.body, "\"providerAvailable\":false"));
    assert(contains(root.body, "\"artworkPrepared\":false"));
    assert(contains(root.body, "\"placeholderVariant\":"));
    assert(contains(root.body, "\"singleRecordingLeaf\":true"));
    assert(contains(root.body, "\"singleRecording\":{\"id\":\"drama-1\""));
    assert(contains(root.body, "\"source\":\"manual\""));
    assert(contains(
        root.body,
        "\"title\":\"Face/Off - Im Körper des Feindes\""));
    assert(contains(
        root.body,
        "\"summary\":\"Manuell ausgewählte Beschreibung\""));
    assert(contains(
        root.body,
        "\"manualAssignment\":{\"active\":true,\"revision\":3"));
    assert(contains(
        root.body,
        "\"posterUrl\":\"/api/vdr/recordings/metadata/image?backend=default&backendNativeId=%2Fsrv%2Fvdr%2Fvideo%2F2026-07-03.20.15.1-0.rec&kind=preferred&index=0\""));
    assert(!contains(
        root.body,
        "/var/cache/vdr-suite/recording-metadata/posters/manual.webp"));

    const ApiResponse series =
        controller.getFolder("default", "Series", 20, 0);

    assert(series.statusCode == 200);
    assert(contains(series.body, "\"Show\""));
    assert(contains(series.body, "\"parentPath\":\"\""));
    assert(contains(series.body, "\"metadata\":{"));

    const ApiResponse rootAgain =
        controller.getFolder("default", "", 20, 0);

    assert(rootAgain.statusCode == 200);
    assert(traceState.recordingInventoryReads == 0);

    sqlite3_trace_v2(
        database.handle(),
        0,
        nullptr,
        nullptr);

    const ApiResponse nativeMetadata = controller.getMetadata(
        "default",
        "/srv/vdr/video/Movies/Movie/2026-07-02.20.15.1-0.rec");
    assert(nativeMetadata.statusCode == 200);
    assert(contains(nativeMetadata.body, "\"available\":true"));
    assert(contains(nativeMetadata.body, "\"provider\":\"tvscraper\""));
    assert(contains(nativeMetadata.body, "\"name\":\"Tom Hanks\""));
    assert(contains(nativeMetadata.body, "\"characterName\":\"Robert Langdon\""));

    const ApiResponse missingMetadata = controller.getMetadata(
        "default",
        "/srv/vdr/video/Movies/Missing.rec");
    assert(missingMetadata.statusCode == 200);
    assert(contains(missingMetadata.body, "\"available\":false"));

    std::cout
        << "test_vdr_recording_folder_controller passed"
        << std::endl;

    return 0;
}
