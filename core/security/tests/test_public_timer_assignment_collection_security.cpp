#include "SecurityHttpGateBrowserTestFixture.h"

#include <cassert>
#include <string>

namespace
{
constexpr const char* Permission = "timers.view";
constexpr const char* Route = "/api/v1/timer-assignments";

HttpServerRequest browserGet(
    SecurityHttpGateBrowserTestFixture& fixture,
    const std::string& target)
{
    HttpServerRequest request;
    request.method = "GET";
    request.path = target;
    request.headers["X-Request-ID"] =
        "phase69d-public-timer-assignment-collection";
    fixture.addBrowserAuthentication(request);
    return request;
}

bool hasDecisionEvent(
    const AccountabilityEventRepository& repository,
    const std::string& reasonCode,
    const std::string& backendId)
{
    for (const AccountabilityEvent& event : repository.listAll())
    {
        if (event.permission == Permission &&
            event.backendId == backendId &&
            event.reasonCode == reasonCode)
            return true;
    }
    return false;
}
}

int main()
{
    {
        SecurityHttpGateBrowserTestFixture fixture;
        assert(fixture.grantRepository.ensureGrant(
            fixture.actorId,
            Permission,
            "backend-one"));

        const SecurityGateDecision decision =
            fixture.gate.evaluate(
                browserGet(
                    fixture,
                    std::string(Route) +
                        "?backend=backend-one&limit=50"));
        assert(decision.allowed);
        assert(!decision.protectedMutation);
        assert(decision.authorizationDecision.allowed);
        assert(decision.authorizationDecision.permission ==
            Permission);
        assert(decision.authorizationDecision.backendId ==
            "backend-one");
        assert(decision.authorizationDecision.action ==
            "timers.view");
        assert(hasDecisionEvent(
            fixture.accountabilityRepository,
            "permission_granted",
            "backend-one"));
    }

    {
        SecurityHttpGateBrowserTestFixture fixture;
        assert(fixture.grantRepository.ensureGrant(
            fixture.actorId,
            Permission,
            "backend-one"));

        const SecurityGateDecision decision =
            fixture.gate.evaluate(
                browserGet(
                    fixture,
                    std::string(Route) +
                        "?backend=backend-two"));
        assert(!decision.allowed);
        assert(decision.rejection.statusCode == 403);
        assert(decision.rejection.body.find(
            "backend_scope_denied") !=
            std::string::npos);
    }

    {
        SecurityHttpGateBrowserTestFixture fixture;

        const SecurityGateDecision missing =
            fixture.gate.evaluate(
                browserGet(fixture, Route));
        assert(!missing.allowed);
        assert(missing.rejection.statusCode == 400);
        assert(missing.rejection.body.find(
            "invalid_backend_scope") !=
            std::string::npos);

        const SecurityGateDecision unsafe =
            fixture.gate.evaluate(
                browserGet(
                    fixture,
                    std::string(Route) +
                        "?backend=backend%2Fone"));
        assert(!unsafe.allowed);
        assert(unsafe.rejection.statusCode == 400);
    }

    {
        SecurityHttpGateBrowserTestFixture fixture;
        const SecurityGateDecision decision =
            fixture.gate.evaluate(
                browserGet(
                    fixture,
                    std::string(Route) +
                        "?backend=backend-one"));
        assert(!decision.allowed);
        assert(decision.rejection.statusCode == 403);
        assert(decision.rejection.body.find(
            "permission_denied") !=
            std::string::npos);
    }

    {
        SecurityHttpGateBrowserTestFixture fixture;
        HttpServerRequest anonymous;
        anonymous.method = "GET";
        anonymous.path =
            std::string(Route) +
            "?backend=backend-one";
        anonymous.headers["X-Request-ID"] =
            "phase69d-public-timer-assignment-collection-anonymous";
        const SecurityGateDecision decision =
            fixture.gate.evaluate(anonymous);
        assert(!decision.allowed);
        assert(decision.rejection.statusCode == 401);
    }

    {
        // Existing scoped Human grants authorize Public-v1 SearchTimer
        // reads without pretending a Browser is a paired Device.
        SecurityHttpGateBrowserTestFixture fixture;
        assert(fixture.grantRepository.ensureGrant(
            fixture.actorId, "searchtimers.view", "backend-one"));
        const std::string collection = "/api/v1/search-timers?backend=backend-one";
        const std::string item = "/api/v1/search-timers/st_00000000000000000000000000000001";
        const std::string backend = "?backend=backend-one";
        const std::string preview = item + "/preview" + backend;
        const std::string draft = "/api/v1/search-timers/draft-preview" + backend +
            "&query=Terra&mode=1&compareTitle=1&compareSubtitle=0&compareSummary=0";
        const auto read = [&](const std::string& uri) {
            return fixture.gate.evaluate(browserGet(fixture, uri));
        };
        for (const auto& uri : {collection, item + backend, item + "/version" + backend})
        {
            const auto allowed = read(uri);
            assert(allowed.allowed);
            assert(allowed.browserAuthenticated && !allowed.deviceAuthenticated);
            assert(allowed.authorizationDecision.permission == "searchtimers.view");
            assert(allowed.authorizationDecision.backendId == "backend-one");
        }
        const auto wrongScope = read("/api/v1/search-timers?backend=backend-two");
        assert(!wrongScope.allowed && wrongScope.rejection.statusCode == 403);
        assert(wrongScope.rejection.headers.at("Content-Type") == "application/problem+json");
        assert(wrongScope.rejection.body.find(R"json("code":"forbidden")json") != std::string::npos);
        assert(!read(preview).allowed);
        assert(!read(draft).allowed);
        assert(fixture.grantRepository.ensureGrant(
            fixture.actorId, "epg.view", "backend-one"));
        assert(read(preview).allowed);
        assert(read(draft).allowed);
    }
    {
        SecurityHttpGateBrowserTestFixture fixture;
        assert(fixture.grantRepository.ensureGrant(
            fixture.actorId, "role.admin", "backend-one"));
        const char* routes[] = {
            "/api/v1/search-timers?backend=backend-one",
            "/api/v1/search-timers/st_00000000000000000000000000000001"
            "?backend=backend-one",
            "/api/v1/search-timers/st_00000000000000000000000000000001/"
            "version?backend=backend-one",
            "/api/v1/search-timers/st_00000000000000000000000000000001/"
            "preview?backend=backend-one",
            "/api/v1/search-timers/draft-preview?backend=backend-one"
            "&query=Terra&mode=1&compareTitle=1&compareSubtitle=0"
            "&compareSummary=0"
        };
        for (const char* route : routes)
        {
            const auto allowed = fixture.gate.evaluate(browserGet(fixture, route));
            assert(allowed.allowed);
            assert(allowed.browserAuthenticated);
            assert(!allowed.deviceAuthenticated);
            assert(allowed.authorizationDecision.permission == "searchtimers.view");
            assert(allowed.authorizationDecision.backendId == "backend-one");
        }
        const auto wrongScope = fixture.gate.evaluate(browserGet(fixture,
            "/api/v1/search-timers?backend=backend-two"));
        assert(!wrongScope.allowed);
        assert(wrongScope.rejection.statusCode == 403);
    }
    return 0;
}
