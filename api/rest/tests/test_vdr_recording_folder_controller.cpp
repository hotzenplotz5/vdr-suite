#include "Database.h"
#include "VdrRecordingCacheRepository.h"
#include "VdrRecordingFolderController.h"

#include <sqlite3.h>

#include <cassert>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
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

static ManualRecordingMetadataAssignment rootManualAssignment()
{
    ManualRecordingMetadataAssignment assignment;
    assignment.found = true;
    assignment.backendId = "default";
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
    std::remove("/tmp/vdr-suite-movie-landscape.jpg");
    std::remove("/tmp/vdr-suite-movie-poster.jpg");

    {
        std::ofstream landscape(
            "/tmp/vdr-suite-movie-landscape.jpg",
            std::ios::binary);
        std::ofstream portrait(
            "/tmp/vdr-suite-movie-poster.jpg",
            std::ios::binary);
        landscape << "LANDSCAPE";
        portrait << "PORTRAIT";
    }

    Database database;
    assert(database.open("/tmp/test_vdr_recording_folder_controller.db"));

    VdrRecordingCacheRepository repository(database);

    VdrRecording cachedFallback = makeRecording(
        "drama-1",
        "Drama Recording",
        "/Drama/2026-07-04.20.15.1-0.rec");
    cachedFallback.metadata.native.eventTitle = "Mosquito Coast";
    cachedFallback.metadata.provider.source =
        VdrRecordingMetadataSource::RestfulApiScraperBridge;
    cachedFallback.metadata.provider.contentKind =
        VdrRecordingContentKind::Movie;
    cachedFallback.metadata.provider.movieId = "11120";
    cachedFallback.metadata.provider.title = "Mosquito Coast";
    cachedFallback.metadata.provider.originalTitle = "The Mosquito Coast";
    cachedFallback.metadata.provider.tagline = "Reise in die Wildnis";
    cachedFallback.metadata.provider.overview =
        "Persistierte TVScraper-Beschreibung";
    cachedFallback.metadata.provider.genreText = "|Drama|Abenteuer|";
    cachedFallback.metadata.provider.releaseDate = "1986-11-26";
    cachedFallback.metadata.provider.runtimeMinutes = 117;
    cachedFallback.metadata.provider.rating = 6.367;

    VdrRecordingArtworkRef cachedPoster;
    cachedPoster.kind = VdrRecordingArtworkKind::Poster;
    cachedPoster.source =
        VdrRecordingMetadataSource::RestfulApiScraperBridge;
    cachedPoster.reference =
        "var/cache/vdr/plugins/tvscraper/movies/11120_poster.jpg";
    cachedFallback.metadata.artwork.push_back(cachedPoster);

    VdrRecordingArtworkRef cachedFanart;
    cachedFanart.kind = VdrRecordingArtworkKind::Fanart;
    cachedFanart.source =
        VdrRecordingMetadataSource::RestfulApiScraperBridge;
    cachedFanart.reference =
        "var/cache/vdr/plugins/tvscraper/movies/11120_backdrop.jpg";
    cachedFallback.metadata.artwork.push_back(cachedFanart);

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

    int manualLookupCalls = 0;
    int manualBatchLookupCalls = 0;
    int nativeLookupCalls = 0;
    VdrRecordingFolderController controller(
        repository,
        [&nativeLookupCalls](
            const std::string& backendId,
            const std::string& backendNativeId)
        {
            ++nativeLookupCalls;
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

            VdrRecordingNativeArtwork landscape;
            landscape.available = true;
            landscape.provider = "tvscraper";
            landscape.path =
                "/tmp/vdr-suite-movie-landscape.jpg";
            landscape.width = 1600;
            landscape.height = 900;
            landscape.orientation = "landscape";
            record.metadata.preferredArtwork = landscape;

            VdrRecordingNativeArtwork portrait;
            portrait.available = true;
            portrait.provider = "tvscraper";
            portrait.path =
                "/tmp/vdr-suite-movie-poster.jpg";
            portrait.width = 1000;
            portrait.height = 1500;
            portrait.orientation = "portrait";
            record.metadata.images.push_back(portrait);

            VdrRecordingNativePerson person;
            person.role = "actor";
            person.name = "Tom Hanks";
            person.characterName = "Robert Langdon";
            record.metadata.people.push_back(person);

            return record;
        },
        [&manualLookupCalls](
            const std::string& backendId,
            const std::string& backendNativeId)
        {
            ++manualLookupCalls;
            if (backendId != "default" ||
                backendNativeId.find("2026-07-03") == std::string::npos)
            {
                return ManualRecordingMetadataAssignment{};
            }
            return rootManualAssignment();
        },
        [&manualBatchLookupCalls](const std::string& backendId)
        {
            ++manualBatchLookupCalls;
            std::map<std::string, ManualRecordingMetadataAssignment> assignments;
            if (backendId == "default")
            {
                assignments.emplace(
                    "/srv/vdr/video/2026-07-03.20.15.1-0.rec",
                    rootManualAssignment());
            }
            return assignments;
        },
        {"/tmp"});

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
    assert(manualBatchLookupCalls == 1);
    assert(manualLookupCalls == 0);
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
        "\"posterUrl\":\"/api/vdr/recordings/metadata/image?backend=default&backendNativeId=%2Fsrv%2Fvdr%2Fvideo%2F2026-07-03.20.15.1-0.rec&kind=preferred&index=0&assignmentRevision=3\""));
    assert(!contains(
        root.body,
        "/var/cache/vdr-suite/recording-metadata/posters/manual.webp"));

    const ApiResponse series =
        controller.getFolder("default", "Series", 20, 0);

    assert(series.statusCode == 200);
    assert(manualBatchLookupCalls == 2);
    assert(manualLookupCalls == 0);
    assert(contains(series.body, "\"Show\""));
    assert(contains(series.body, "\"parentPath\":\"\""));
    assert(contains(series.body, "\"metadata\":{"));

    const int nativeCallsBeforeMovies =
        nativeLookupCalls;

    const ApiResponse movies =
        controller.getFolder("default", "Movies", 20, 0);

    assert(movies.statusCode == 200);
    assert(nativeLookupCalls > nativeCallsBeforeMovies);
    assert(contains(
        movies.body,
        "\"nativeMetadata\":{\"available\":true"));
    assert(contains(
        movies.body,
        "\"provider\":\"tvscraper\""));
    assert(contains(
        movies.body,
        "\"orientation\":\"portrait\""));
    assert(contains(
        movies.body,
        "&kind=gallery&index=0"));
    assert(!contains(
        movies.body,
        "/tmp/vdr-suite-movie-poster.jpg"));

    const ApiResponse rootAgain =
        controller.getFolder("default", "", 20, 0);

    assert(rootAgain.statusCode == 200);
    assert(manualBatchLookupCalls == 4);
    assert(manualLookupCalls == 0);
    assert(traceState.recordingInventoryReads == 0);

    sqlite3_trace_v2(
        database.handle(),
        0,
        nullptr,
        nullptr);

    const ApiResponse nativeMetadata = controller.getMetadata(
        "default",
        "/srv/vdr/video/Movies/Movie/2026-07-02.20.15.1-0.rec");
    assert(manualLookupCalls == 1);
    assert(nativeMetadata.statusCode == 200);
    assert(contains(nativeMetadata.body, "\"available\":true"));
    assert(contains(nativeMetadata.body, "\"provider\":\"tvscraper\""));
    assert(contains(nativeMetadata.body, "\"name\":\"Tom Hanks\""));
    assert(contains(nativeMetadata.body, "\"characterName\":\"Robert Langdon\""));

    const ApiResponse posterImage = controller.getMetadataImage(
        "default",
        "/srv/vdr/video/Movies/Movie/2026-07-02.20.15.1-0.rec",
        "poster",
        0);
    assert(posterImage.statusCode == 200);
    assert(posterImage.contentType == "image/jpeg");
    assert(posterImage.body == "PORTRAIT");

    const ApiResponse preferredImage = controller.getMetadataImage(
        "default",
        "/srv/vdr/video/Movies/Movie/2026-07-02.20.15.1-0.rec",
        "preferred",
        0);
    assert(preferredImage.statusCode == 200);
    assert(preferredImage.body == "LANDSCAPE");

    const ApiResponse missingMetadata = controller.getMetadata(
        "default",
        "/srv/vdr/video/Movies/Missing.rec");
    assert(manualLookupCalls == 4);
    assert(missingMetadata.statusCode == 200);
    assert(contains(missingMetadata.body, "\"available\":false"));

    assert(repository.upsertRecordingsForBackend(
        "default",
        {cachedFallback}));

    const ApiResponse cachedMetadata = controller.getMetadata(
        "default",
        cachedFallback.backendNativeId);
    assert(cachedMetadata.statusCode == 200);
    assert(contains(cachedMetadata.body, "\"available\":true"));
    assert(contains(cachedMetadata.body, "\"provider\":\"tvscraper\""));
    assert(contains(cachedMetadata.body, "\"providerId\":11120"));
    assert(contains(cachedMetadata.body, "\"title\":\"Mosquito Coast\""));
    assert(contains(
        cachedMetadata.body,
        "\"overview\":\"Persistierte TVScraper-Beschreibung\""));
    assert(contains(cachedMetadata.body, "\"genres\":[\"Drama\",\"Abenteuer\"]"));
    assert(contains(cachedMetadata.body, "\"voteAverage\":6.367"));
    assert(contains(cachedMetadata.body, "\"orientation\":\"portrait\""));
    assert(contains(cachedMetadata.body, "\"orientation\":\"landscape\""));
    assert(contains(cachedMetadata.body, "/recording-artwork/default/"));
    assert(!contains(
        cachedMetadata.body,
        "var/cache/vdr/plugins/tvscraper"));

    std::remove("/tmp/vdr-suite-movie-landscape.jpg");
    std::remove("/tmp/vdr-suite-movie-poster.jpg");

    std::cout
        << "test_vdr_recording_folder_controller passed"
        << std::endl;

    return 0;
}
