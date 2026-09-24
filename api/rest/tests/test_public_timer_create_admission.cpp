#include "PublicApiRuntime.h"
#include "PublicResourcePreconditions.h"

#include <cassert>
#include <string>

namespace
{
ApiResponse post(
    PublicApiRuntime& runtime,
    const std::string& body = "{}",
    const std::string& actor = "actor-owner",
    const std::string& ifMatch =
        vdrsuite::http::publicStrongEntityTag("7"),
    const std::string& idempotencyKey = "idem-public-create-1",
    const std::string& contentType = "application/json; charset=utf-8",
    const std::string& backendId = "backend-one")
{
    ApiResponse response;
    const bool handled = runtime.tryHandlePost(
        "/api/v1/timer-assignments/assignment:one?backend=backend-one",
        "request-public-timer-create",
        "correlation-public-timer-create",
        response,
        body,
        actor,
        ifMatch,
        idempotencyKey,
        contentType,
        backendId);
    assert(handled);
    return response;
}

PublicTimerCreateAdmissionResult admissionResult(
    PublicTimerCreateAdmissionStatus status)
{
    PublicTimerCreateAdmissionResult result;
    result.status = status;
    if (status == PublicTimerCreateAdmissionStatus::accepted ||
        status == PublicTimerCreateAdmissionStatus::replayed)
    {
        result.operation.operationId =
            "op_0123456789abcdef0123456789abcdef";
        result.operation.state = "accepted";
        result.operation.backendId = "backend-one";
        result.operation.resourceRevision = "1";
    }
    return result;
}
}

