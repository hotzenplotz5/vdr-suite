#include "PublicApiRuntime.h"

#include <cassert>
#include <string>

int main()
{
    PublicApiRuntime& runtime = PublicApiRuntime::instance();
    runtime.resetTimerAssignmentLookup();

    runtime.registerTimerAssignmentLookup(
        [](
            const std::string& timerAssignmentId,
            const std::string& backendId)
        {
            PublicTimerAssignmentLookupResult result;

            if (timerAssignmentId.empty() || backendId.empty())
            {
                result.status =
                    PublicTimerAssignmentLookupStatus::invalid;
                return result;
            }

            if (timerAssignmentId != "assignment:one" ||
                backendId != "backend-one")
            {
                result.status =
                    PublicTimerAssignmentLookupStatus::notFound;
                return result;
            }

            result.status =
                PublicTimerAssignmentLookupStatus::ok;
            result.assignment.timerAssignmentId =
                timerAssignmentId;
            result.assignment.backendId =
                backendId;
            result.assignment.resourceRevision =
                "revision:7";
            return result;
        });

    ApiResponse anonymous;
    assert(runtime.tryHandleGet(
        "/api/v1/timer-assignments/assignment:one?backend=backend-one",
        "",
        "phase69c-timer-read-anonymous",
        "",
        anonymous,
        "",
        "backend-one"));
    assert(anonymous.statusCode == 401);

    ApiResponse missingAuthorizedBackend;
    assert(runtime.tryHandleGet(
        "/api/v1/timer-assignments/assignment:one?backend=backend-one",
        "actor:test",
        "phase69c-timer-read-no-scope",
        "",
        missingAuthorizedBackend));
    assert(missingAuthorizedBackend.statusCode == 400);
    assert(missingAuthorizedBackend.body.find(
        "\"code\":\"invalid_request\"") !=
        std::string::npos);

    ApiResponse resource;
    assert(runtime.tryHandleGet(
        "/api/v1/timer-assignments/assignment:one?backend=backend-one",
        "actor:test",
        "phase69c-timer-read-ok",
        "phase69c-correlation",
        resource,
        "",
        "backend-one"));
    assert(resource.statusCode == 200);
    assert(resource.body.find(
        "\"timerAssignmentId\":\"assignment:one\"") !=
        std::string::npos);
    assert(resource.body.find(
        "\"backendId\":\"backend-one\"") !=
        std::string::npos);
    assert(resource.body.find(
        "\"self\":\"/api/v1/timer-assignments/assignment:one?backend=backend-one\"") !=
        std::string::npos);
    assert(resource.body.find("resourceRevision") ==
        std::string::npos);
    assert(resource.body.find("\"state\"") ==
        std::string::npos);
    assert(resource.body.find("\"role\"") ==
        std::string::npos);
    assert(resource.body.find("nativeTimerBindingId") ==
        std::string::npos);
    assert(resource.headers.at("Cache-Control") == "no-store");
    assert(resource.headers.at("X-Content-Type-Options") == "nosniff");
    assert(resource.headers.at("X-Request-ID") ==
        "phase69c-timer-read-ok");
    assert(resource.headers.at("X-Correlation-ID") ==
        "phase69c-correlation");

    const std::string entityTag =
        resource.headers.at("ETag");
    assert(!entityTag.empty());

    ApiResponse notModified;
    assert(runtime.tryHandleGet(
        "/api/v1/timer-assignments/assignment:one?backend=backend-one",
        "actor:test",
        "phase69c-timer-read-304",
        "",
        notModified,
        entityTag,
        "backend-one"));
    assert(notModified.statusCode == 304);
    assert(notModified.body.empty());
    assert(notModified.headers.at("ETag") ==
        entityTag);

    ApiResponse malformedCondition;
    assert(runtime.tryHandleGet(
        "/api/v1/timer-assignments/assignment:one?backend=backend-one",
        "actor:test",
        "phase69c-timer-read-malformed",
        "",
        malformedCondition,
        "not-an-etag",
        "backend-one"));
    assert(malformedCondition.statusCode == 400);
    assert(malformedCondition.body.find(
        "\"code\":\"invalid_request\"") !=
        std::string::npos);

    ApiResponse hiddenWrongBackend;
    assert(runtime.tryHandleGet(
        "/api/v1/timer-assignments/assignment:one?backend=backend-two",
        "actor:test",
        "phase69c-timer-read-hidden",
        "",
        hiddenWrongBackend,
        "",
        "backend-two"));
    assert(hiddenWrongBackend.statusCode == 404);

    ApiResponse collectionStillClosed;
    assert(runtime.tryHandleGet(
        "/api/v1/timer-assignments",
        "actor:test",
        "phase69c-timer-read-collection",
        "",
        collectionStillClosed,
        "",
        "backend-one"));
    assert(collectionStillClosed.statusCode == 404);

    ApiResponse capabilities;
    assert(runtime.tryHandleGet(
        "/api/v1/capabilities",
        "actor:test",
        "phase69c-timer-read-capabilities",
        "",
        capabilities));
    assert(capabilities.statusCode == 200);
    assert(capabilities.body.find(
        "\"id\":\"public-api.timer-assignments-read\"") !=
        std::string::npos);
    assert(capabilities.body.find(
        "\"availability\":\"available\"") !=
        std::string::npos);

    ApiResponse postMismatch;
    assert(runtime.tryHandlePost(
        "/api/v1/timer-assignments/assignment:one?backend=backend-one",
        "phase69c-timer-read-post",
        "",
        postMismatch));
    assert(postMismatch.statusCode == 405);
    assert(postMismatch.headers.at("Allow") == "GET");

    ApiResponse deleteMismatch;
    assert(runtime.tryHandleUnsupportedMethod(
        "DELETE",
        "/api/v1/timer-assignments/assignment:one?backend=backend-one",
        "phase69c-timer-read-delete",
        "",
        deleteMismatch));
    assert(deleteMismatch.statusCode == 405);
    assert(deleteMismatch.headers.at("Allow") == "GET");

    runtime.resetTimerAssignmentLookup();
    return 0;
}
