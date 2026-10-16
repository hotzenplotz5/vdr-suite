#include "PublicApiRuntime.h"

#include <cassert>
#include <string>
#include <vector>

namespace
{

std::string nextCursor(const std::string& body)
{
    const std::string marker = "\"nextCursor\":\"";
    const std::size_t start = body.find(marker);
    if (start == std::string::npos) return "";
    const std::size_t valueStart = start + marker.size();
    const std::size_t end = body.find('"', valueStart);
    if (end == std::string::npos) return "";
    return body.substr(valueStart, end - valueStart);
}

}

int main()
{
    PublicApiRuntime& runtime = PublicApiRuntime::instance();
    runtime.resetTimerAssignmentCollectionLookup();

    runtime.registerTimerAssignmentCollectionLookup(
        [](
            const PublicTimerAssignmentCollectionRequest& request)
        {
            PublicTimerAssignmentCollectionResult result;
            if (request.backendId == "backend-fail")
            {
                result.status =
                    PublicTimerAssignmentCollectionStatus::unavailable;
                return result;
            }

            result.status =
                PublicTimerAssignmentCollectionStatus::ok;
            if (request.backendId == "backend-empty")
                return result;

            const std::vector<std::string> ids = {
                "assignment:a",
                "assignment:b",
                "assignment:c"};

            std::vector<std::string> eligible;
            for (const std::string& id : ids)
            {
                if (request.afterTimerAssignmentId.empty() ||
                    id > request.afterTimerAssignmentId)
                    eligible.push_back(id);
            }

            result.hasMore = eligible.size() > request.limit;
            const std::size_t count =
                result.hasMore ? request.limit : eligible.size();
            for (std::size_t index = 0U; index < count; ++index)
            {
                PublicTimerAssignmentCollectionItem item;
                item.timerAssignmentId = eligible[index];
                item.backendId = request.backendId;
                if (eligible[index] == "assignment:a")
                {
                    item.plannedPresent = true;
                    item.plannedTitle = "Die Nachrichten";
                    item.plannedDay = "20261017";
                    item.plannedWeekdays = "-------";
                    item.plannedStartTime = "2015";
                    item.plannedEndTime = "2100";
                }
                result.assignments.push_back(std::move(item));
            }
            return result;
        });

    ApiResponse first;
    assert(runtime.tryHandleGet(
        "/api/v1/timer-assignments?backend=backend-one&limit=2",
        "actor:test",
        "phase69d-collection-first",
        "phase69d-correlation",
        first,
        "",
        "backend-one"));
    assert(first.statusCode == 200);
    assert(first.body.find(
        "\"timerAssignmentId\":\"assignment:a\"") !=
        std::string::npos);
    assert(first.body.find(
        "\"timerAssignmentId\":\"assignment:b\"") !=
        std::string::npos);
    assert(first.body.find(
        "\"planned\":{\"title\":\"Die Nachrichten\","
        "\"day\":\"20261017\",\"weekdays\":\"-------\","
        "\"startTime\":\"2015\",\"endTime\":\"2100\"}") !=
        std::string::npos);
    const std::size_t secondAssignment =
        first.body.find("\"timerAssignmentId\":\"assignment:b\"");
    assert(secondAssignment != std::string::npos);
    assert(first.body.find("\"planned\"", secondAssignment) ==
        std::string::npos);
    assert(first.body.find(
        "\"timerAssignmentId\":\"assignment:c\"") ==
        std::string::npos);
    assert(first.body.find("\"limit\":2") !=
        std::string::npos);
    assert(first.body.find("\"hasMore\":true") !=
        std::string::npos);
    assert(first.body.find("\"partial\":false") !=
        std::string::npos);
    assert(first.body.find("resourceRevision") ==
        std::string::npos);
    assert(first.body.find("nativeTimerBindingId") ==
        std::string::npos);
    assert(first.headers.find("ETag") == first.headers.end());
    assert(first.headers.at("Cache-Control") == "no-store");

    const std::string cursor = nextCursor(first.body);
    assert(!cursor.empty());
    assert(cursor.rfind("ta1_", 0U) == 0U);

    ApiResponse second;
    assert(runtime.tryHandleGet(
        std::string(
            "/api/v1/timer-assignments?backend=backend-one&limit=2&cursor=") +
            cursor,
        "actor:test",
        "phase69d-collection-second",
        "",
        second,
        "",
        "backend-one"));
    assert(second.statusCode == 200);
    assert(second.body.find(
        "\"timerAssignmentId\":\"assignment:c\"") !=
        std::string::npos);
    assert(second.body.find("\"nextCursor\":null") !=
        std::string::npos);
    assert(second.body.find("\"hasMore\":false") !=
        std::string::npos);

    ApiResponse wrongBackend;
    assert(runtime.tryHandleGet(
        std::string(
            "/api/v1/timer-assignments?backend=backend-two&cursor=") +
            cursor,
        "actor:test",
        "phase69d-collection-wrong-backend",
        "",
        wrongBackend,
        "",
        "backend-two"));
    assert(wrongBackend.statusCode == 400);

    ApiResponse malformedCursor;
    assert(runtime.tryHandleGet(
        "/api/v1/timer-assignments?backend=backend-one&cursor=invalid",
        "actor:test",
        "phase69d-collection-invalid-cursor",
        "",
        malformedCursor,
        "",
        "backend-one"));
    assert(malformedCursor.statusCode == 400);

    for (const char* badLimit : {"0", "101", "abc"})
    {
        ApiResponse bad;
        assert(runtime.tryHandleGet(
            std::string(
                "/api/v1/timer-assignments?backend=backend-one&limit=") +
                badLimit,
            "actor:test",
            "phase69d-collection-invalid-limit",
            "",
            bad,
            "",
            "backend-one"));
        assert(bad.statusCode == 400);
    }

    for (const char* badQuery : {
             "sort=state",
             "order=desc",
             "offset=1"})
    {
        ApiResponse bad;
        assert(runtime.tryHandleGet(
            std::string(
                "/api/v1/timer-assignments?backend=backend-one&") +
                badQuery,
            "actor:test",
            "phase69d-collection-invalid-query",
            "",
            bad,
            "",
            "backend-one"));
        assert(bad.statusCode == 400);
    }

    ApiResponse empty;
    assert(runtime.tryHandleGet(
        "/api/v1/timer-assignments?backend=backend-empty",
        "actor:test",
        "phase69d-collection-empty",
        "",
        empty,
        "",
        "backend-empty"));
    assert(empty.statusCode == 200);
    assert(empty.body.find("\"items\":[]") !=
        std::string::npos);
    assert(empty.body.find("\"partial\":false") !=
        std::string::npos);

    ApiResponse unavailable;
    assert(runtime.tryHandleGet(
        "/api/v1/timer-assignments?backend=backend-fail",
        "actor:test",
        "phase69d-collection-unavailable",
        "",
        unavailable,
        "",
        "backend-fail"));
    assert(unavailable.statusCode == 503);
    assert(unavailable.body.find(
        "\"code\":\"service_unavailable\"") !=
        std::string::npos);

    ApiResponse post;
    assert(runtime.tryHandlePost(
        "/api/v1/timer-assignments?backend=backend-one",
        "phase69d-collection-post",
        "",
        post));
    assert(post.statusCode == 405);
    assert(post.headers.at("Allow") == "GET");

    ApiResponse remove;
    assert(runtime.tryHandleUnsupportedMethod(
        "DELETE",
        "/api/v1/timer-assignments?backend=backend-one",
        "phase69d-collection-delete",
        "",
        remove));
    assert(remove.statusCode == 405);
    assert(remove.headers.at("Allow") == "GET");

    runtime.resetTimerAssignmentCollectionLookup();

    // Read-only Device SearchTimers do not disclose backend-native IDs.
    runtime.resetSearchTimerCollectionLookup();
    runtime.registerSearchTimerCollectionLookup(
        [](const PublicSearchTimerCollectionRequest& request)
        {
            PublicSearchTimerCollectionResult page;
            if (request.backendId == "offline") return page;
            page.status = PublicSearchTimerCollectionStatus::ok;
            if (request.backendId == "empty") return page;
            const std::vector<std::string> ids = {
                "st_00000000000000000000000000000001",
                "st_00000000000000000000000000000002",
                "st_00000000000000000000000000000003"};
            for (const auto& id : ids)
            {
                if (!request.afterSearchTimerId.empty() &&
                    id <= request.afterSearchTimerId) continue;
                if (page.items.size() >= request.limit)
                {
                    page.hasMore = true;
                    break;
                }
                PublicSearchTimerCollectionItem item;
                item.searchTimerId = id;
                item.backendId = request.backendId;
                item.name = "Terra X";
                item.query = "Terra X";
                item.state = "active";
                page.items.push_back(std::move(item));
            }
            return page;
        });
    ApiResponse searchFirst;
    assert(runtime.tryHandleGet(
        "/api/v1/search-timers?backend=one&limit=2",
        "actor:test", "st-read", "", searchFirst, "", "one"));
    assert(searchFirst.statusCode == 200);
    assert(searchFirst.body.find("\"searchTimerId\":\"st_") != std::string::npos);
    assert(searchFirst.body.find("backendNativeId") == std::string::npos);
    assert(searchFirst.body.find("\"state\":\"active\"") != std::string::npos);
    assert(searchFirst.body.find("\"hasMore\":true") != std::string::npos);
    const std::string searchCursor = nextCursor(searchFirst.body);
    assert(searchCursor.rfind("st1_", 0U) == 0U);
    ApiResponse searchSecond;
    assert(runtime.tryHandleGet(
        "/api/v1/search-timers?backend=one&limit=2&cursor=" + searchCursor,
        "actor:test", "st-next", "", searchSecond, "", "one"));
    assert(searchSecond.statusCode == 200);
    assert(searchSecond.body.find("st_00000000000000000000000000000003") !=
        std::string::npos);
    assert(searchSecond.body.find("\"hasMore\":false") != std::string::npos);
    ApiResponse searchCrossBackend;
    assert(runtime.tryHandleGet(
        "/api/v1/search-timers?backend=two&cursor=" + searchCursor,
        "actor:test", "st-scope", "", searchCrossBackend, "", "two"));
    assert(searchCrossBackend.statusCode == 400);
    for (const std::string& bad : {
             std::string("limit=31"), std::string("limit=0"),
             std::string("cursor=invalid"), std::string("offset=20"),
             std::string("backend=two")})
    {
        ApiResponse rejected;
        assert(runtime.tryHandleGet(
            "/api/v1/search-timers?backend=one&" + bad,
            "actor:test", "st-invalid", "", rejected, "", "one"));
        assert(rejected.statusCode == 400);
    }
    ApiResponse emptySearch;
    assert(runtime.tryHandleGet(
        "/api/v1/search-timers?backend=empty",
        "actor:test", "st-empty", "", emptySearch, "", "empty"));
    assert(emptySearch.statusCode == 200);
    assert(emptySearch.body.find("\"items\":[]") != std::string::npos);
    ApiResponse offlineSearch;
    assert(runtime.tryHandleGet(
        "/api/v1/search-timers?backend=offline",
        "actor:test", "st-offline", "", offlineSearch, "", "offline"));
    assert(offlineSearch.statusCode == 503);
    ApiResponse anonymousSearch;
    assert(runtime.tryHandleGet(
        "/api/v1/search-timers?backend=one",
        "", "st-anon", "", anonymousSearch, "", "one"));
    assert(anonymousSearch.statusCode == 401);
    ApiResponse searchPost;
    assert(runtime.tryHandlePost(
        "/api/v1/search-timers?backend=one", "st-post", "", searchPost));
    assert(searchPost.statusCode == 405);
    ApiResponse searchDelete;
    assert(runtime.tryHandleUnsupportedMethod(
        "DELETE", "/api/v1/search-timers?backend=one",
        "st-delete", "", searchDelete));
    assert(searchDelete.statusCode == 405);
    runtime.resetSearchTimerCollectionLookup();

    // Saved SearchTimer preview is a distinct, bounded Public-v1 resource.
    runtime.resetSearchTimerPreviewLookup();
    runtime.registerSearchTimerPreviewLookup(
        [](const PublicSearchTimerPreviewRequest& request)
        {
            PublicSearchTimerPreviewResult result;
            if (request.backendId == "offline") return result;
            if (request.searchTimerId ==
                "st_00000000000000000000000000000002")
            {
                result.status = PublicSearchTimerPreviewStatus::notFound;
                return result;
            }
            result.status = PublicSearchTimerPreviewStatus::ok;
            result.backendId = request.backendId;
            result.searchTimerId = request.searchTimerId;
            result.totalCount = 42;
            PublicSearchTimerPreviewEvent event;
            event.title = "Terra X";
            event.subtitle = "Planet Erde";
            event.channelId = "C-42";
            event.startTime = "1760724000";
            event.endTime = "1760727600";
            result.items.push_back(event);
            return result;
        });
    const std::string previewRoute =
        "/api/v1/search-timers/st_00000000000000000000000000000001/preview";
    ApiResponse preview;
    assert(runtime.tryHandleGet(previewRoute + "?backend=one",
        "actor:test", "preview-ok", "", preview, "", "one"));
    assert(preview.statusCode == 200);
    assert(preview.body.find("\"totalCount\":42") != std::string::npos);
    assert(preview.body.find("\"title\":\"Terra X\"") != std::string::npos);
    assert(preview.body.find("backendNativeId") == std::string::npos);
    assert(preview.headers.at("Cache-Control") == "no-store");
    for (const std::string& invalid : {
             std::string("backend=two"),
             std::string("backend=one&cursor=bad"),
             std::string("backend=one&offset=1"),
             std::string("backend=one&limit=31")})
    {
        ApiResponse bad;
        assert(runtime.tryHandleGet(previewRoute + "?" + invalid,
            "actor:test", "preview-invalid", "", bad, "", "one"));
        assert(bad.statusCode == 400);
    }
    ApiResponse previewNoMatch;
    assert(runtime.tryHandleGet(
        "/api/v1/search-timers/st_00000000000000000000000000000002/preview?backend=one",
        "actor:test", "preview-missing", "", previewNoMatch, "", "one"));
    assert(previewNoMatch.statusCode == 404);
    ApiResponse previewUnavailable;
    assert(runtime.tryHandleGet(previewRoute + "?backend=offline",
        "actor:test", "preview-offline", "", previewUnavailable, "", "offline"));
    assert(previewUnavailable.statusCode == 503);
    ApiResponse previewAnon;
    assert(runtime.tryHandleGet(previewRoute + "?backend=one",
        "", "preview-anon", "", previewAnon, "", "one"));
    assert(previewAnon.statusCode == 401);
    ApiResponse previewPost;
    assert(runtime.tryHandlePost(previewRoute + "?backend=one",
        "preview-post", "", previewPost));
    assert(previewPost.statusCode == 405);
    runtime.resetSearchTimerPreviewLookup();
    return 0;
}
