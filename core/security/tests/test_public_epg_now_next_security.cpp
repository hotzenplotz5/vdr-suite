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
        for (const std::string& path : {
            "/api/v1/genres/epg?backendId=home&limit=30",
            "/api/v1/genres/epg/events?backendId=home&contentClass=movie"})
        {
            const auto genre = fixture.gate.evaluate(browserRead(fixture,path));
            assert(genre.allowed);
            assert(genre.authorizedBackendIds == std::vector<std::string>{"home"});
            assert(!genre.protectedMutation);
        }
    }
    // M2 EPG image bytes follow the strict EPG detail grant, never a
    // different backend or live-playback-only grant.
    const std::string artwork =
        "/api/v1/epg/artwork?backendId=home&channelId=S19.2E-1-100&eventId=81234";
    {
        SecurityHttpGateBrowserTestFixture fixture;
        assert(fixture.grantRepository.ensureGrant(
            fixture.actorId, "epg.view", "home"));
        const auto allowed = fixture.gate.evaluate(browserRead(fixture, artwork));
        assert(allowed.allowed);
        assert(allowed.authorizedBackendIds ==
            std::vector<std::string>{"home"});
    }
    for (const auto& permission : {"media.live.play", "channels.view"}) {
        SecurityHttpGateBrowserTestFixture fixture;
        assert(fixture.grantRepository.ensureGrant(
            fixture.actorId, permission, "home"));
        const auto denied = fixture.gate.evaluate(browserRead(fixture, artwork));
        assert(!denied.allowed);
        assert(denied.rejection.statusCode == 403);
    }
    {
        SecurityHttpGateBrowserTestFixture fixture;
        assert(fixture.grantRepository.ensureGrant(
            fixture.actorId, "epg.view", "other"));
        const auto denied = fixture.gate.evaluate(browserRead(fixture, artwork));
        assert(!denied.allowed);
        assert(denied.rejection.statusCode == 403);
    }

    // A Live-TV permission includes the public TV guide on the SAME
    // backend. EPG-only actors remain legal; channel list alone is not.
    const std::string Detail =
        "/api/v1/epg/event?backendId=home&channelId=S19.2E-1-100&eventId=81234";
    const std::string Window =
        "/api/v1/epg/window?backendId=home&channelId=S19.2E-1-100"
        "&fromTime=1791565200&untilTime=1791586800";
    {
        SecurityHttpGateBrowserTestFixture fixture;
        assert(fixture.grantRepository.ensureGrant(
            fixture.actorId, "epg.view", "home"));
        const auto allowed = fixture.gate.evaluate(browserRead(fixture,Window));
        assert(allowed.allowed);
        assert(allowed.authorizedBackendIds ==
            std::vector<std::string>{"home"});
    }
    {
        SecurityHttpGateBrowserTestFixture fixture;
        assert(fixture.grantRepository.ensureGrant(
            fixture.actorId, "media.live.play", "home"));
        for (const std::string& path: {std::string(Route), Window})
        {
            const auto allowed = fixture.gate.evaluate(browserRead(fixture,path));
            assert(allowed.allowed);
            assert(allowed.authorizedBackendIds ==
                std::vector<std::string>{"home"});
        }
        // Detailed EPG event identity/revision needs the independent epg.view
        // grant; a Live-playback permission must not reveal those facts.
        const auto detailDenied = fixture.gate.evaluate(browserRead(fixture,Detail));
        assert(!detailDenied.allowed);
        assert(detailDenied.rejection.statusCode == 403);
    }
    {
        SecurityHttpGateBrowserTestFixture fixture;
        assert(fixture.grantRepository.ensureGrant(
            fixture.actorId, "media.live.play", "other"));
        for (const std::string& path: {std::string(Route), Window, Detail})
        {
            const auto denied = fixture.gate.evaluate(browserRead(fixture,path));
            assert(!denied.allowed);
            assert(denied.rejection.statusCode == 403);
        }
    }
    {
        SecurityHttpGateBrowserTestFixture fixture;
        assert(fixture.grantRepository.ensureGrant(
            fixture.actorId, "channels.view", "home"));
        const auto denied = fixture.gate.evaluate(browserRead(fixture,Route));
        assert(!denied.allowed);
        assert(denied.rejection.statusCode == 403);
        assert(denied.authorizedBackendIds.empty());
        for (const std::string& path : {
            "/api/v1/genres/epg?backendId=home",
            "/api/v1/genres/epg/events?backendId=home&contentClass=movie"})
        {
            const auto noEpgGrant = fixture.gate.evaluate(browserRead(fixture,path));
            assert(!noEpgGrant.allowed);
            assert(noEpgGrant.rejection.statusCode == 403);
        }
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
    // Exact native event detail shares the EPG grant; channel-only access
    // must never expose the source ID or strong resource revision.
    {
        SecurityHttpGateBrowserTestFixture fixture;
        assert(fixture.grantRepository.ensureGrant(
            fixture.actorId, "epg.view", "home"));
        const auto allowed = fixture.gate.evaluate(browserRead(fixture, Detail));
        assert(allowed.allowed && !allowed.protectedMutation);
        assert(allowed.authorizedBackendIds ==
            std::vector<std::string>{"home"});
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
