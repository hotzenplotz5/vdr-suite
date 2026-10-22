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
                leaf.singleRecording.edited = true;
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
    assert(children.body.find("\"edited\":true") != std::string::npos);
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

    runtime.registerSearchLookup([](const PublicSearchRequest& request) {
        GlobalSearchResult web;
        web.backendId = request.backendId;
        web.recordingTotal = 2;
        std::vector<VdrRecording> live;
        for (int i = 0; i < 2; ++i)
        {
            GlobalSearchRecording found;
            found.backendId = request.backendId;
            found.backendNativeId = i == 0 ? "cut-1" : "manual-2";
            found.title = i == 0 ? "Filme~%Der_Hobbit" :
                "Manueller Hobbit-Titel";
            found.startTime = "2026-10-15";
            web.recordings.push_back(found);
            VdrRecording original;
            original.backendId = request.backendId;
            original.backendNativeId = found.backendNativeId;
            original.title = i == 0 ? found.title : "Filme~Der_Hobbit";
            original.path = i == 0 ?
                "/srv/vdr/video/Filme/%Der_Hobbit/2026-10-15.rec" :
                "/srv/vdr/video/Filme/Der_Hobbit/2026-10-15.rec";
            live.push_back(original);
        }
        return PublicSearchProjection::project(
            request, web, live,
            [](const std::string&, const std::string& native)
                -> std::optional<std::string>
            {
                if (native == "cut-1")
                    return "rec_dddddddddddddddddddddddddddddddd";
                if (native == "manual-2")
                    return "rec_eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee";
                return std::nullopt;
            });
    });
    ApiResponse cutSearch;
    assert(runtime.tryHandleGet(
        "/api/v1/search/recordings?backendId=backend-a&query=Hobbit",
        "actor:tv", "cut-search", "", cutSearch, "", "", {"backend-a"}));
    assert(cutSearch.statusCode == 200);
    assert(cutSearch.body.find("\"title\":\"Der Hobbit\",\"recordingId\":"
        "\"rec_dddddddddddddddddddddddddddddddd\",\"edited\":true") !=
        std::string::npos);
    assert(cutSearch.body.find("\"title\":\"Manueller Hobbit-Titel\"") !=
        std::string::npos);
    assert(cutSearch.body.find("/srv/vdr/video") == std::string::npos);

    // M1 detail: exact public rec_ identity and backend, no native VDR paths
    // or provider artwork URL. The collection contract remains unchanged.
    runtime.registerRecordingDetailLookup(
        [](const PublicRecordingDetailRequest& request) {
            PublicRecordingDetailResult result;
            if (request.backendId == "backend-down")
                return result;
            if (request.recordingId !=
                "rec_00000000000000000000000000000001") {
                result.status = PublicRecordingDetailStatus::notFound;
                return result;
            }
            result.status = PublicRecordingDetailStatus::ok;
            result.item.recording = recording(
                "rec_00000000000000000000000000000001");
            result.item.metadataState = "available";
            result.item.metadataSource = "restfulapi-scraper-bridge";
            result.item.contentKind = "series-episode";
            result.item.subtitle = "VDR: Untertitel";
            result.item.epgTitle = "EPG";
            result.item.epgDescription = "EPG description";
            result.item.providerTitle = "Scraped title";
            result.item.originalTitle = "Original";
            result.item.seriesTitle = "Series";
            result.item.episodeTitle = "Episode";
            result.item.genreText = "Drama";
            result.item.seasonNumber = 3;
            result.item.episodeNumber = 7;
            return result;
        });

    const std::string detailPath =
        "/api/v1/recordings/rec_00000000000000000000000000000001"
        "?backendId=backend-a";
    ApiResponse detail;
    assert(runtime.tryHandleGet(detailPath, "actor:tv", "meta-detail",
        "", detail, "", "", {"backend-a"}));
    assert(detail.statusCode == 200);
    assert(detail.body.find("\"metadata\":{\"state\":\"available\"") !=
        std::string::npos);
    assert(detail.body.find("\"seasonNumber\":3") != std::string::npos);
    assert(detail.body.find("\"providerTitle\":\"Scraped title\"") !=
        std::string::npos);
    assert(detail.body.find("\"episodeNumber\":7") != std::string::npos);
    assert(detail.body.find("\"subtitle\":\"VDR: Untertitel\"") !=
        std::string::npos);
    assert(detail.body.find("\"recordingId\":"
        "\"rec_00000000000000000000000000000001\"") !=
        std::string::npos);
    assert(detail.body.find("backendNativeId") == std::string::npos);
    assert(detail.body.find("recordingPath") == std::string::npos);
    assert(detail.body.find("/private") == std::string::npos);
    assert(detail.body.find("poster.jpg") == std::string::npos);
    assert(detail.headers.at("Cache-Control") == "no-store");

    ApiResponse malformed;
    assert(runtime.tryHandleGet(
        "/api/v1/recordings/rec_bad?backendId=backend-a",
        "actor:tv", "meta-malformed", "", malformed, "", "",
        {"backend-a"}));
    assert(malformed.statusCode == 400);
    ApiResponse duplicateQuery;
    assert(runtime.tryHandleGet(detailPath + "&backendId=backend-a",
        "actor:tv", "meta-duplicate", "", duplicateQuery, "", "",
        {"backend-a"}));
    assert(duplicateQuery.statusCode == 400);
    ApiResponse crossBackend;
    assert(runtime.tryHandleGet(detailPath,
        "actor:tv", "meta-backend", "", crossBackend, "", "",
        {"backend-b"}));
    assert(crossBackend.statusCode == 400);
    ApiResponse unauthenticatedDetail;
    assert(runtime.tryHandleGet(detailPath, "", "meta-unauth", "",
        unauthenticatedDetail, "", "", {"backend-a"}));
    assert(unauthenticatedDetail.statusCode == 401);
    ApiResponse missingDetail;
    assert(runtime.tryHandleGet(
        "/api/v1/recordings/rec_ffffffffffffffffffffffffffffffff"
        "?backendId=backend-a", "actor:tv", "meta-not-found", "",
        missingDetail, "", "", {"backend-a"}));
    assert(missingDetail.statusCode == 404);
    ApiResponse unavailableDetail;
    assert(runtime.tryHandleGet(
        "/api/v1/recordings/rec_00000000000000000000000000000001"
        "?backendId=backend-down", "actor:tv", "meta-down", "",
        unavailableDetail, "", "", {"backend-down"}));
    assert(unavailableDetail.statusCode == 503);

    runtime.registerRecordingDetailLookup(
        [](const PublicRecordingDetailRequest&) {
            PublicRecordingDetailResult invalid;
            invalid.status = PublicRecordingDetailStatus::ok;
            invalid.item.recording =
                recording("rec_00000000000000000000000000000001");
            invalid.item.recording.backendId = "backend-b";
            return invalid;
        });
    ApiResponse badSource;
    assert(runtime.tryHandleGet(detailPath, "actor:tv", "meta-bad-source",
        "", badSource, "", "", {"backend-a"}));
    assert(badSource.statusCode == 503);
    runtime.resetRecordingDetailLookup();

    runtime.resetSearchLookup();
    return 0;
}
