#include "PublicApiRuntime.h"
#include "PublicSearchProjection.h"

#include <cassert>
#include <string>
#include <vector>

namespace
{
std::string nextCursor(const std::string& body)
{
    const std::string marker = "\"nextCursor\":\"";
    const auto start = body.find(marker);
    if (start == std::string::npos) return {};
    const auto value = start + marker.size();
    const auto end = body.find('"', value);
    return end == std::string::npos ? std::string() :
        body.substr(value, end - value);
}

PublicRecordingCollectionItem recording(const char* id)
{
    PublicRecordingCollectionItem value;
    value.recordingId = id;
    value.backendId = "backend-a";
    value.title = "Film";
    value.recordedAt = "2026-10-09T20:15:00Z";
    value.durationSeconds = 3600;
    value.durationKnown = true;
    value.description = "Kurzbeschreibung";
    value.sizeMb = 2048;
    return value;
}
}

int main()
{
    auto& runtime = PublicApiRuntime::instance();
    runtime.resetRecordingCollectionLookup();
    runtime.registerRecordingCollectionLookup(
        [](const PublicRecordingCollectionRequest& request)
        {
            PublicRecordingCollectionResult page;
            if (request.backendId == "backend-down")
                return page;
            page.status = PublicRecordingCollectionStatus::ok;
            for (const auto& item : {
                recording("rec_00000000000000000000000000000001"), recording("rec_00000000000000000000000000000002"), recording("rec_00000000000000000000000000000003")})
            {
                if (!request.afterRecordingId.empty() &&
                    item.recordingId <= request.afterRecordingId)
                    continue;
                if (page.recordings.size() >= request.limit)
                {
                    page.hasMore = true;
                    break;
                }
                page.recordings.push_back(item);
            }
            return page;
        });

    ApiResponse capability;
    assert(runtime.tryHandleGet(
        "/api/v1/capabilities", "actor:tv", "caps-r2", "",
        capability, "", "", {"backend-a"}));
    assert(capability.statusCode == 200);
    assert(capability.body.find("public-api.recordings-browse") !=
        std::string::npos);

    ApiResponse first;
    assert(runtime.tryHandleGet(
        "/api/v1/recordings?backendId=backend-a&limit=2",
        "actor:tv", "recording-first", "", first, "", "",
        {"backend-a"}));
    assert(first.statusCode == 200);
    assert(first.body.find("\"recordingId\":\"rec_00000000000000000000000000000001\"") != std::string::npos);
    assert(first.body.find("\"durationSeconds\":3600") != std::string::npos);
    assert(first.body.find("\"durationKnown\":true") != std::string::npos);
    assert(first.body.find("\"description\":\"Kurzbeschreibung\"") != std::string::npos);
    assert(first.body.find("\"sizeMb\":2048") != std::string::npos);
    assert(first.body.find("\"hasMore\":true") != std::string::npos);
    assert(first.body.find("backendNativeId") == std::string::npos);
    assert(first.body.find("recordingPath") == std::string::npos);
    assert(first.body.find("/private") == std::string::npos);
    assert(first.headers.at("Cache-Control") == "no-store");

    const auto cursor = nextCursor(first.body);
    assert(cursor.rfind("rc1_", 0U) == 0U);
    ApiResponse second;
    assert(runtime.tryHandleGet(
        "/api/v1/recordings?backendId=backend-a&limit=2&cursor=" + cursor,
        "actor:tv", "recording-second", "", second, "", "",
        {"backend-a"}));
    assert(second.statusCode == 200);
    assert(second.body.find("\"recordingId\":\"rec_00000000000000000000000000000003\"") != std::string::npos);
    assert(second.body.find("\"hasMore\":false") != std::string::npos);

    // Folder traversal is an additive view of the same recordings.view route.
    runtime.resetRecordingCollectionLookup();
    runtime.registerRecordingCollectionLookup(
        [](const PublicRecordingCollectionRequest& request)
        {
            PublicRecordingCollectionResult page;
            if (!request.browseFolders || request.backendId != "backend-a")
                return page;
            page.status = PublicRecordingCollectionStatus::ok;
            page.totalEntries = 3U;
            if (request.offset == 0U)
            {
                PublicRecordingFolderItem folder;
                folder.folderId = "fld1_0123456789abcdef0123456789abcdef";
                folder.name = "Serien";
                folder.recordingCount = 1;
                page.folders.push_back(folder);
            }
            else if (request.offset == 1U)
            {
                PublicRecordingFolderItem leaf;
                leaf.singleRecordingLeaf = true;
                leaf.singleRecording =
                    recording("rec_00000000000000000000000000000001");
                leaf.singleRecording.title = "48 Hrs";
                page.folders.push_back(leaf);
            }
            else if (request.offset == 2U)
                page.recordings.push_back(
                    recording("rec_00000000000000000000000000000002"));
            page.hasMore = request.offset < 2U;
            return page;
        });
    ApiResponse folders;
    assert(runtime.tryHandleGet(
        "/api/v1/recordings?backendId=backend-a&view=folders&limit=1",
        "actor:tv", "browse-root", "", folders, "", "", {"backend-a"}));
    assert(folders.statusCode == 200);
    assert(folders.body.find("\"kind\":\"folder\"") != std::string::npos);
    assert(folders.body.find("Serien") != std::string::npos);
    assert(folders.body.find("folderId") != std::string::npos);
    assert(folders.body.find("backendNativeId") == std::string::npos);
    ApiResponse children;
    assert(runtime.tryHandleGet(
        "/api/v1/recordings?backendId=backend-a&view=folders&limit=1&offset=1",
        "actor:tv", "browse-child", "", children, "", "", {"backend-a"}));
    assert(children.statusCode == 200);
    assert(children.body.find("\"kind\":\"recording\"") != std::string::npos);
    assert(children.body.find("\"title\":\"48 Hrs\"") != std::string::npos);
    assert(children.body.find("\"folderId\"") == std::string::npos);
    assert(children.body.find("\"hasMore\":true") != std::string::npos);
    ApiResponse direct;
    assert(runtime.tryHandleGet(
        "/api/v1/recordings?backendId=backend-a&view=folders&limit=1&offset=2",
        "actor:tv", "browse-direct", "", direct, "", "", {"backend-a"}));
    assert(direct.statusCode == 200);
    assert(direct.body.find("\"kind\":\"recording\"") != std::string::npos);
    assert(direct.body.find("\"hasMore\":false") != std::string::npos);
    ApiResponse invalidFolder;
    assert(runtime.tryHandleGet(
        "/api/v1/recordings?backendId=backend-a&view=folders&folderId=/srv/private",
        "actor:tv", "browse-invalid", "", invalidFolder, "", "", {"backend-a"}));
    assert(invalidFolder.statusCode == 400);
    ApiResponse unauthorizedBrowse;
    assert(runtime.tryHandleGet(
        "/api/v1/recordings?backendId=backend-a&view=folders",
        "", "browse-unauth", "", unauthorizedBrowse, "", "", {"backend-a"}));
    assert(unauthorizedBrowse.statusCode == 401);
    runtime.resetRecordingCollectionLookup();
    runtime.registerRecordingCollectionLookup(
        [](const PublicRecordingCollectionRequest& request)
        {
            PublicRecordingCollectionResult page;
            if (request.backendId == "backend-down")
                return page;
            page.status = PublicRecordingCollectionStatus::ok;
            for (const auto& item : {
                recording("rec_00000000000000000000000000000001"),
                recording("rec_00000000000000000000000000000002"),
                recording("rec_00000000000000000000000000000003")})
            {
                if (!request.afterRecordingId.empty() &&
                    item.recordingId <= request.afterRecordingId)
                    continue;
                if (page.recordings.size() >= request.limit)
                {
                    page.hasMore = true;
                    break;
                }
                page.recordings.push_back(item);
            }
            return page;
        });

    ApiResponse wrongScope;
    assert(runtime.tryHandleGet(
        "/api/v1/recordings?backendId=backend-b&cursor=" + cursor,
        "actor:tv", "recording-scope", "", wrongScope, "", "",
        {"backend-b"}));
    assert(wrongScope.statusCode == 409);

    for (const std::string& invalid : {
             "limit=0", "limit=101", "sort=startTime",
             "order=desc", "offset=5", "backendId=backend-b"})
    {
        ApiResponse response;
        assert(runtime.tryHandleGet(
            "/api/v1/recordings?backendId=backend-a&" + invalid,
            "actor:tv", "recording-invalid", "", response, "", "",
            {"backend-a"}));
        assert(response.statusCode == 400);
    }

    ApiResponse absent;
    assert(runtime.tryHandleGet(
        "/api/v1/recordings?backendId=backend-down",
        "actor:tv", "recording-down", "", absent, "", "",
        {"backend-down"}));
    assert(absent.statusCode == 503);

    ApiResponse unauthenticated;
    assert(runtime.tryHandleGet(
        "/api/v1/recordings?backendId=backend-a",
        "", "recording-auth", "", unauthenticated, "", "",
        {"backend-a"}));
    assert(unauthenticated.statusCode == 401);

    runtime.resetRecordingCollectionLookup();
    runtime.resetSearchLookup();
    runtime.registerSearchLookup([](const PublicSearchRequest& request) {
        PublicSearchResult result;
        result.status = PublicSearchStatus::ok;
        result.totalCount = 1U;
        if (request.offset == 0U) {
            PublicSearchItem item;
            item.backendId = request.backendId;
            item.title = "Der Hobbit";
            if (request.epg) {
                item.channelId = "S19.2E-1-100";
                item.eventId = "123";
                item.startTime = "1791565200";
                item.endTime = "1791572400";
            } else {
                item.recordingId = "rec_0123456789abcdef0123456789abcdef";
            }
            result.items.push_back(item);
        }
        return result;
    });
    ApiResponse searchRecording;
    assert(runtime.tryHandleGet(
        "/api/v1/search/recordings?backendId=backend-a&query=hobbit",
        "actor:tv", "search-rec", "", searchRecording, "", "", {"backend-a"}));
    assert(searchRecording.statusCode == 200);
    assert(searchRecording.body.find("\"kind\":\"recording\"") != std::string::npos);
    assert(searchRecording.body.find("rec_0123456789abcdef0123456789abcdef") != std::string::npos);
    assert(searchRecording.body.find("backendNativeId") == std::string::npos);
    assert(searchRecording.headers.at("Cache-Control") == "no-store");
    ApiResponse searchEpg;
    assert(runtime.tryHandleGet(
        "/api/v1/search/epg?backendId=backend-a&query=Hobbit",
        "actor:tv", "search-epg", "", searchEpg, "", "", {"backend-a"}));
    assert(searchEpg.statusCode == 200);
    assert(searchEpg.body.find("\"kind\":\"epg\"") != std::string::npos);
    for (const std::string& invalid : {
        "backendId=backend-a&query=%GG",
        "backendId=backend-a&query=a",
        "backendId=backend-a&query=Hobbit&limit=31",
        "backendId=backend-a&query=Hobbit&query=duplicate",
        "backendId=backend-a&query=Hobbit&offset=1000001"})
    {
        ApiResponse response;
        assert(runtime.tryHandleGet(
            "/api/v1/search/recordings?" + invalid,
            "actor:tv", "invalid-search", "", response, "", "", {"backend-a"}));
        assert(response.statusCode == 400);
    }
    ApiResponse deniedSearch;
    assert(runtime.tryHandleGet(
        "/api/v1/search/recordings?backendId=backend-a&query=Hobbit",
        "actor:tv", "wrong-scope", "", deniedSearch, "", "", {"backend-b"}));
    assert(deniedSearch.statusCode == 400);
    ApiResponse anonymousSearch;
    assert(runtime.tryHandleGet(
        "/api/v1/search/epg?backendId=backend-a&query=Hobbit",
        "", "anonymous", "", anonymousSearch, "", "", {"backend-a"}));
    assert(anonymousSearch.statusCode == 401);
    // HTTP-level parity: transport the same three canonical Web search
    // records through the actual Public-v1 serializer, without file paths.
    runtime.registerSearchLookup([](const PublicSearchRequest& request) {
        GlobalSearchResult web;
        web.query = request.query;
        web.backendId = request.backendId;
        web.recordingTotal = 3;
        std::vector<VdrRecording> live;
        for (int i = 0; i != 3; ++i)
        {
            GlobalSearchRecording result;
            result.backendId = request.backendId;
            result.backendNativeId = "hobbit-" + std::to_string(i);
            result.title = "Der Hobbit " + std::to_string(i);
            result.path = "/private/recordings/hobbit";
            web.recordings.push_back(result);
            VdrRecording current;
            current.backendId = request.backendId;
            current.backendNativeId = result.backendNativeId;
            current.title = "VDR-native-title";
            live.push_back(current);
        }
        return PublicSearchProjection::project(
            request, web, live,
            [](const std::string&, const std::string& native)
                -> std::optional<std::string>
            {
                if (native == "hobbit-0")
                    return "rec_aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
                if (native == "hobbit-1")
                    return "rec_bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb";
                if (native == "hobbit-2")
                    return "rec_cccccccccccccccccccccccccccccccc";
                return std::nullopt;
            });
    });
    ApiResponse parity;
    assert(runtime.tryHandleGet(
        "/api/v1/search/recordings?backendId=backend-a&query=Hobbit&limit=30&offset=0",
        "actor:tv", "web-parity", "", parity, "", "", {"backend-a"}));
    assert(parity.statusCode == 200);
    assert(parity.body.find("\"totalCount\":3") != std::string::npos);
    assert(parity.body.find("\"title\":\"Der Hobbit 0\"") != std::string::npos);
    assert(parity.body.find("\"title\":\"Der Hobbit 1\"") != std::string::npos);
    assert(parity.body.find("\"title\":\"Der Hobbit 2\"") != std::string::npos);
    assert(parity.body.find("VDR-native-title") == std::string::npos);
    assert(parity.body.find("/private") == std::string::npos);
    assert(parity.body.find("backendNativeId") == std::string::npos);
    assert(parity.body.find("\"hasMore\":false") != std::string::npos);

    runtime.resetSearchLookup();
    return 0;
}
