#include "Database.h"
#include "MockVdrAdapter.h"
#include "VdrRecordingCacheRepository.h"
#include "VdrRecordingQueryService.h"
#include "VdrService.h"

#include <cassert>
#include <cstdio>
#include <iostream>
#include <string>
#include <vector>

static VdrRecording makeRecording(
    const std::string& id,
    const std::string& title,
    const std::string& path)
{
    VdrRecording recording;

    recording.id = id;
    recording.backendNativeId = "/srv/vdr/video" + path;
    recording.title = title;
    recording.path = path;
    recording.startTime = "1782928800";
    recording.durationSeconds = 3600;
    recording.sizeMb = 1024;

    return recording;
}

static void test_query_service_uses_cache_when_cache_has_data()
{
    std::remove("/tmp/test_vdr_recording_query_service_cache_has_data.db");

    Database database;
    assert(database.open("/tmp/test_vdr_recording_query_service_cache_has_data.db"));

    VdrRecordingCacheRepository cacheRepository(database);

    assert(cacheRepository.replaceRecordingsForBackend(
        "default",
        {
            makeRecording(
                "cached-1",
                "Cached Recording",
                "/Cached/Recording/2026-07-01.20.15.1-0.rec")
        }));

    assert(cacheRepository.markRefreshFinished("default", 1));

    MockVdrAdapter adapter;
    VdrService vdrService(adapter);

    VdrRecordingQueryService queryService(
        vdrService,
        &cacheRepository,
        "default");

    const VdrRecordingQueryResult result =
        queryService.queryRecordings(
            VdrRecordingQuery::all());

    assert(result.totalCount() == 1);
    assert(result.returnedCount() == 1);
    assert(result.recordings().at(0).id == "cached-1");
    assert(result.recordings().at(0).backendId == "default");
    assert(result.recordings().at(0).title == "Cached Recording");
}

static void test_query_service_falls_back_to_live_and_populates_empty_cache()
{
    std::remove("/tmp/test_vdr_recording_query_service_cache_empty.db");

    Database database;
    assert(database.open("/tmp/test_vdr_recording_query_service_cache_empty.db"));

    VdrRecordingCacheRepository cacheRepository(database);

    assert(cacheRepository.ensureSchema());
    assert(cacheRepository.countForBackend("default") == 0);

    MockVdrAdapter adapter;
    VdrService vdrService(adapter);

    VdrRecordingQueryService queryService(
        vdrService,
        &cacheRepository,
        "default");

    const VdrRecordingQueryResult result =
        queryService.queryRecordings(
            VdrRecordingQuery::all());

    assert(result.totalCount() == 2);
    assert(result.returnedCount() == 2);
    assert(cacheRepository.countForBackend("default") == 2);

    const VdrRecordingCacheStatus status =
        cacheRepository.statusForBackend("default");

    assert(status.state == "ready");
    assert(status.totalCount == 2);
}

static void test_query_service_keeps_legacy_live_mode_without_cache()
{
    MockVdrAdapter adapter;
    VdrService vdrService(adapter);

    VdrRecordingQueryService queryService(vdrService);

    const VdrRecordingQueryResult result =
        queryService.queryRecordings(
            VdrRecordingQuery::all());

    assert(result.totalCount() == 2);
    assert(result.returnedCount() == 2);
}

int main()
{
    test_query_service_uses_cache_when_cache_has_data();
    test_query_service_falls_back_to_live_and_populates_empty_cache();
    test_query_service_keeps_legacy_live_mode_without_cache();

    std::cout
        << "test_vdr_recording_query_service_cache passed"
        << std::endl;

    return 0;
}
