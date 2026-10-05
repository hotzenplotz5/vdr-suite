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
                result.assignments.push_back(
                    PublicTimerAssignmentCollectionItem{
                        eligible[index],
                        request.backendId});
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
    return 0;
}
