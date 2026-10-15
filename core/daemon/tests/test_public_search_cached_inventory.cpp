#include "PublicSearchRecordingInventory.h"
#include "PublicSearchProjection.h"
#include "PublicRecordingIdentityRepository.h"
#include "Database.h"

#include <cassert>
#include <cstdio>
#include <optional>
#include <string>
#include <vector>

namespace
{
VdrRecording makeRecording(const std::string& path, const std::string& title,
                           const std::string& id)
{
    VdrRecording value;
    value.backendId = "default";
    value.backendNativeId = path;
    value.path = path;
    value.id = id;
    value.title = title;
    value.startTime = "1792088100";
    value.durationSeconds = 3600;
    return value;
}

GlobalSearchRecording webItem(const VdrRecording& source)
{
    GlobalSearchRecording item;
    item.backendId = source.backendId;
    item.backendNativeId = source.backendNativeId;
    item.title = source.title;
    item.startTime = source.startTime;
    item.path = "/private/must-not-leak";
    return item;
}
}

int main()
{
    Database database;
    assert(database.open(":memory:"));
    VdrRecordingCacheRepository cache(database);
    assert(cache.ensureSchema());

    // Production's fast startup VDR snapshot has zero Recording entries.
    const std::vector<VdrRecording> startupSnapshot;
    assert(startupSnapshot.empty());
    assert(!PublicSearchRecordingInventory::readReady(cache, "default"));
    assert(cache.markRefreshStarted("default"));
    assert(!PublicSearchRecordingInventory::readReady(cache, "default"));

    // Current Web/Genre inventory contains the original, its VDR cut and a
    // third Recording. Native VDR identity differs from numeric Recording id.
    const VdrRecording original = makeRecording(
        "/srv/vdr/video/Filme/Der_Hobbit/2026-10-15.20.15.00-0.rec",
        "Filme~Der_Hobbit", "41");
    const VdrRecording cut = makeRecording(
        "/srv/vdr/video/Filme/%Der_Hobbit/2026-10-15.20.15.00-0.rec",
        "Filme~%Der_Hobbit", "42");
    const VdrRecording third = makeRecording(
        "/srv/vdr/video/Filme/Hobbit_Spezial/2026-10-15.22.15.00-0.rec",
        "Filme~Hobbit_Spezial", "43");
    assert(cache.replaceRecordingsForBackend("default", {original, cut, third}));
    assert(cache.markRefreshFinished("default", 3));

    const auto inventory = PublicSearchRecordingInventory::readReady(
        cache, "default");
    assert(inventory.has_value() && inventory->size() == 3U);
    assert(!PublicSearchRecordingInventory::readReady(cache, "other"));
    assert(!PublicSearchRecordingInventory::readReady(cache, "*"));

    PublicRecordingIdentityRepository identities(database);
    assert(identities.ensureSchema());
    const PublicSearchProjection::RecordingIdLookup lookup =
        [&identities](const std::string& backend, const std::string& native)
            -> std::optional<std::string>
        {
            return identities.resolveOrCreate(backend, native);
        };

    GlobalSearchResult web;
    web.backendId = "default";
    web.recordingTotal = 3;
    for (const auto& item : {original, cut, third})
        web.recordings.push_back(webItem(item));

    PublicSearchRequest request;
    request.backendId = "default";
    request.limit = 30;

    // The adapter must not re-search, re-rank or depend on query casing.
    for (const std::string& query : {"Hobbit", "hobbit", "HOBBIT"})
    {
        request.query = query;
        web.query = query;
        const auto page = PublicSearchProjection::project(
            request, web, *inventory, lookup);
        assert(page.status == PublicSearchStatus::ok);
        assert(page.items.size() == 3U && page.totalCount == 3U);
        assert(!page.hasMore);
        assert(page.items[0].title == "Der Hobbit");
        assert(page.items[1].title == "Der Hobbit");
        assert(page.items[2].title == "Hobbit Spezial");
        assert(!page.items[0].edited && page.items[1].edited);
        assert(!page.items[2].edited);
        assert(page.items[0].recordingId != page.items[1].recordingId);
        for (const auto& item : page.items)
        {
            assert(PublicSearchProjection::validRecordingId(item.recordingId));
            assert(item.recordingId.find("/private") == std::string::npos);
        }
    }

    // SQL search may have returned a page before a verified deletion:
    // the current inventory no longer contains that native identity.
    assert(cache.removeByBackendNativeId("default", cut.backendNativeId));
    const auto afterDeletion =
        PublicSearchRecordingInventory::readReady(cache, "default");
    assert(afterDeletion.has_value() && afterDeletion->size() == 2U);
    const auto deleted = PublicSearchProjection::project(
        request, web, *afterDeletion, lookup);
    assert(deleted.status == PublicSearchStatus::ok);
    assert(deleted.items.size() == 2U && deleted.totalCount == 2U);
    assert(deleted.items[0].title == "Der Hobbit");
    assert(deleted.items[1].title == "Hobbit Spezial");
    for (const auto& item : deleted.items)
        assert(item.recordingId != lookup("default", cut.backendNativeId).value());

    // An indexed result from another backend can never escape default.
    web.recordings.front().backendId = "other";
    const auto scoped = PublicSearchProjection::project(
        request, web, *afterDeletion, lookup);
    assert(scoped.status == PublicSearchStatus::ok);
    assert(scoped.items.size() == 1U);
    web.recordings.front().backendId = "default";

    // Failed inventory refreshes fail closed instead of pretending an empty
    // complete search is authoritative.
    assert(cache.markRefreshFailed("default", "fixture"));
    assert(!PublicSearchRecordingInventory::readReady(cache, "default"));

    // EPG projection is deliberately independent of the Recording cache.
    GlobalSearchResult epg;
    epg.backendId = "default";
    epg.epgTotal = 2;
    for (int i = 0; i < 2; ++i)
    {
        GlobalSearchEpgEvent event;
        event.backendId = "default";
        event.channelId = "channel";
        event.eventId = std::to_string(i + 1);
        event.title = "Young Sheldon";
        event.subtitle = "Hobbits und die Wissenschaftspause";
        event.startTime = "1792000000";
        event.endTime = "1792001800";
        epg.epg.push_back(event);
    }
    request.epg = true;
    const auto epgPage = PublicSearchProjection::project(
        request, epg, {}, lookup);
    assert(epgPage.status == PublicSearchStatus::ok);
    assert(epgPage.items.size() == 2U && epgPage.totalCount == 2U);
    std::puts("Public-v1 Recording cached-inventory search parity PASS");
}
