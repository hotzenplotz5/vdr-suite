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

    controller.setPublicRecordingIdLookup(
        [](const std::string& backendId, const std::string& nativeId)
        {
            return backendId == "default" &&
                nativeId == "/srv/vdr/video/2026-07-03.20.15.1-0.rec"
                ? std::string("rec_0123456789abcdef0123456789abcdef")
                : std::string{};
        });
    controller.setPublicRecordingNativeLookup(
        [](const std::string& backendId, const std::string& publicId)
        {
            return backendId == "default" &&
                publicId == "rec_0123456789abcdef0123456789abcdef"
                ? std::string("/srv/vdr/video/2026-07-03.20.15.1-0.rec")
                : std::string{};
        });
    const auto publicId = "rec_0123456789abcdef0123456789abcdef";
    const auto browserDetail = controller.getRecordingForBrowser("default", publicId);
    assert(browserDetail.statusCode == 200);
    assert(contains(browserDetail.body, "\"publicRecordingId\":\"rec_0123456789abcdef0123456789abcdef\""));
    assert(contains(browserDetail.body, "\"id\":\"root-1\""));
    assert(contains(browserDetail.body, "\"backendNativeId\":\"/srv/vdr/video/2026-07-03.20.15.1-0.rec\""));
    assert(contains(browserDetail.body, "\"metadataDetail\":{"));
    assert(contains(browserDetail.body, "Manuell ausgewählte Beschreibung"));
    assert(controller.getRecordingForBrowser("other", publicId).statusCode == 404);
    assert(controller.getRecordingForBrowser("default", "root-1").statusCode == 400);
    assert(controller.getRecordingForBrowser("default",
        "rec_ffffffffffffffffffffffffffffffff").statusCode == 404);
    // Leave the existing folder batching assertions independent.
    manualLookupCalls = 0;
    manualBatchLookupCalls = 0;
    nativeLookupCalls = 0;

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
    const auto seriesId = VdrRecordingCacheRepository::publicFolderId("default", "Series");
    assert(contains(root.body, "\"folderId\":\"" + seriesId + "\""));
    assert(contains(root.body, "\"Movies\""));
    assert(contains(root.body, "\"Drama\""));
    assert(contains(root.body, "\"Root Recording\""));
    assert(contains(root.body, "\"publicRecordingId\":\"rec_0123456789abcdef0123456789abcdef\""));
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
    assert(contains(series.body, "\"folderId\":\"" + seriesId + "\""));
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

    // Combined card paging includes embedded leaves in the budget, not
    // merely the direct-recording slice. Legacy callers remain unchanged.
    const int beforeHomeNative = nativeLookupCalls;
    const auto homeFirst = controller.getFolder("default", "", 1, 0, true);
    assert(homeFirst.statusCode == 200);
    assert(contains(homeFirst.body, "\"homeCardPage\":true"));
    assert(contains(homeFirst.body, "\"cardCount\":4"));
    assert(contains(homeFirst.body, "\"returnedCards\":1"));
    assert(contains(homeFirst.body, "\"hasMore\":true"));
    assert(contains(homeFirst.body, "\"singleRecording\":{\"id\":\"drama-1\""));
    assert(!contains(homeFirst.body, "\"name\":\"Movies\""));
    assert(!contains(homeFirst.body, "\"id\":\"root-1\""));
    assert(nativeLookupCalls - beforeHomeNative <= 1);
    const auto homeMiddle = controller.getFolder("default", "", 2, 1, true);
    assert(contains(homeMiddle.body, "\"returnedCards\":2"));
    assert(contains(homeMiddle.body, "\"name\":\"Movies\""));
    assert(contains(homeMiddle.body, "\"name\":\"Series\""));
    assert(!contains(homeMiddle.body, "\"id\":\"root-1\""));
    const auto homeMixed = controller.getFolder("default", "", 2, 2, true);
    assert(contains(homeMixed.body, "\"returnedCards\":2"));
    assert(contains(homeMixed.body, "\"name\":\"Series\""));
    assert(contains(homeMixed.body, "\"id\":\"root-1\""));
    assert(contains(homeMixed.body, "\"hasMore\":false"));
    const auto homeLast = controller.getFolder("default", "", 2, 3, true);
    assert(contains(homeLast.body, "\"returnedCards\":1"));
    assert(contains(homeLast.body, "\"hasMore\":false"));
    assert(contains(homeLast.body, "\"id\":\"root-1\""));
    assert(contains(homeLast.body, "\"manualAssignment\":{\"active\":true"));
    assert(contains(homeLast.body, "\"folders\":[]"));
    const auto homePastEnd = controller.getFolder("default", "", 12, 999, true);
    assert(contains(homePastEnd.body, "\"returnedCards\":0"));
    assert(contains(homePastEnd.body, "\"hasMore\":false"));
    assert(!contains(root.body, "\"homeCardPage\""));
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

    // Deep multi-recording subfolders: one canonical portrait address per
    // combined card, no recursive HTTP requests or complete episode payload.
    assert(repository.replaceRecordingsForBackend("default", {
        makeRecording("ep-b", "B Episode", "/Series/Movie Show/Season 2/2.rec"),
        makeRecording("ep-a", "A Episode", "/Series/Movie Show/Season 1/1.rec"),
        makeRecording("another", "Other Episode", "/Series/Z Show/Season 1/3.rec"),
        makeRecording("another-b", "Other Two", "/Series/Z Show/Season 2/4.rec")
    }));
    assert(repository.markRefreshFinished("default", 4));
    const auto beforeRepresentative = nativeLookupCalls;
    const auto representativePage = controller.getFolder("default", "Series", 1, 0, true);
    assert(contains(representativePage.body, "\"returnedCards\":1"));
    assert(contains(representativePage.body, "\"hasMore\":true"));
    assert(contains(representativePage.body, "\"singleRecordingLeaf\":false"));
    assert(contains(representativePage.body, "\"representativePosterUrl\":\"/api/vdr/recordings/metadata/image?backend=default&backendNativeId=%2Fsrv%2Fvdr%2Fvideo%2FSeries%2FMovie%20Show%2FSeason%201%2F1.rec&kind=gallery&index=0\""));
    assert(!contains(representativePage.body, "\"name\":\"Z Show\""));
    assert(!contains(representativePage.body, "\"nativeMetadata\""));
    assert(!contains(representativePage.body, "/tmp/vdr-suite-movie-poster.jpg"));
    assert(nativeLookupCalls == beforeRepresentative + 1);
    const auto legacySeries = controller.getFolder("default", "Series", 1, 0);
    assert(!contains(legacySeries.body, "representativePosterUrl"));
    const auto otherPage = controller.getFolder("other", "Series", 1, 0, true);
    assert(!contains(otherPage.body, "representativePosterUrl"));

    // Public detail artwork and the binary route share the same current
    // recording-scoped asset ID and the already validated image reader.
    VdrRecording artworkRecording = makeRecording("movie-public", "Movie",
        "/Movies/Movie/2026-07-02.20.15.1-0.rec");
    artworkRecording.backendId = "default";
    const auto persisted = controller.findNativeMetadata(
        artworkRecording.backendId, artworkRecording.backendNativeId);
    auto trailerMetadata = persisted;
    trailerMetadata.metadata.providerId = 13;
    assert(controller.metadataTrailerIdentity(artworkRecording, {}, trailerMetadata) ==
        std::make_pair(std::string("movie"), std::string("13")));
    auto manualTrailer = rootManualAssignment();
    manualTrailer.backendId = artworkRecording.backendId;
    manualTrailer.providerId = "tmdb";
    manualTrailer.externalId = "99";
    manualTrailer.externalNamespace = "movie";
    assert(controller.metadataTrailerIdentity(artworkRecording, manualTrailer, trailerMetadata) ==
        std::make_pair(std::string("movie"), std::string("99")));
    manualTrailer.externalNamespace = "tv-episode";
    assert(controller.metadataTrailerIdentity(artworkRecording, manualTrailer, trailerMetadata).first.empty());
    auto withPortrait = persisted;
    withPortrait.metadata.people.front().image = withPortrait.metadata.images.front();
    const auto assets = controller.publicMetadataArtwork(artworkRecording, {}, withPortrait);
    assert(assets.size() == 3);
    assert(assets[0].kind == "preferred" && assets[1].kind == "person" &&
        assets[2].kind == "gallery");
    assert(assets[1].artworkId != assets[2].artworkId);
    const auto& preferred = withPortrait.metadata.preferredArtwork;
    assert(controller.publicSearchPreferredArtworkId(artworkRecording,
        preferred.path, preferred.width, preferred.height, false, 0) ==
        assets[0].artworkId);
    assert(controller.publicSearchPreferredArtworkId(artworkRecording,
        "", 0, 0, true, 7).empty());
    const auto portraitBytes = controller.loadPublicMetadataArtwork(
        artworkRecording, {}, withPortrait, assets[1].artworkId);
    assert(portraitBytes && portraitBytes->statusCode == 200 &&
        portraitBytes->body == "PORTRAIT");
    const auto galleryBytes = controller.loadPublicMetadataArtwork(
        artworkRecording, {}, withPortrait, assets[2].artworkId);
    assert(galleryBytes && galleryBytes->statusCode == 200 &&
        galleryBytes->body == "PORTRAIT");
    withPortrait.metadata.people.front().image.path = "/tmp/replaced-portrait.jpg";
    assert(!controller.loadPublicMetadataArtwork(
        artworkRecording, {}, withPortrait, assets[1].artworkId));
    auto manualPortrait = manualTrailer;
    manualPortrait.found = true;
    manualPortrait.relationshipLocked = true;
    manualPortrait.revision = 7;
    manualPortrait.posterReference.clear();
    manualPortrait.people.resize(2);
    manualPortrait.people[0].profilePath.clear();
    manualPortrait.people[1].profilePath =
        "/var/cache/vdr-suite/recording-metadata/posters/person.jpg";
    const auto manualImages = controller.publicMetadataArtwork(
        artworkRecording, manualPortrait, {});
    assert(manualImages.size() == 1U && manualImages[0].index == 1);
    assert(controller.publicManualPortraitArtworkId(artworkRecording, 1, 7,
        manualPortrait.people[1].profilePath) == manualImages[0].artworkId);
    assert(controller.publicManualPortraitArtworkId(artworkRecording, 0, 7,
        manualPortrait.people[1].profilePath) != manualImages[0].artworkId);
    assert(controller.publicManualPortraitArtworkId(artworkRecording, 1, 8,
        manualPortrait.people[1].profilePath) != manualImages[0].artworkId);
    manualPortrait.posterReference =
        "/var/cache/vdr-suite/recording-metadata/posters/poster.jpg";
    const auto manualPosterImages = controller.publicMetadataArtwork(
        artworkRecording, manualPortrait, {});
    assert(manualPosterImages.size() == 2U && manualPosterImages[0].kind == "preferred");
    assert(controller.publicSearchPreferredArtworkId(artworkRecording,
        manualPortrait.posterReference, 0, 0, true, 7) ==
        manualPosterImages[0].artworkId);
    assert(controller.publicSearchPreferredArtworkId(artworkRecording,
        manualPortrait.posterReference, 0, 0, true, 8) !=
        manualPosterImages[0].artworkId);
    auto foreign = artworkRecording;
    foreign.backendId = "other";
    assert(controller.publicMetadataArtwork(foreign, {}, withPortrait).empty());
    assert(!controller.loadPublicMetadataArtwork(foreign, {}, withPortrait, assets[2].artworkId));

    std::remove("/tmp/vdr-suite-movie-landscape.jpg");
    std::remove("/tmp/vdr-suite-movie-poster.jpg");

    std::cout
        << "test_vdr_recording_folder_controller passed"
        << std::endl;

    return 0;
}
