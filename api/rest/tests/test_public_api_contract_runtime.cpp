#include "PublicApiRuntime.h"
#include "ServerBuildIdentity.h"

#include <cassert>
#include <string>

int main()
{
    PublicApiRuntime& runtime = PublicApiRuntime::instance();

    const std::string requestId = "phase69b-request-001";
    const std::string correlationId = "phase69b-correlation-001";

    ApiResponse authenticatedRoot;
    assert(runtime.tryHandleGet(
        "/api/v1?ignored=true",
        "actor-test",
        requestId,
        correlationId,
        authenticatedRoot));
    assert(authenticatedRoot.statusCode == 200);
    assert(authenticatedRoot.contentType == "application/json; charset=utf-8");
    assert(authenticatedRoot.headers.at("Cache-Control") == "no-store");
    assert(authenticatedRoot.headers.at("X-Content-Type-Options") == "nosniff");
    assert(authenticatedRoot.headers.at("X-Request-ID") == requestId);
    assert(authenticatedRoot.headers.at("X-Correlation-ID") == correlationId);
    assert(authenticatedRoot.body.find("\"apiVersion\":\"v1\"") != std::string::npos);
    assert(authenticatedRoot.body.find(
        std::string("\"serverVersion\":\"")
        + VdrSuiteServerBuildIdentity::ServerVersion + "\"") != std::string::npos);
    assert(authenticatedRoot.body.find(
        "\"supportedApiMajors\":[\"v1\"]") != std::string::npos);
    assert(authenticatedRoot.body.find(
        "\"authentication\":{\"authenticated\":true}") != std::string::npos);
    assert(authenticatedRoot.body.find(
        "\"self\":\"/api/v1\"") != std::string::npos);
    assert(authenticatedRoot.body.find(
        "\"capabilities\":\"/api/v1/capabilities\"") != std::string::npos);

    ApiResponse anonymousRoot;
    assert(runtime.tryHandleGet(
        "/api/v1",
        "",
        "phase69b-request-anonymous",
        "",
        anonymousRoot));
    assert(anonymousRoot.body.find(
        "\"authentication\":{\"authenticated\":false}") != std::string::npos);
    assert(anonymousRoot.headers.at("X-Request-ID") ==
        "phase69b-request-anonymous");
    assert(anonymousRoot.headers.find("X-Correlation-ID") ==
        anonymousRoot.headers.end());

    ApiResponse capabilities;
    assert(runtime.tryHandleGet(
        "/api/v1/capabilities",
        "actor-test",
        requestId,
        correlationId,
        capabilities));
    assert(capabilities.statusCode == 200);
    assert(capabilities.headers.at("X-Request-ID") == requestId);
    assert(capabilities.body.find(
        "\"id\":\"public-api.contract-root\"") != std::string::npos);
    assert(capabilities.body.find(
        "\"version\":1") != std::string::npos);
    assert(capabilities.body.find(
        "\"availability\":\"available\"") != std::string::npos);
    assert(capabilities.body.find(
        "\"root\":\"/api/v1\"") != std::string::npos);

    ApiResponse missing;
    missing.statusCode = 418;
    assert(runtime.tryHandleGet(
        "/api/v1/private?ignored=true",
        "actor-test",
        requestId,
        correlationId,
        missing));
    assert(missing.statusCode == 404);
    assert(missing.contentType == "application/problem+json");
    assert(missing.headers.at("X-Request-ID") == requestId);
    assert(missing.headers.at("X-Correlation-ID") == correlationId);
    assert(missing.body.find(
        "\"type\":\"urn:vdr-suite:error:not-found\"") != std::string::npos);
    assert(missing.body.find("\"status\":404") != std::string::npos);
    assert(missing.body.find("\"code\":\"not_found\"") != std::string::npos);
    assert(missing.body.find(
        "\"requestId\":\"phase69b-request-001\"") != std::string::npos);
    assert(missing.body.find(
        "\"correlationId\":\"phase69b-correlation-001\"") != std::string::npos);
    assert(missing.body.find(
        "\"instance\":\"/api/v1/private\"") != std::string::npos);

    ApiResponse rootPost;
    assert(runtime.tryHandlePost(
        "/api/v1",
        requestId,
        correlationId,
        rootPost));
    assert(rootPost.statusCode == 405);
    assert(rootPost.contentType == "application/problem+json");
    assert(rootPost.headers.at("Allow") == "GET");
    assert(rootPost.headers.at("X-Request-ID") == requestId);
    assert(rootPost.body.find(
        "\"type\":\"urn:vdr-suite:error:method-not-allowed\"") !=
        std::string::npos);
    assert(rootPost.body.find(
        "\"code\":\"method_not_allowed\"") != std::string::npos);

    ApiResponse unknownPost;
    unknownPost.statusCode = 418;
    assert(!runtime.tryHandlePost(
        "/api/v1/private",
        requestId,
        "",
        unknownPost));
    assert(unknownPost.statusCode == 418);

    ApiResponse legacyGet;
    legacyGet.statusCode = 418;
    assert(!runtime.tryHandleGet(
        "/api/vdr/status",
        "actor-test",
        requestId,
        correlationId,
        legacyGet));
    assert(legacyGet.statusCode == 418);

    ApiResponse futureMajor;
    futureMajor.statusCode = 418;
    assert(!runtime.tryHandleGet(
        "/api/v10",
        "actor-test",
        requestId,
        correlationId,
        futureMajor));
    assert(futureMajor.statusCode == 418);

    ApiResponse legacyPost;
    legacyPost.statusCode = 418;
    assert(!runtime.tryHandlePost(
        "/api/vdr/timers/actions/create",
        requestId,
        correlationId,
        legacyPost));
    assert(legacyPost.statusCode == 418);

    return 0;
}
