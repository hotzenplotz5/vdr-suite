#include "PublicApiRuntime.h"
#include "ServerBuildIdentity.h"

#include <cassert>
#include <string>

int main()
{
    PublicApiRuntime& runtime = PublicApiRuntime::instance();

    ApiResponse authenticatedRoot;
    assert(runtime.tryHandleGet(
        "/api/v1?ignored=true",
        "actor-test",
        authenticatedRoot));
    assert(authenticatedRoot.statusCode == 200);
    assert(authenticatedRoot.contentType == "application/json; charset=utf-8");
    assert(authenticatedRoot.headers.at("Cache-Control") == "no-store");
    assert(authenticatedRoot.headers.at("X-Content-Type-Options") == "nosniff");
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
    assert(runtime.tryHandleGet("/api/v1", "", anonymousRoot));
    assert(anonymousRoot.body.find(
        "\"authentication\":{\"authenticated\":false}") != std::string::npos);

    ApiResponse capabilities;
    assert(runtime.tryHandleGet(
        "/api/v1/capabilities",
        "actor-test",
        capabilities));
    assert(capabilities.statusCode == 200);
    assert(capabilities.body.find(
        "\"id\":\"public-api.contract-root\"") != std::string::npos);
    assert(capabilities.body.find(
        "\"version\":1") != std::string::npos);
    assert(capabilities.body.find(
        "\"availability\":\"available\"") != std::string::npos);
    assert(capabilities.body.find(
        "\"root\":\"/api/v1\"") != std::string::npos);

    ApiResponse unknown;
    unknown.statusCode = 418;
    assert(!runtime.tryHandleGet(
        "/api/v1/private",
        "actor-test",
        unknown));
    assert(unknown.statusCode == 418);

    return 0;
}
