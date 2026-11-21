#include "PublicSearchProjection.h"

#include <cassert>
#include <cstdio>
#include <optional>
#include <string>
#include <vector>

namespace
{
constexpr const char* FirstId = "rec_11111111111111111111111111111111";
constexpr const char* SecondId = "rec_22222222222222222222222222222222";

GlobalSearchRecording recording(
    const std::string& native, const std::string& effectiveTitle)
{
    GlobalSearchRecording item;
    item.backendId = "default";
    item.backendNativeId = native;
    item.title = effectiveTitle;
    item.path = "/private/must-not-leak";
    item.startTime = "1760000000";
    return item;
}

VdrRecording live(const std::string& native, const std::string& vdrTitle)
{
    VdrRecording item;
    item.backendId = "default";
    item.backendNativeId = native;
    item.title = vdrTitle;
    return item;
}
}

int main()
{
    // The Web and Public-v1 use the same canonical GlobalSearchResult.
    // Reproduce the three verified Hobbit results, including a manual title.
    GlobalSearchResult web;
    web.backendId = "default";
    web.query = "Hobbit";
    web.recordingTotal = 3;
    web.recordings.push_back(recording("a", "Der Hobbit - Eine unerwartete Reise"));
    web.recordings.push_back(recording("b", "Der Hobbit - Smaugs Einöde"));
    web.recordings.push_back(recording("c", "Der Hobbit - Die Schlacht der fünf Heere"));

    std::vector<VdrRecording> snapshot = {
        live("a", "Fantasy/Der Hobbit - Eine unerwartete Reise"),
        live("b", "Fantasy/Der Hobbit - Smaugs Einöde"),
        live("c", "Fantasy/Der Hobbit - Die Schlacht der fünf Heere")
    };
    const PublicSearchProjection::RecordingIdLookup lookup =
        [](const std::string& backend, const std::string& native)
            -> std::optional<std::string>
        {
            if (backend != "default") return std::nullopt;
            return native == "a" ? std::optional<std::string>(FirstId) :
                native == "b" ? std::optional<std::string>(SecondId) :
                native == "c" ? std::optional<std::string>(
                    "rec_33333333333333333333333333333333") : std::nullopt;
        };
    PublicSearchRequest request;
    request.backendId = "default";
    request.query = "Hobbit";
    request.limit = 30;

    const auto page = PublicSearchProjection::project(
        request, web, snapshot, lookup);
    assert(page.status == PublicSearchStatus::ok);
    assert(page.totalCount == 3U && page.items.size() == 3U);
    assert(!page.hasMore);
    for (std::size_t index = 0; index < 3U; ++index)
    {
        assert(page.items[index].title == web.recordings[index].title);
        assert(page.items[index].recordingId.rfind("rec_", 0U) == 0U);
        assert(page.items[index].channelId.empty());
        assert(page.items[index].eventId.empty());
        assert(page.items[index].recordingId.find("private") == std::string::npos);
    }
    assert(page.items.front().title != snapshot.front().title);

    // An indexed, already deleted recording is not leaked and must not
    // cause HTTP 503 for the still-current Hobbit results.
    web.recordingTotal = 4;
    web.recordings.insert(web.recordings.begin(),
        recording("stale", "Der Hobbit - gelöschte Aufnahme"));
    const auto withStale = PublicSearchProjection::project(
        request, web, snapshot, lookup);
    assert(withStale.status == PublicSearchStatus::ok);
    assert(withStale.items.size() == 3U);
    assert(withStale.totalCount == 3U);
    assert(withStale.items.front().title ==
        "Der Hobbit - Eine unerwartete Reise");

    // An unavailable canonical search still fails closed.
    web.status = "unavailable";
    const auto failed = PublicSearchProjection::project(
        request, web, snapshot, lookup);
    assert(failed.status == PublicSearchStatus::unavailable);

    // EPG subtitles and titles are projected verbatim from the Web search.
    GlobalSearchResult epg;
    epg.backendId = "default";
    epg.epgTotal = 2;
    for (int i = 0; i < 2; ++i)
    {
        GlobalSearchEpgEvent event;
        event.backendId = "default";
        event.channelId = "channel";
        event.eventId = std::to_string(i+1);
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
    assert(epgPage.items.size() == 2U);
    assert(epgPage.totalCount == 2U);
    assert(epgPage.items.front().subtitle ==
        "Hobbits und die Wissenschaftspause");
    assert(epgPage.items.front().recordingId.empty());

    // Never allow an item from another backend through the public adapter.
    web.status = "ready";
    web.recordings.clear();
    web.recordings.push_back(recording("a", "Der Hobbit"));
    web.recordings.front().backendId = "other";
    web.recordingTotal = 1;
    request.epg = false;
    const auto scoped = PublicSearchProjection::project(
        request, web, snapshot, lookup);
    assert(scoped.status == PublicSearchStatus::ok);
    assert(scoped.items.empty());

    std::puts("Public-v1 / Web global-search projection parity PASS");
    return 0;
}