int main()
{
    PublicApiRuntime& runtime = PublicApiRuntime::instance();
    runtime.resetTimerCreateAdmission();

    ApiResponse unavailableCapabilities;
    assert(runtime.tryHandleGet(
        "/api/v1/capabilities",
        "actor-owner",
        "cap-unavailable",
        "",
        unavailableCapabilities));
    assert(unavailableCapabilities.body.find(
        "{\"id\":\"public-api.timer-create-admission\","
        "\"version\":1,\"availability\":\"unavailable\"}") !=
        std::string::npos);

    const ApiResponse unconfigured = post(runtime);
    assert(unconfigured.statusCode == 503);

    int calls = 0;
    PublicTimerCreateAdmissionRequest captured;
    PublicTimerCreateAdmissionStatus nextStatus =
        PublicTimerCreateAdmissionStatus::accepted;

    runtime.registerTimerCreateAdmission(
        [&](const PublicTimerCreateAdmissionRequest& request)
        {
            ++calls;
            captured = request;
            return admissionResult(nextStatus);
        });

    ApiResponse availableCapabilities;
    assert(runtime.tryHandleGet(
        "/api/v1/capabilities",
        "actor-owner",
        "cap-available",
        "",
        availableCapabilities));
    assert(availableCapabilities.body.find(
        "{\"id\":\"public-api.timer-create-admission\","
        "\"version\":1,\"availability\":\"available\"}") !=
        std::string::npos);

    const ApiResponse accepted = post(runtime, " { \n } ");
    assert(accepted.statusCode == 202);
    assert(accepted.contentType ==
        "application/json; charset=utf-8");
    assert(accepted.headers.at("Location") ==
        "/api/v1/operations/op_0123456789abcdef0123456789abcdef");
    assert(accepted.headers.at("Cache-Control") == "no-store");
    assert(accepted.headers.at("X-Content-Type-Options") == "nosniff");
    assert(accepted.headers.at("X-Request-ID") ==
        "request-public-timer-create");
    assert(accepted.headers.at("X-Correlation-ID") ==
        "correlation-public-timer-create");
    assert(accepted.headers.at("ETag") ==
        vdrsuite::http::publicStrongEntityTag("1"));
    assert(accepted.body.find(
        "\"operationId\":\"op_0123456789abcdef0123456789abcdef\"") !=
        std::string::npos);
    assert(accepted.body.find("\"state\":\"accepted\"") !=
        std::string::npos);
    assert(accepted.body.find("\"backendId\":\"backend-one\"") !=
        std::string::npos);
    assert(accepted.body.find("idempotency") == std::string::npos);
    assert(accepted.body.find("payload") == std::string::npos);
    assert(calls == 1);
    assert(captured.actorRef == "actor-owner");
    assert(captured.backendId == "backend-one");
    assert(captured.timerAssignmentId == "assignment:one");
    assert(captured.expectedAssignmentRevision == "7");
    assert(captured.idempotencyKey == "idem-public-create-1");

    nextStatus = PublicTimerCreateAdmissionStatus::replayed;
    const ApiResponse replay = post(runtime);
    assert(replay.statusCode == 202);
    assert(replay.headers.at("Location") ==
        accepted.headers.at("Location"));
    assert(calls == 2);

    const int beforeValidation = calls;

    const ApiResponse missingIfMatch = post(
        runtime, "{}", "actor-owner", "");
    assert(missingIfMatch.statusCode == 428);
    assert(missingIfMatch.body.find(
        "\"code\":\"precondition_required\"") !=
        std::string::npos);

    const ApiResponse weakIfMatch = post(
        runtime,
        "{}",
        "actor-owner",
        "W/" + vdrsuite::http::publicStrongEntityTag("7"));
    assert(weakIfMatch.statusCode == 400);

    const ApiResponse wildcardIfMatch = post(
        runtime, "{}", "actor-owner", "*");
    assert(wildcardIfMatch.statusCode == 400);

    const ApiResponse listIfMatch = post(
        runtime,
        "{}",
        "actor-owner",
        vdrsuite::http::publicStrongEntityTag("7") +
            ", " + vdrsuite::http::publicStrongEntityTag("8"));
    assert(listIfMatch.statusCode == 400);

    const ApiResponse missingKey = post(
        runtime,
        "{}",
        "actor-owner",
        vdrsuite::http::publicStrongEntityTag("7"),
        "");
    assert(missingKey.statusCode == 400);

    const ApiResponse invalidKey = post(
        runtime,
        "{}",
        "actor-owner",
        vdrsuite::http::publicStrongEntityTag("7"),
        "contains space");
    assert(invalidKey.statusCode == 400);

    const ApiResponse invalidJson = post(runtime, "{");
    assert(invalidJson.statusCode == 400);
    assert(invalidJson.body.find(
        "\"code\":\"invalid_json\"") !=
        std::string::npos);

    const ApiResponse unknownField = post(
        runtime,
        "{\"nativeTimerId\":\"42\"}");
    assert(unknownField.statusCode == 422);
    assert(unknownField.body.find(
        "\"code\":\"validation_error\"") !=
        std::string::npos);

    const ApiResponse nonObject = post(runtime, "null");
    assert(nonObject.statusCode == 422);

    const ApiResponse wrongMediaType = post(
        runtime,
        "{}",
        "actor-owner",
        vdrsuite::http::publicStrongEntityTag("7"),
        "idem-public-create-1",
        "text/plain");
    assert(wrongMediaType.statusCode == 415);

    const ApiResponse anonymous = post(
        runtime,
        "{}",
        "",
        vdrsuite::http::publicStrongEntityTag("7"));
    assert(anonymous.statusCode == 401);

    assert(calls == beforeValidation);

    nextStatus = PublicTimerCreateAdmissionStatus::readOnlyBackend;
    const ApiResponse readOnly = post(runtime);
    assert(readOnly.statusCode == 403);
    assert(readOnly.body.find(
        "\"code\":\"read_only_backend\"") !=
        std::string::npos);

    nextStatus = PublicTimerCreateAdmissionStatus::revisionConflict;
    const ApiResponse stale = post(runtime);
    assert(stale.statusCode == 412);
    assert(stale.body.find(
        "\"code\":\"revision_conflict\"") !=
        std::string::npos);

    nextStatus = PublicTimerCreateAdmissionStatus::idempotencyConflict;
    const ApiResponse idemConflict = post(runtime);
    assert(idemConflict.statusCode == 409);
    assert(idemConflict.body.find(
        "\"code\":\"idempotency_conflict\"") !=
        std::string::npos);

    nextStatus = PublicTimerCreateAdmissionStatus::generationConflict;
    const ApiResponse generationConflict = post(runtime);
    assert(generationConflict.statusCode == 409);
    assert(generationConflict.body.find(
        "\"code\":\"generation_conflict\"") !=
        std::string::npos);

    nextStatus = PublicTimerCreateAdmissionStatus::stateConflict;
    const ApiResponse stateConflict = post(runtime);
    assert(stateConflict.statusCode == 409);
    assert(stateConflict.body.find(
        "\"code\":\"operation_conflict\"") !=
        std::string::npos);

    nextStatus = PublicTimerCreateAdmissionStatus::notFound;
    assert(post(runtime).statusCode == 404);

    nextStatus = PublicTimerCreateAdmissionStatus::backendUnavailable;
    const ApiResponse backendUnavailable = post(runtime);
    assert(backendUnavailable.statusCode == 503);
    assert(backendUnavailable.body.find(
        "\"code\":\"backend_unavailable\"") !=
        std::string::npos);

    nextStatus = PublicTimerCreateAdmissionStatus::serviceUnavailable;
    assert(post(runtime).statusCode == 503);

    runtime.resetTimerCreateAdmission();
    assert(!runtime.timerCreateAdmissionConfigured());
    return 0;
}
