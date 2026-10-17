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
    // Public-v1 EPG windows additionally expose channel contents.
    // An epg.view grant alone must not expose the programme timeline.
    {
        SecurityHttpGateBrowserTestFixture fixture;
        assert(fixture.grantRepository.ensureGrant(
            fixture.actorId, "epg.view", "home"));
        const std::string window =
            "/api/v1/epg/window?backendId=home&channelId=S19.2E-1-100"
            "&fromTime=1791565200&untilTime=1791586800";
        const auto denied = fixture.gate.evaluate(browserRead(fixture,window));
        assert(!denied.allowed);
        assert(denied.rejection.statusCode == 403);
        assert(fixture.grantRepository.ensureGrant(
            fixture.actorId, "channels.view", "home"));
        const auto allowed = fixture.gate.evaluate(browserRead(fixture,window));
        assert(allowed.allowed);
        assert(allowed.authorizedBackendIds ==
            std::vector<std::string>{"home"});
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
    // Native timer listings are separate from TimerAssignments, but require
    // the same exact backend-scoped timers.view grant.
    {
        SecurityHttpGateBrowserTestFixture fixture;
        const std::string route =
            "/api/v1/native-timers?backendId=home&limit=30&offset=0";
        const auto denied = fixture.gate.evaluate(browserRead(fixture,route));
        assert(!denied.allowed);
        assert(denied.rejection.statusCode == 403);
        assert(fixture.grantRepository.ensureGrant(
            fixture.actorId,"timers.view","home"));
        const auto allowed = fixture.gate.evaluate(browserRead(fixture,route));
        assert(allowed.allowed);
        assert(allowed.authorizedBackendIds ==
            std::vector<std::string>{"home"});
        const auto invalid = fixture.gate.evaluate(browserRead(
            fixture,"/api/v1/native-timers?backendId=home&backendId=home"));
        assert(!invalid.allowed);
        assert(invalid.rejection.statusCode == 400);
    }
    return 0;
}
