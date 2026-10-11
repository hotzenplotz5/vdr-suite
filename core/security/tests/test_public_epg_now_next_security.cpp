#include "SecurityHttpGateBrowserTestFixture.h"

#include <cassert>
#include <string>
#include <vector>

namespace
{
constexpr const char* Route =
    "/api/v1/epg/now-next?backendId=home&channelId=S19.2E-1-100&fromTime=1791565200";

HttpServerRequest browserRead(SecurityHttpGateBrowserTestFixture& fixture,
    const std::string& target)
{
    HttpServerRequest request;
    request.method = "GET";
    request.path = target;
    fixture.addBrowserAuthentication(request);
    return request;
}
}

int main()
{
    {
        SecurityHttpGateBrowserTestFixture fixture;
        assert(fixture.grantRepository.ensureGrant(
            fixture.actorId, "epg.view", "home"));
        const auto allowed = fixture.gate.evaluate(browserRead(fixture,Route));
        assert(allowed.allowed);
        assert(allowed.authorizedBackendIds ==
            std::vector<std::string>{"home"});
        assert(!allowed.protectedMutation);
    }
    {
        SecurityHttpGateBrowserTestFixture fixture;
        assert(fixture.grantRepository.ensureGrant(
            fixture.actorId, "channels.view", "home"));
        const auto denied = fixture.gate.evaluate(browserRead(fixture,Route));
        assert(!denied.allowed);
        assert(denied.rejection.statusCode == 403);
        assert(denied.authorizedBackendIds.empty());
    }
    {
        SecurityHttpGateBrowserTestFixture fixture;
        assert(fixture.grantRepository.ensureGrant(
            fixture.actorId,"epg.view","other"));
        const auto denied = fixture.gate.evaluate(browserRead(fixture,Route));
        assert(!denied.allowed);
        assert(denied.rejection.statusCode == 403);
    }
    {
        SecurityHttpGateBrowserTestFixture fixture;
        for(const std::string& path: {
            "/api/v1/epg/now-next",
            "/api/v1/epg/now-next?backendId=",
            "/api/v1/epg/now-next?backendId=../secret",
            "/api/v1/epg/now-next?backendId=home&backendId=home"})
        {
            const auto result = fixture.gate.evaluate(browserRead(fixture,path));
            assert(!result.allowed);
            assert(result.rejection.statusCode == 400);
        }
    }
    {
        SecurityHttpGateBrowserTestFixture fixture;
        HttpServerRequest anonymous;
        anonymous.method="GET";
        anonymous.path=Route;
        const auto denied = fixture.gate.evaluate(anonymous);
        assert(!denied.allowed);
        assert(denied.rejection.statusCode == 401);
    }
    return 0;
}
