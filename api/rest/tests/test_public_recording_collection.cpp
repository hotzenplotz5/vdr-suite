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
            if (request.backendId != "backend-a")
                return page;
            if (!request.homeView.empty()) {
                assert(request.homeView == "new" || (request.homeView == "movies" &&
                    request.movieReleaseYearFrom == 2022 && request.movieReleaseYearTo == 2026));
                page.status = PublicRecordingCollectionStatus::ok;
                page.totalEntries = 2;
                auto item = recording("rec_00000000000000000000000000000003");
                item.subtitle = "2026";
                item.releaseDate = "2026-05-03";
                item.preferredArtworkId = "fedcba9876543210fedcba9876543210";
                if (request.homeView == "new")
                {
                    item.card.cardMediaType = "series";
                    item.card.cardTitle = "Serie";
                    item.card.episodeName = "Pilot";
                    item.card.seasonNumber = 1;
                    item.card.episodeNumber = 2;
                    item.card.seriesGroupKey = "sg1_0123456789abcdef0123456789abcdef";
                    item.card.seriesAncestorFolderIds = {"fld1_0123456789abcdef0123456789abcdef"};
                }
                if (request.offset == 0) page.recordings.push_back(item);
                page.hasMore = request.offset == 0;
                return page;
            }
            if (!request.browseFolders) return page;
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
    ApiResponse movies;
    assert(runtime.tryHandleGet(
        "/api/v1/recordings?backendId=backend-a&view=movies&movieReleaseYearFrom=2022&movieReleaseYearTo=2026&limit=1",
        "actor:tv", "home-movies", "", movies, "", "", {"backend-a"}));
    assert(movies.statusCode == 200);
    assert(movies.body.find("\"subtitle\":\"2026\"") != std::string::npos);
    assert(movies.body.find("\"releaseDate\":\"2026-05-03\"") != std::string::npos);
    assert(movies.body.find("\"preferredArtworkId\":\"fedcba9876543210fedcba9876543210\"") != std::string::npos);
    assert(movies.body.find("backendNativeId") == std::string::npos);
    assert(movies.body.find("\"totalCount\":2") != std::string::npos);
    ApiResponse newly;
    assert(runtime.tryHandleGet(
        "/api/v1/recordings?backendId=backend-a&view=new&limit=1",
        "actor:tv", "home-new", "", newly, "", "", {"backend-a"}));
    assert(newly.statusCode == 200);
    assert(newly.body.find("\"cardMetadata\":{\"mediaType\":\"series\"") != std::string::npos);
    assert(newly.body.find("\"episodeName\":\"Pilot\"") != std::string::npos);
    assert(newly.body.find("\"seriesGrouping\":{\"groupKey\":\"sg1_0123456789abcdef0123456789abcdef\"") != std::string::npos);
    assert(newly.body.find("fld1_0123456789abcdef0123456789abcdef") != std::string::npos);
    assert(newly.body.find("backendNativeId") == std::string::npos);
    assert(newly.body.find("releaseDate") == std::string::npos);
    for (const std::string& query : {"view=movies", "view=new&movieReleaseYearFrom=2022",
                                    "view=movies&movieReleaseYearFrom=2026&movieReleaseYearTo=2022"}) {
        ApiResponse invalidHome;
        assert(runtime.tryHandleGet("/api/v1/recordings?backendId=backend-a&" + query,
            "actor:tv", "home-invalid", "", invalidHome, "", "", {"backend-a"}));
        assert(invalidHome.statusCode == 400);
    }
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
    int combinedLookups = 0;
    runtime.registerSearchLookup([&combinedLookups](const PublicSearchRequest& request) {
        PublicSearchResult result;
        if (request.combined) ++combinedLookups;
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
                item.durationSeconds = 7200;
                item.artworkAvailable = true;
            } else {
                item.recordingId = "rec_0123456789abcdef0123456789abcdef";
                item.personPortraitRecordingId = item.recordingId;
                item.personPortraitArtworkId = "0123456789abcdef0123456789abcdef";
                item.preferredArtworkId = "abcdef0123456789abcdef0123456789";
                item.startTime = "1791565200";
                item.durationSeconds = 3600;
                item.sizeMb = 2048;
                item.artworkAvailable = true;
            }
            result.items.push_back(item);
            if (request.combined)
            {
                PublicSearchItem event;
                event.backendId = request.backendId;
                event.channelId = "S19.2E-1-100";
                event.eventId = "123";
                event.title = "EPG-only person";
                event.startTime = "1791565200";
                event.endTime = "1791572400";
                event.personPortraitRecordingId = item.recordingId;
                event.personPortraitArtworkId = item.personPortraitArtworkId;
                result.epgItems.push_back(event);
                result.epgTotalCount = 1U;
            }
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
    assert(searchRecording.body.find("\"startTime\":\"1791565200\"") != std::string::npos);
    assert(searchRecording.body.find("\"durationSeconds\":3600") != std::string::npos);
    assert(searchRecording.body.find("\"sizeMb\":2048") != std::string::npos);
    assert(searchRecording.body.find("\"artworkAvailable\":true") != std::string::npos);
    assert(searchRecording.body.find("\"personPortrait\":{\"recordingId\":\"rec_0123456789abcdef0123456789abcdef\",\"artworkId\":\"0123456789abcdef0123456789abcdef\"}") != std::string::npos);
    assert(searchRecording.body.find("\"preferredArtworkId\":\"abcdef0123456789abcdef0123456789\"") != std::string::npos);
    assert(searchRecording.headers.at("Cache-Control") == "no-store");
    ApiResponse searchEpg;
    assert(runtime.tryHandleGet(
        "/api/v1/search/epg?backendId=backend-a&query=Hobbit",
        "actor:tv", "search-epg", "", searchEpg, "", "", {"backend-a"}));
    assert(searchEpg.statusCode == 200);
    assert(searchEpg.body.find("\"kind\":\"epg\"") != std::string::npos);
    assert(searchEpg.body.find("\"durationSeconds\":7200") != std::string::npos);
    assert(searchEpg.body.find("\"artworkAvailable\":true") != std::string::npos);
    assert(searchEpg.body.find("\"sizeMb\"") == std::string::npos);
    assert(searchEpg.body.find("\"personPortrait\"") == std::string::npos);
    assert(searchEpg.body.find("\"preferredArtworkId\"") == std::string::npos);
    ApiResponse combinedSearch;
    assert(runtime.tryHandleGet(
        "/api/v1/search?backendId=backend-a&query=Hobbit&limit=24",
        "actor:tv", "combined-search", "", combinedSearch, "", "", {"backend-a"}));
    assert(combinedSearch.statusCode == 200 && combinedLookups == 1);
    assert(combinedSearch.body.find("\"recordings\":{\"items\":[") != std::string::npos);
    assert(combinedSearch.body.find("\"epg\":{\"items\":[") != std::string::npos);
    assert(combinedSearch.body.find("\"title\":\"EPG-only person\"") != std::string::npos);
    assert(combinedSearch.body.find("\"personPortrait\"", combinedSearch.body.find("\"epg\":")) != std::string::npos);
    assert(combinedSearch.body.find("backendNativeId") == std::string::npos);
    ApiResponse wrongCombinedScope;
    assert(runtime.tryHandleGet(
        "/api/v1/search?backendId=backend-a&query=Hobbit",
        "actor:tv", "combined-scope", "", wrongCombinedScope, "", "", {"backend-b"}));
    assert(wrongCombinedScope.statusCode == 400 && combinedLookups == 1);
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
            result.matchedPerson = "Ewan McGregor";
            result.matchedRole = "actor";
            result.personKey = "ewan mcgregor\nactor";
            result.matchReason = "person";
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
    assert(parity.body.find("\"personProjectionVersion\":1") != std::string::npos);
    assert(parity.body.find("\"matchedPerson\":\"Ewan McGregor\"") != std::string::npos);
    assert(parity.body.find("\"personKey\":\"ewan mcgregor\\nactor\"") != std::string::npos);
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
            result.item.voteAverage = 7.5;
            result.item.voteCount = 123;
            result.item.trailerLookupAvailable = true;
            result.item.firstAired = "2026-10-19";
            result.item.genres = {"Drama", "Mystery"};
            result.item.people.push_back({"actor", "Actor \"Name\"", "Character",
                {std::string(32, 'a'), 120, 180}});
            result.item.preferredArtwork = {std::string(32, 'b'), 720, 1080};
            result.item.images.push_back({"landscape", {std::string(32, 'c'), 1920, 1080}});
            // A manual Genre assignment may exist without a selected
            // manual metadata entity.
            result.item.manualGenreId = "film.drama";
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
    assert(detail.body.find("\"voteAverage\":7.5") != std::string::npos);
    assert(detail.body.find("\"voteCount\":123") != std::string::npos);
    assert(detail.body.find("\"trailerLookupAvailable\":true") != std::string::npos);
    assert(detail.body.find("\"preferredArtwork\":{\"available\":true") != std::string::npos);
    assert(detail.body.find("\"images\":[{\"orientation\":\"landscape\"") != std::string::npos);
    assert(detail.body.find("/api/v1/recordings/rec_00000000000000000000000000000001/artwork/" +
        std::string(32, 'a') + "?backendId=backend-a") != std::string::npos);
    assert(detail.body.find("\"genres\":[\"Drama\",\"Mystery\"]") != std::string::npos);
    assert(detail.body.find("\"name\":\"Actor \\\"Name\\\"\"") != std::string::npos);
    assert(detail.body.find("\"manualGenreId\":\"film.drama\"") !=
        std::string::npos);
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
    runtime.registerRecordingDetailLookup(
        [](const PublicRecordingDetailRequest&) {
            PublicRecordingDetailResult selected;
            selected.status = PublicRecordingDetailStatus::ok;
            selected.item.recording =
                recording("rec_00000000000000000000000000000001");
            selected.item.metadataState = "available";
            selected.item.metadataSource = "manual";
            selected.item.contentKind = "series-episode";
            selected.item.providerTitle = "Manual episode";
            selected.item.overview = "Manually selected synopsis";
            selected.item.manualGenreId = "film.documentary";
            selected.item.seasonNumber = 2;
            selected.item.episodeNumber = 5;
            selected.item.manualAssignmentId =
                "mdasg_" + std::string(32, 'a');
            selected.item.metadataEntityId =
                "mdent_" + std::string(32, 'b');
            selected.item.manualRevision = 4;
            return selected;
        });
    ApiResponse manualDetail;
    assert(runtime.tryHandleGet(detailPath, "actor:tv", "meta-manual",
        "", manualDetail, "", "", {"backend-a"}));
    assert(manualDetail.statusCode == 200);
    assert(manualDetail.body.find("\"source\":\"manual\"") !=
        std::string::npos);
    assert(manualDetail.body.find("\"providerTitle\":\"Manual episode\"") !=
        std::string::npos);
    assert(manualDetail.body.find("\"overview\":\"Manually selected synopsis\"") !=
        std::string::npos);
    assert(manualDetail.body.find("\"manualRevision\":4") !=
        std::string::npos);
    assert(manualDetail.body.find("\"manualGenreId\":\"film.documentary\"") !=
        std::string::npos);
    assert(manualDetail.body.find("\"manualAssignmentId\":\"mdasg_") !=
        std::string::npos);
    assert(manualDetail.body.find("\"metadataEntityId\":\"mdent_") !=
        std::string::npos);
    assert(manualDetail.body.find("resourceKey") == std::string::npos);
    assert(manualDetail.body.find("externalId") == std::string::npos);
    assert(manualDetail.body.find("actorRef") == std::string::npos);
    assert(manualDetail.headers.at("Cache-Control") == "no-store");

    // Fail closed on selected records with malformed provenance rather
    // than returning arbitrary untrusted strings in the public contract.
    runtime.registerRecordingDetailLookup(
        [](const PublicRecordingDetailRequest&) {
            PublicRecordingDetailResult malformedManual;
            malformedManual.status = PublicRecordingDetailStatus::ok;
            malformedManual.item.recording =
                recording("rec_00000000000000000000000000000001");
            malformedManual.item.metadataState = "available";
            malformedManual.item.metadataSource = "manual";
            malformedManual.item.contentKind = "movie";
            malformedManual.item.providerTitle = "Invalid assignment";
            malformedManual.item.manualAssignmentId = "mdasg_bad";
            malformedManual.item.metadataEntityId =
                "mdent_" + std::string(32, 'b');
            malformedManual.item.manualRevision = 4;
            return malformedManual;
        });
    ApiResponse rejectedManual;
    assert(runtime.tryHandleGet(detailPath, "actor:tv", "meta-invalid-manual",
        "", rejectedManual, "", "", {"backend-a"}));
    assert(rejectedManual.statusCode == 503);
    runtime.resetRecordingDetailLookup();

    // M2: Recording -> scoped artwork reference -> actual binary response.
    const std::string artworkPath =
        "/api/v1/recordings/rec_00000000000000000000000000000001/artwork";
    const std::string opaqueArtworkId(32, 'a');
    runtime.registerRecordingArtworkLookup(
        [&opaqueArtworkId](const PublicRecordingArtworkRequest& request) {
            PublicRecordingArtworkResult out;
            if (request.backendId == "backend-down")
                return out;
            if (request.recordingId !=
                "rec_00000000000000000000000000000001")
            {
                out.status = PublicRecordingArtworkStatus::notFound;
                return out;
            }
            if (!request.artworkId.empty() &&
                request.artworkId != opaqueArtworkId)
            {
                out.status = PublicRecordingArtworkStatus::notFound;
                return out;
            }
            out.status = PublicRecordingArtworkStatus::ok;
            out.items.push_back({opaqueArtworkId, "poster", 640, 960});
            if (!request.artworkId.empty())
            {
                out.contentType = "image/png";
                out.content = "PNGbytes";
            }
            return out;
        });
    ApiResponse artList;
    assert(runtime.tryHandleGet(artworkPath + "?backendId=backend-a",
        "actor:tv", "art-list", "", artList, "", "", {"backend-a"}));
    assert(artList.statusCode == 200);
    assert(artList.body.find("poster") != std::string::npos);
    assert(artList.body.find("/artwork/" + opaqueArtworkId +
        "?backendId=backend-a") != std::string::npos);
    assert(artList.body.find("recordingPath") == std::string::npos);
    ApiResponse artBytes;
    assert(runtime.tryHandleGet(artworkPath + "/" + opaqueArtworkId +
        "?backendId=backend-a", "actor:tv", "art-bytes", "",
        artBytes, "", "", {"backend-a"}));
    assert(artBytes.statusCode == 200);
    assert(artBytes.contentType == "image/png");
    assert(artBytes.body == "PNGbytes");
    assert(artBytes.headers.at("Cache-Control") == "private, max-age=300");
    assert(artBytes.headers.at("X-Content-Type-Options") == "nosniff");
    for (const std::string& query : {
        std::string("?backendId=backend-a&variant=home"),
        std::string("?variant=home&backendId=backend-a")})
    {
        ApiResponse previewSource;
        assert(runtime.tryHandleGet(artworkPath + "/" + opaqueArtworkId + query,
            "actor:tv", "art-preview", "", previewSource, "", "", {"backend-a"}));
        assert(previewSource.statusCode == 200);
        assert(previewSource.body == artBytes.body); // HTTP decorator owns resizing.
    }
    for (const std::string& query : {
        std::string("?backendId=backend-a&variant=unknown"),
        std::string("?backendId=backend-a&variant=home&variant=home"),
        std::string("?backendId=backend-a&variant="),
        std::string("?backendId=backend-a&variant=home&other=1")})
    {
        ApiResponse invalidPreview;
        assert(runtime.tryHandleGet(artworkPath + "/" + opaqueArtworkId + query,
            "actor:tv", "art-invalid-preview", "", invalidPreview, "", "", {"backend-a"}));
        assert(invalidPreview.statusCode == 400);
    }
    ApiResponse invalidListPreview;
    assert(runtime.tryHandleGet(artworkPath + "?backendId=backend-a&variant=home",
        "actor:tv", "art-list-preview", "", invalidListPreview, "", "", {"backend-a"}));
    assert(invalidListPreview.statusCode == 400);
    ApiResponse artWrongBackend;
    assert(runtime.tryHandleGet(artworkPath + "?backendId=backend-a",
        "actor:tv", "art-cross", "", artWrongBackend, "", "",
        {"backend-b"}));
    assert(artWrongBackend.statusCode == 400);
    ApiResponse artAnonymous;
    assert(runtime.tryHandleGet(artworkPath + "?backendId=backend-a",
        "", "art-anon", "", artAnonymous, "", "", {"backend-a"}));
    assert(artAnonymous.statusCode == 401);
    ApiResponse artMissing;
    assert(runtime.tryHandleGet(artworkPath + "/" + std::string(32, 'b') +
        "?backendId=backend-a", "actor:tv", "art-missing", "",
        artMissing, "", "", {"backend-a"}));
    assert(artMissing.statusCode == 404);
    ApiResponse artBadId;
    assert(runtime.tryHandleGet(artworkPath + "/%2e%2e?backendId=backend-a",
        "actor:tv", "art-bad", "", artBadId, "", "", {"backend-a"}));
    assert(artBadId.statusCode == 400);
    // An empty asset segment must not alias the collection route and emit
    // broken /artwork//<id> URLs.
    ApiResponse artEmptyAsset;
    assert(runtime.tryHandleGet(artworkPath + "/?backendId=backend-a",
        "actor:tv", "art-empty", "", artEmptyAsset, "", "",
        {"backend-a"}));
    assert(artEmptyAsset.statusCode == 400);
    runtime.resetRecordingArtworkLookup();
    runtime.resetSearchLookup();
    return 0;
}
