#include "PublicApiRuntime.h"

#include <cassert>
#include <string>

namespace
{
const char* Route =
    "/api/v1/timer-assignments/assignment:one?backend=backend-one";

std::string validBody()
{
    return "{"
        "\"nativeTimer\":{"
        "\"title\":\"Evening News\","
        "\"directory\":\"News\","
        "\"day\":\"2026-09-23\","
        "\"weekdays\":\"-------\","
        "\"startTime\":\"2015\","
        "\"endTime\":\"2100\","
        "\"priority\":50,"
        "\"lifetime\":99,"
        "\"enabled\":true,"
        "\"vps\":false"
        "}}";
}
}

int main()
{
    PublicApiRuntime& runtime = PublicApiRuntime::instance();
    runtime.resetTimerCreateSubmission();
    runtime.resetTimerAssignmentLookup();

    runtime.registerTimerAssignmentLookup(
        [](
            const std::string& timerAssignmentId,
            const std::string& backendId)
        {
            PublicTimerAssignmentLookupResult result;
            if (timerAssignmentId != "assignment:one" ||
                backendId != "backend-one")
            {
                result.status =
                    PublicTimerAssignmentLookupStatus::notFound;
                return result;
            }
            result.status = PublicTimerAssignmentLookupStatus::ok;
            result.assignment.timerAssignmentId = timerAssignmentId;
            result.assignment.backendId = backendId;
            result.assignment.resourceRevision = "7";
            return result;
        });

    ApiResponse read;
    assert(runtime.tryHandleGet(
        Route,
        "actor:one",
        "request-read",
        "",
        read,
        "",
        "backend-one"));
    assert(read.statusCode == 200);
    const std::string etag = read.headers.at("ETag");
    assert(!etag.empty());

    ApiResponse missingPrecondition;
    assert(runtime.tryHandlePost(
        Route,
        validBody(),
        "actor:one",
        "request-428",
        "",
        missingPrecondition,
        "idem-one",
        "",
        "backend-one"));
    assert(missingPrecondition.statusCode == 428);
    assert(missingPrecondition.body.find(
        "\"code\":\"precondition_required\"") !=
        std::string::npos);

    ApiResponse stale;
    assert(runtime.tryHandlePost(
        Route,
        validBody(),
        "actor:one",
        "request-412",
        "",
        stale,
        "idem-one",
        "\"wrong\"",
        "backend-one"));
    assert(stale.statusCode == 412);
    assert(stale.body.find(
        "\"code\":\"revision_conflict\"") !=
        std::string::npos);

    ApiResponse missingKey;
    assert(runtime.tryHandlePost(
        Route,
        validBody(),
        "actor:one",
        "request-no-key",
        "",
        missingKey,
        "",
        etag,
        "backend-one"));
    assert(missingKey.statusCode == 400);

    ApiResponse malformedJson;
    assert(runtime.tryHandlePost(
        Route,
        "{",
        "actor:one",
        "request-json",
        "",
        malformedJson,
        "idem-json",
        etag,
        "backend-one"));
    assert(malformedJson.statusCode == 400);
    assert(malformedJson.body.find(
        "\"code\":\"invalid_json\"") !=
        std::string::npos);

    ApiResponse unknownField;
    std::string withUnknown = validBody();
    withUnknown.insert(
        withUnknown.size() - 2U,
        ",\"backendGeneration\":9");
    assert(runtime.tryHandlePost(
        Route,
        withUnknown,
        "actor:one",
        "request-unknown",
        "",
        unknownField,
        "idem-unknown",
        etag,
        "backend-one"));
    assert(unknownField.statusCode == 400);
    assert(unknownField.body.find(
        "\"code\":\"invalid_request\"") !=
        std::string::npos);

    std::string invalidValues = validBody();
    const std::string priority = "\"priority\":50";
    invalidValues.replace(
        invalidValues.find(priority),
        priority.size(),
        "\"priority\":100");
    ApiResponse validation;
    assert(runtime.tryHandlePost(
        Route,
        invalidValues,
        "actor:one",
        "request-validation",
        "",
        validation,
        "idem-validation",
        etag,
        "backend-one"));
    assert(validation.statusCode == 422);
    assert(validation.body.find(
        "\"code\":\"validation_error\"") !=
        std::string::npos);

    bool callbackCalled = false;
    runtime.registerTimerCreateSubmission(
        [&](const PublicTimerCreateSubmissionRequest& request)
        {
            callbackCalled = true;
            assert(request.timerAssignmentId == "assignment:one");
            assert(request.backendId == "backend-one");
            assert(request.actorRef == "actor:one");
            assert(request.idempotencyKey == "idem-success");
            assert(request.expectedResourceRevision == "7");
            assert(request.specification.title == "Evening News");
            assert(request.specification.directory == "News");

            PublicTimerCreateSubmissionResult result;
            result.status = PublicTimerCreateSubmissionStatus::accepted;
            result.operation.operationId = "op-public-create-1";
            result.operation.state = "dispatching";
            result.operation.backendId = "backend-one";
            result.operation.resourceRevision = "2";
            return result;
        });

    ApiResponse accepted;
    assert(runtime.tryHandlePost(
        Route,
        validBody(),
        "actor:one",
        "request-success",
        "correlation-success",
        accepted,
        "idem-success",
        etag,
        "backend-one"));
    assert(callbackCalled);
    assert(accepted.statusCode == 202);
    assert(accepted.headers.at("Location") ==
        "/api/v1/operations/op-public-create-1");
    assert(!accepted.headers.at("ETag").empty());
    assert(accepted.body.find(
        "\"operationId\":\"op-public-create-1\"") !=
        std::string::npos);
    assert(accepted.body.find(
        "\"state\":\"dispatching\"") !=
        std::string::npos);

    runtime.registerTimerCreateSubmission(
        [](const PublicTimerCreateSubmissionRequest&)
        {
            PublicTimerCreateSubmissionResult result;
            result.status =
                PublicTimerCreateSubmissionStatus::idempotencyConflict;
            return result;
        });
    ApiResponse conflict;
    assert(runtime.tryHandlePost(
        Route,
        validBody(),
        "actor:one",
        "request-conflict",
        "",
        conflict,
        "idem-conflict",
        etag,
        "backend-one"));
    assert(conflict.statusCode == 409);
    assert(conflict.body.find(
        "\"code\":\"idempotency_conflict\"") !=
        std::string::npos);

    ApiResponse capabilities;
    assert(runtime.tryHandleGet(
        "/api/v1/capabilities",
        "actor:one",
        "request-capabilities",
        "",
        capabilities));
    assert(capabilities.body.find(
        "\"id\":\"public-api.timer-assignments-create\"") !=
        std::string::npos);
    assert(capabilities.body.find(
        "\"public-api.timer-assignments-create\",\"version\":1,\"availability\":\"available\"") !=
        std::string::npos);

    ApiResponse deleteMismatch;
    assert(runtime.tryHandleUnsupportedMethod(
        "DELETE",
        Route,
        "request-delete",
        "",
        deleteMismatch));
    assert(deleteMismatch.statusCode == 405);
    assert(deleteMismatch.headers.at("Allow") == "GET, POST");

    runtime.resetTimerCreateSubmission();
    runtime.resetTimerAssignmentLookup();
    return 0;
}
