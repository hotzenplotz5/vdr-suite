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

static void test_recording_cache_repository_removes_one_native_recording()
{
    std::remove("/tmp/test_vdr_recording_cache_repository_remove.db");

    Database database;
    assert(database.open("/tmp/test_vdr_recording_cache_repository_remove.db"));

    VdrRecordingCacheRepository repository(database);

    const VdrRecording keep = makeRecording(
        "1",
        "/srv/vdr/video/Movies/Keep/2026-07-01.18.00.1-0.rec",
        "Keep",
        "/Movies/Keep/2026-07-01.18.00.1-0.rec",
        "1782928800",
        7200,
        8192);
    const VdrRecording remove = makeRecording(
        "2",
        "/srv/vdr/video/Movies/Remove/2026-07-01.20.15.1-0.rec",
        "Remove",
        "/Movies/Remove/2026-07-01.20.15.1-0.rec",
        "1782936900",
        3600,
        4096);

    assert(repository.replaceRecordingsForBackend(
        "home-vdr",
        {keep, remove}));
    assert(repository.warmBrowseSnapshotForBackend("home-vdr"));

    assert(repository.removeByBackendNativeId(
        "home-vdr",
        remove.backendNativeId));
    assert(repository.countForBackend("home-vdr") == 1);

    VdrRecording found;
    assert(!repository.findByBackendNativeId(
        "home-vdr",
        remove.backendNativeId,
        found));
    assert(repository.findByBackendNativeId(
        "home-vdr",
        keep.backendNativeId,
        found));

    const VdrRecordingFolderPage root =
        repository.folderPageForBackend("home-vdr", "", 50, 0);
    assert(root.totalCount == 1);
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


static void test_public_folder_browse_reuses_home_hierarchy()
{
    Database database;
    assert(database.open(":memory:"));
    VdrRecordingCacheRepository repository(database);
    assert(repository.replaceRecordingsForBackend("home-vdr", {
        makeRecording("a", "/srv/vdr/video/Series/Alpha/1.rec",
                      "Alpha", "/Series/Alpha/1.rec", "123", 60, 10),
        makeRecording("b", "/srv/vdr/video/Series/Beta/2.rec",
                      "Beta", "/Series/Beta/2.rec", "124", 60, 10),
        makeRecording("c", "/srv/vdr/video/Movies/Zeta/3.rec",
                      "Zeta", "/Movies/Zeta/3.rec", "125", 60, 10)
    }));

    VdrRecordingFolderPage root;
    assert(repository.folderPageForBackendByPublicId(
        "home-vdr", "", 30, 0, root));
    assert(root.folderCount == 2);
    assert(root.folders.size() == 2U);
    std::string seriesPath;
    for (const auto& folder : root.folders)
        if (folder.name == "Series") seriesPath = folder.path;
    assert(!seriesPath.empty());

    const auto id = VdrRecordingCacheRepository::publicFolderId(
        "home-vdr", seriesPath);
    assert(id.size() == 37U && id.rfind("fld1_", 0U) == 0U);
    assert(id.find("Series") == std::string::npos);
    assert(id == VdrRecordingCacheRepository::publicFolderId(
        "home-vdr", seriesPath));

    VdrRecordingFolderPage children;
    assert(repository.folderPageForBackendByPublicId(
        "home-vdr", id, 30, 0, children));
    assert(children.folderCount == 2);
    assert(children.folders.size() == 2U);
    // VDR title folders with exactly one .rec are recording leaves.
    for (const auto& child : children.folders)
    {
        assert(child.singleRecordingLeaf);
        assert(!child.singleRecording.backendNativeId.empty());
        assert(child.singleRecording.title == child.name);
    }
    assert(!repository.folderPageForBackendByPublicId(
        "other-backend", id, 30, 0, children));
    assert(!repository.folderPageForBackendByPublicId(
        "home-vdr", "/Series", 30, 0, children));
}

static void test_public_root_leaf_preserves_backend_scope_after_replace()
{
    // Production VDR inventory entries may have an empty backendId.
    // The in-memory browse snapshot must be scoped like the SQLite rows.
    Database database;
    assert(database.open(":memory:"));
    VdrRecordingCacheRepository repository(database);
    const std::string nativeId =
        "/srv/vdr/video/Die_Fotografin/2026-10-09.20.15.1-0.rec";
    VdrRecording raw = makeRecording(
        "raw-1", nativeId, "Die Fotografin",
        "/Die_Fotografin/2026-10-09.20.15.1-0.rec",
        "1791576900", 5700, 2800);
    assert(raw.backendId.empty());
    assert(repository.replaceRecordingsForBackend("default", {raw}));

    VdrRecordingFolderPage root;
    assert(repository.folderPageForBackendByPublicId(
        "default", "", 12, 0, root));
    assert(root.folderCount == 1);
    assert(root.folders.size() == 1U);
    const auto& leaf = root.folders.front();
    assert(leaf.singleRecordingLeaf);
    assert(leaf.name == "Die_Fotografin");
    assert(leaf.singleRecording.backendId == "default");
    assert(leaf.singleRecording.backendNativeId == nativeId);
    assert(leaf.singleRecording.title == "Die Fotografin");
    assert(leaf.singleRecording.durationSeconds == 5700);
    assert(!repository.folderPageForBackendByPublicId(
        "other-backend", VdrRecordingCacheRepository::publicFolderId(
            "default", leaf.path), 12, 0, root));
}

static void test_genre_members_resolve_from_cache_without_startup_recording_snapshot()
{
    // The daemon's startup VDR snapshot intentionally has no recordings.
    // Genre member resolution must use the independently warmed cache.
    Database database;
    assert(database.open(":memory:"));
    VdrRecordingCacheRepository repository(database);
    const std::string nativeId =
        "/srv/vdr/video/Movies/48_Hrs/2026-10-09.20.15.1-0.rec";
    assert(repository.replaceRecordingsForBackend("default", {
        makeRecording("cached-1", nativeId, "48 Hrs",
                      "/Movies/48_Hrs/2026-10-09.20.15.1-0.rec",
                      "1791576900", 5700, 2800)
    }));
    VdrRecording found;
    assert(repository.findByBackendNativeId("default", nativeId, found));
    assert(found.backendId == "default");
    assert(found.backendNativeId == nativeId);
    assert(found.title == "48 Hrs");
    assert(found.durationSeconds == 5700);
    assert(!repository.findByBackendNativeId("other-backend", nativeId, found));
}

int main()
{
    test_public_root_leaf_preserves_backend_scope_after_replace();
    test_genre_members_resolve_from_cache_without_startup_recording_snapshot();
    test_recording_cache_repository_schema();
    test_recording_cache_repository_migrates_duration_authority();
    test_recording_cache_repository_upserts_and_reads_recordings();
    test_recording_cache_repository_replace_removes_stale_recordings();
    test_recording_cache_repository_removes_one_native_recording();
    test_recording_cache_repository_normalizes_empty_backend();
    test_public_folder_browse_reuses_home_hierarchy();

    std::cout
        << "test_vdr_recording_cache_repository passed"
        << std::endl;

    return 0;
}
