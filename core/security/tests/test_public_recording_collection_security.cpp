#include "SecurityHttpGateBrowserTestFixture.h"

#include <cassert>
#include <string>
#include <vector>

namespace
{
constexpr const char* Permission = "recordings.view";
constexpr const char* Route = "/api/v1/recordings";

HttpServerRequest requestFor(
    SecurityHttpGateBrowserTestFixture& fixture,
    const std::string& target)
{
    HttpServerRequest request;
    request.method = "GET";
    request.path = target;
    request.headers["X-Request-ID"] = "vidaa-r2-recording-security";
    fixture.addBrowserAuthentication(request);
    return request;
}
}

int main()
{
    {
        SecurityHttpGateBrowserTestFixture fixture;
        assert(fixture.grantRepository.ensureGrant(
            fixture.actorId, Permission, "backend-a"));
        const auto result = fixture.gate.evaluate(
            requestFor(fixture,
                std::string(Route) + "?backendId=backend-a"));
        assert(result.allowed);
        assert(result.authorizedBackendIds ==
            std::vector<std::string>{"backend-a"});
        assert(result.authorizationDecision.permission == Permission);
        assert(result.authorizationDecision.backendId == "backend-a");
    }
    {
        SecurityHttpGateBrowserTestFixture fixture;
        assert(fixture.grantRepository.ensureGrant(
            fixture.actorId, Permission, "backend-a"));
        const auto result = fixture.gate.evaluate(
            requestFor(fixture,
                std::string(Route) + "?backendId=backend-b"));
        assert(!result.allowed);
        assert(result.rejection.statusCode == 403);
        assert(result.authorizedBackendIds.empty());
    }
    for (const std::string target : {
         std::string(Route),
         std::string(Route) + "?backendId=",
         std::string(Route) + "?backendId=bad%2Fpath",
         std::string(Route) + "?backendId=backend-a&backendId=backend-a"})
    {
        SecurityHttpGateBrowserTestFixture fixture;
        const auto result = fixture.gate.evaluate(
            requestFor(fixture, target));
        assert(!result.allowed);
        assert(result.rejection.statusCode == 400);
    }
    {
        SecurityHttpGateBrowserTestFixture fixture;
        HttpServerRequest anonymous;
        anonymous.method = "GET";
        anonymous.path = std::string(Route) + "?backendId=backend-a";
        const auto result = fixture.gate.evaluate(anonymous);
        assert(!result.allowed);
        assert(result.rejection.statusCode == 401);
    }
}
