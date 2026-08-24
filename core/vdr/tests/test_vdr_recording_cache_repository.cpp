#include "Database.h"
#include "VdrRecordingCacheRepository.h"

#include <cassert>
#include <cstdio>
#include <iostream>
#include <string>
#include <vector>

static VdrRecording makeRecording(
    const std::string& id,
    const std::string& nativeId,
    const std::string& title,
    const std::string& path,
    const std::string& startTime,
    int durationSeconds,
    long long sizeMb,
    bool recordingDurationKnown = false)
{
    VdrRecording recording;

    recording.id = id;
    recording.backendNativeId = nativeId;
    recording.title = title;
    recording.path = path;
    recording.startTime = startTime;
    recording.durationSeconds = durationSeconds;
    recording.recordingDurationKnown = recordingDurationKnown;
    recording.sizeMb = sizeMb;

    return recording;
}

static void test_recording_cache_repository_schema()
{
    std::remove("/tmp/test_vdr_recording_cache_repository_schema.db");

    Database database;
    assert(database.open("/tmp/test_vdr_recording_cache_repository_schema.db"));

    VdrRecordingCacheRepository repository(database);

    assert(repository.ensureSchema());
    assert(database.tableExists("vdr_recording_cache"));
}

static void test_recording_cache_repository_migrates_duration_authority()
{
    const char* path =
        "/tmp/test_vdr_recording_cache_repository_duration_migration.db";
    std::remove(path);

    Database database;
    assert(database.open(path));
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
        "metadata_payload TEXT NOT NULL DEFAULT '',"
        "created_at TEXT DEFAULT CURRENT_TIMESTAMP,"
        "updated_at TEXT DEFAULT CURRENT_TIMESTAMP,"
        "last_seen_at TEXT DEFAULT CURRENT_TIMESTAMP,"
        "PRIMARY KEY (backend_id, cache_key)"
        ");"));

    VdrRecordingCacheRepository repository(database);
    assert(repository.ensureSchema());

    const VdrRecording recording = makeRecording(
        "duration-known",
        "/srv/vdr/video/Movies/Known/2026-08-23.20.15.1-0.rec",
        "Known duration",
        "/Movies/Known/2026-08-23.20.15.1-0.rec",
        "1787516100",
        77,
        512,
        true);

    assert(repository.upsertRecordingsForBackend(
        "home-vdr",
        {recording}));
    const std::vector<VdrRecording> cached =
        repository.findAllForBackend("home-vdr");
    assert(cached.size() == 1);
    assert(cached.front().durationSeconds == 77);
    assert(cached.front().recordingDurationKnown);
}

static void test_recording_cache_repository_upserts_and_reads_recordings()
{
    std::remove("/tmp/test_vdr_recording_cache_repository_upsert.db");

    Database database;
    assert(database.open("/tmp/test_vdr_recording_cache_repository_upsert.db"));

    VdrRecordingCacheRepository repository(database);

    const std::vector<VdrRecording> recordings = {
        makeRecording(
            "2",
            "/srv/vdr/video/Series/Zeta/2026-07-01.20.15.1-0.rec",
            "Zeta",
            "/Series/Zeta/2026-07-01.20.15.1-0.rec",
            "1782936900",
            3600,
            4096),
        makeRecording(
            "1",
            "/srv/vdr/video/Movies/Alpha/2026-07-01.18.00.1-0.rec",
            "Alpha",
            "/Movies/Alpha/2026-07-01.18.00.1-0.rec",
            "1782928800",
            7200,
            8192,
            true)
    };

    assert(repository.upsertRecordingsForBackend("home-vdr", recordings));
    assert(repository.countForBackend("home-vdr") == 2);

    const std::vector<VdrRecording> cached =
        repository.findAllForBackend("home-vdr");

    assert(cached.size() == 2);

    assert(cached.at(0).id == "1");
    assert(cached.at(0).backendId == "home-vdr");
    assert(cached.at(0).backendNativeId == "/srv/vdr/video/Movies/Alpha/2026-07-01.18.00.1-0.rec");
    assert(cached.at(0).title == "Alpha");
    assert(cached.at(0).path == "/Movies/Alpha/2026-07-01.18.00.1-0.rec");
    assert(cached.at(0).startTime == "1782928800");
    assert(cached.at(0).durationSeconds == 7200);
    assert(cached.at(0).recordingDurationKnown);
    assert(cached.at(0).sizeMb == 8192);

    assert(cached.at(1).title == "Zeta");
    assert(!cached.at(1).recordingDurationKnown);
}

static void test_recording_cache_repository_replace_removes_stale_recordings()
{
    std::remove("/tmp/test_vdr_recording_cache_repository_replace.db");

    Database database;
    assert(database.open("/tmp/test_vdr_recording_cache_repository_replace.db"));

    VdrRecordingCacheRepository repository(database);

    assert(repository.upsertRecordingsForBackend(
        "home-vdr",
        {
            makeRecording(
                "1",
                "/srv/vdr/video/Movies/Alpha/2026-07-01.18.00.1-0.rec",
                "Alpha",
                "/Movies/Alpha/2026-07-01.18.00.1-0.rec",
                "1782928800",
                7200,
                8192),
            makeRecording(
                "2",
                "/srv/vdr/video/Series/Zeta/2026-07-01.20.15.1-0.rec",
                "Zeta",
                "/Series/Zeta/2026-07-01.20.15.1-0.rec",
                "1782936900",
                3600,
                4096)
        }));

    assert(repository.countForBackend("home-vdr") == 2);

    assert(repository.replaceRecordingsForBackend(
        "home-vdr",
        {
            makeRecording(
                "2",
                "/srv/vdr/video/Series/Zeta/2026-07-01.20.15.1-0.rec",
                "Zeta Updated",
                "/Series/Zeta/2026-07-01.20.15.1-0.rec",
                "1782936900",
                3660,
                4100,
                true)
        }));

    const std::vector<VdrRecording> cached =
        repository.findAllForBackend("home-vdr");

    assert(repository.countForBackend("home-vdr") == 1);
    assert(cached.size() == 1);
    assert(cached.at(0).id == "2");
    assert(cached.at(0).title == "Zeta Updated");
    assert(cached.at(0).durationSeconds == 3660);
    assert(cached.at(0).recordingDurationKnown);
    assert(cached.at(0).sizeMb == 4100);
}

static void test_recording_cache_repository_normalizes_empty_backend()
{
    std::remove("/tmp/test_vdr_recording_cache_repository_default.db");

    Database database;
    assert(database.open("/tmp/test_vdr_recording_cache_repository_default.db"));

    VdrRecordingCacheRepository repository(database);

    assert(repository.upsertRecordingsForBackend(
        "",
        {
            makeRecording(
                "1",
                "/srv/vdr/video/default/one.rec",
                "Default Recording",
                "/default/one.rec",
                "1782928800",
                60,
                100)
        }));

    assert(repository.countForBackend("default") == 1);

    const std::vector<VdrRecording> cached =
        repository.findAllForBackend("");

    assert(cached.size() == 1);
    assert(cached.at(0).backendId == "default");
    assert(cached.at(0).title == "Default Recording");
}

int main()
{
    test_recording_cache_repository_schema();
    test_recording_cache_repository_migrates_duration_authority();
    test_recording_cache_repository_upserts_and_reads_recordings();
    test_recording_cache_repository_replace_removes_stale_recordings();
    test_recording_cache_repository_normalizes_empty_backend();

    std::cout
        << "test_vdr_recording_cache_repository passed"
        << std::endl;

    return 0;
}
