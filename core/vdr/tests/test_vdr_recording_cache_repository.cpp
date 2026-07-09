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
    long long sizeMb)
{
    VdrRecording recording;

    recording.id = id;
    recording.backendNativeId = nativeId;
    recording.title = title;
    recording.path = path;
    recording.startTime = startTime;
    recording.durationSeconds = durationSeconds;
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
            8192)
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
    assert(cached.at(0).sizeMb == 8192);

    assert(cached.at(1).title == "Zeta");
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
                4100)
        }));

    const std::vector<VdrRecording> cached =
        repository.findAllForBackend("home-vdr");

    assert(repository.countForBackend("home-vdr") == 1);
    assert(cached.size() == 1);
    assert(cached.at(0).id == "2");
    assert(cached.at(0).title == "Zeta Updated");
    assert(cached.at(0).durationSeconds == 3660);
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
    test_recording_cache_repository_upserts_and_reads_recordings();
    test_recording_cache_repository_replace_removes_stale_recordings();
    test_recording_cache_repository_normalizes_empty_backend();

    std::cout
        << "test_vdr_recording_cache_repository passed"
        << std::endl;

    return 0;
}
