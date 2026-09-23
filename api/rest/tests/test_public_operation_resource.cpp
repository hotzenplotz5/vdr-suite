#include "PublicApiRuntime.h"
#include "PublicResourcePreconditions.h"

#include <cassert>
#include <string>

namespace
{

PublicOperationLookupResult operationResult(
    const std::string& operationId,
    const std::string& actorRef)
{
    PublicOperationLookupResult result;

    if (operationId == "invalid")
    {
        result.status = PublicOperationLookupStatus::invalid;
        return result;
    }

    if (operationId == "unavailable")
    {
        result.status = PublicOperationLookupStatus::unavailable;
        return result;
    }

    if (operationId != "op-1" ||
        actorRef != "actor-owner")
    {
        result.status = PublicOperationLookupStatus::notFound;
        return result;
    }

    result.status = PublicOperationLookupStatus::ok;
    result.operation.operationId = "op-1";
    result.operation.state = "queued";
    result.operation.backendId = "default";
    result.operation.resourceRevision = "7";
    return result;
}

}

int main()
{
    PublicApiRuntime& runtime = PublicApiRuntime::instance();
    runtime.resetOperationLookup();

    ApiResponse unavailableCapabilities;
    assert(runtime.tryHandleGet(
        "/api/v1/capabilities",
        "actor-owner",
        "request-capability-1",
        "",
        unavailableCapabilities));
    assert(unavailableCapabilities.statusCode == 200);
    assert(unavailableCapabilities.body.find(
        "\"id\":\"public-api.durable-operations-read\"") !=
        std::string::npos);
    assert(unavailableCapabilities.body.find(
        "\"availability\":\"unavailable\"") !=
        std::string::npos);

    runtime.registerOperationLookup(operationResult);
    assert(runtime.operationLookupConfigured());

    ApiResponse availableCapabilities;
    assert(runtime.tryHandleGet(
        "/api/v1/capabilities",
        "actor-owner",
        "request-capability-2",
        "",
        availableCapabilities));
    assert(availableCapabilities.body.find(
        "\"id\":\"public-api.durable-operations-read\"") !=
        std::string::npos);
    assert(availableCapabilities.body.find(
        "\"availability\":\"available\"") !=
        std::string::npos);

    const std::string requestId = "request-operation-1";
    const std::string correlationId = "correlation-operation-1";

    ApiResponse read;
    assert(runtime.tryHandleGet(
        "/api/v1/operations/op-1?ignored=true",
        "actor-owner",
        requestId,
        correlationId,
        read));
    assert(read.statusCode == 200);
    assert(read.contentType == "application/json; charset=utf-8");
    assert(read.headers.at("Cache-Control") == "no-store");
    assert(read.headers.at("X-Content-Type-Options") == "nosniff");
    assert(read.headers.at("X-Request-ID") == requestId);
    assert(read.headers.at("X-Correlation-ID") == correlationId);

    const std::string expectedTag =
        vdrsuite::http::publicStrongEntityTag("7");
    assert(!expectedTag.empty());
    assert(read.headers.at("ETag") == expectedTag);

    assert(read.body.find(
        "\"operationId\":\"op-1\"") != std::string::npos);
    assert(read.body.find(
        "\"state\":\"queued\"") != std::string::npos);
    assert(read.body.find(
        "\"backendId\":\"default\"") != std::string::npos);
    assert(read.body.find(
        "\"self\":\"/api/v1/operations/op-1\"") !=
        std::string::npos);

    for (const std::string& forbidden :
         {std::string("idempotency"),
          std::string("fingerprint"),
          std::string("payload"),
          std::string("resultReference"),
          std::string("resourceRevision"),
          std::string("operationRevision")})
    {
        assert(read.body.find(forbidden) == std::string::npos);
    }

    ApiResponse notModified;
    assert(runtime.tryHandleGet(
        "/api/v1/operations/op-1",
        "actor-owner",
        requestId,
        correlationId,
        notModified,
        expectedTag));
    assert(notModified.statusCode == 304);
    assert(notModified.body.empty());
    assert(notModified.headers.at("ETag") == expectedTag);
    assert(notModified.headers.at("X-Request-ID") == requestId);

    ApiResponse weakNotModified;
    assert(runtime.tryHandleGet(
        "/api/v1/operations/op-1",
        "actor-owner",
        requestId,
        "",
        weakNotModified,
        "W/" + expectedTag));
    assert(weakNotModified.statusCode == 304);
    assert(weakNotModified.headers.at("ETag") == expectedTag);

    ApiResponse listNotModified;
    assert(runtime.tryHandleGet(
        "/api/v1/operations/op-1",
        "actor-owner",
        requestId,
        "",
        listNotModified,
        "\"other\", " + expectedTag));
    assert(listNotModified.statusCode == 304);

    ApiResponse changed;
    assert(runtime.tryHandleGet(
        "/api/v1/operations/op-1",
        "actor-owner",
        requestId,
        "",
        changed,
        "\"other\""));
    assert(changed.statusCode == 200);
    assert(changed.headers.at("ETag") == expectedTag);

    ApiResponse malformed;
    assert(runtime.tryHandleGet(
        "/api/v1/operations/op-1",
        "actor-owner",
        requestId,
        correlationId,
        malformed,
        "*, " + expectedTag));
    assert(malformed.statusCode == 400);
    assert(malformed.contentType == "application/problem+json");
    assert(malformed.body.find(
        "\"code\":\"invalid_request\"") != std::string::npos);

    ApiResponse anonymous;
    assert(runtime.tryHandleGet(
        "/api/v1/operations/op-1",
        "",
        requestId,
        "",
        anonymous));
    assert(anonymous.statusCode == 401);
    assert(anonymous.body.find(
        "\"code\":\"unauthorized\"") != std::string::npos);

    ApiResponse hidden;
    assert(runtime.tryHandleGet(
        "/api/v1/operations/op-1",
        "actor-other",
        requestId,
        "",
        hidden));
    assert(hidden.statusCode == 404);
    assert(hidden.body.find(
        "\"code\":\"not_found\"") != std::string::npos);

    ApiResponse missing;
    assert(runtime.tryHandleGet(
        "/api/v1/operations/missing",
        "actor-owner",
        requestId,
        "",
        missing));
    assert(missing.statusCode == 404);

    ApiResponse invalid;
    assert(runtime.tryHandleGet(
        "/api/v1/operations/invalid",
        "actor-owner",
        requestId,
        "",
        invalid));
    assert(invalid.statusCode == 400);

    ApiResponse unavailable;
    assert(runtime.tryHandleGet(
        "/api/v1/operations/unavailable",
        "actor-owner",
        requestId,
        "",
        unavailable));
    assert(unavailable.statusCode == 503);
    assert(unavailable.body.find(
        "\"code\":\"service_unavailable\"") != std::string::npos);

    ApiResponse post;
    assert(runtime.tryHandlePost(
        "/api/v1/operations/op-1",
        requestId,
        "",
        post));
    assert(post.statusCode == 405);
    assert(post.headers.at("Allow") == "GET");

    ApiResponse deleteResponse;
    assert(runtime.tryHandleUnsupportedMethod(
        "DELETE",
        "/api/v1/operations/op-1",
        requestId,
        "",
        deleteResponse));
    assert(deleteResponse.statusCode == 405);
    assert(deleteResponse.headers.at("Allow") == "GET");

    ApiResponse nested;
    assert(runtime.tryHandleGet(
        "/api/v1/operations/op-1/private",
        "actor-owner",
        requestId,
        "",
        nested));
    assert(nested.statusCode == 404);

    runtime.resetOperationLookup();
    assert(!runtime.operationLookupConfigured());

    ApiResponse unconfigured;
    assert(runtime.tryHandleGet(
        "/api/v1/operations/op-1",
        "actor-owner",
        requestId,
        "",
        unconfigured));
    assert(unconfigured.statusCode == 503);

    return 0;
}
