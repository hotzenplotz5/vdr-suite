#include "Database.h"
#include "VdrRecordingCacheRepository.h"

#include <cassert>
#include <cstdio>
#include <iostream>
#include <string>
#include <vector>

namespace
{

const char* databasePath =
    "/tmp/test_vdr_recording_cache_metadata_persistence.db";

VdrRecording makeMetadataRecording()
{
    VdrRecording recording;
    recording.id = "42";
    recording.backendId = "default";
    recording.backendNativeId =
        "/srv/vdr/video/Movies/Zero/2026-07-16.20.15.1-0.rec";
    recording.title = "Zero";
    recording.path =
        "/Movies/Zero/2026-07-16.20.15.1-0.rec";
    recording.startTime = "1784232900";
    recording.durationSeconds = 5400;
    recording.sizeMb = 4096;

    recording.metadata.native.eventTitle = "Zero";
    recording.metadata.native.shortText =
        "Fernsehfilm Deutschland 2021";
    recording.metadata.native.description =
        "Berlin in naher Zukunft";

    recording.metadata.provider.source =
        VdrRecordingMetadataSource::RestfulApiScraperBridge;
    recording.metadata.provider.contentKind =
        VdrRecordingContentKind::Movie;
    recording.metadata.provider.movieId = "785533";
    recording.metadata.provider.title = "Zero";
    recording.metadata.provider.overview =
        "Ein Film mit persistenten Metadaten";
    recording.metadata.provider.genreText = "Drama";
    recording.metadata.provider.releaseDate = "2021-11-03";
    recording.metadata.provider.runtimeMinutes = 90;
    recording.metadata.provider.rating = 7.25;

    VdrRecordingArtworkRef poster;
    poster.kind = VdrRecordingArtworkKind::Poster;
    poster.source =
        VdrRecordingMetadataSource::RestfulApiScraperBridge;
    poster.reference = "movies/785533/poster.jpg";
    poster.width = 680;
    poster.height = 1000;
    poster.temporary = true;
    recording.metadata.artwork.push_back(poster);

    return recording;
}

void createLegacyCache(Database& database)
{
    assert(database.execute(
        "CREATE TABLE vdr_recording_cache ("
        "backend_id TEXT NOT NULL,"
        "cache_key TEXT NOT NULL,"
        "recording_id TEXT NOT NULL DEFAULT '',"
        "backend_native_id TEXT NOT NULL DEFAULT '',"
        "title TEXT NOT NULL DEFAULT '',"
        "path TEXT NOT NULL DEFAULT '',"
        "start_time TEXT NOT NULL DEFAULT '',"
        "duration_seconds INTEGER NOT NULL DEFAULT 0,"
        "size_mb INTEGER NOT NULL DEFAULT 0,"
        "created_at TEXT DEFAULT CURRENT_TIMESTAMP,"
        "updated_at TEXT DEFAULT CURRENT_TIMESTAMP,"
        "last_seen_at TEXT DEFAULT CURRENT_TIMESTAMP,"
        "PRIMARY KEY (backend_id, cache_key)"
        ");"
        "INSERT INTO vdr_recording_cache ("
        "backend_id, cache_key, recording_id, title, path"
        ") VALUES ("
        "'default', 'legacy.rec', '1', 'Legacy', '/legacy.rec'"
        ");"));
}

}

int main()
{
    std::remove(databasePath);

    {
        Database database;
        assert(database.open(databasePath));
        createLegacyCache(database);

        VdrRecordingCacheRepository repository(database);
        assert(repository.ensureSchema());

        const std::vector<VdrRecording> migrated =
            repository.findAllForBackend("default");
        assert(migrated.size() == 1);
        assert(migrated.front().title == "Legacy");
        assert(!migrated.front().metadata.hasProviderData());
        assert(!migrated.front().metadata.hasArtwork());

        const VdrRecording recording = makeMetadataRecording();
        assert(repository.replaceRecordingsForBackend(
            "default",
            {recording}));
    }

    {
        Database database;
        assert(database.open(databasePath));

        VdrRecordingCacheRepository repository(database);
        assert(repository.ensureSchema());

        const std::vector<VdrRecording> recordings =
            repository.findAllForBackend("default");

        assert(recordings.size() == 1);

        const VdrRecording& recording = recordings.front();
        assert(recording.id == "42");
        assert(recording.backendId == "default");
        assert(recording.title == "Zero");
        assert(recording.metadata.native.shortText ==
               "Fernsehfilm Deutschland 2021");
        assert(recording.metadata.provider.hasData());
        assert(recording.metadata.provider.contentKind ==
               VdrRecordingContentKind::Movie);
        assert(recording.metadata.provider.movieId == "785533");
        assert(recording.metadata.provider.overview ==
               "Ein Film mit persistenten Metadaten");
        assert(recording.metadata.provider.runtimeMinutes == 90);
        assert(recording.metadata.provider.rating == 7.25);
        assert(recording.metadata.artwork.size() == 1);
        assert(recording.metadata.artwork.front().kind ==
               VdrRecordingArtworkKind::Poster);
        assert(recording.metadata.artwork.front().reference ==
               "movies/785533/poster.jpg");
        assert(recording.metadata.artwork.front().width == 680);
        assert(recording.metadata.artwork.front().height == 1000);
    }

    std::cout
        << "test_vdr_recording_cache_metadata_persistence passed"
        << std::endl;

    return 0;
}
