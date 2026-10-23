#include "SecurityHttpGateBrowserTestFixture.h"

#include <cassert>
#include <string>
#include <vector>

namespace
{
constexpr const char* Permission = "channels.view";
constexpr const char* Route = "/api/v1/channels";

HttpServerRequest browserGet(
    SecurityHttpGateBrowserTestFixture& fixture,
    const std::string& target)
{
    HttpServerRequest request;
    request.method = "GET";
    request.path = target;
    request.headers["X-Request-ID"] =
        "phase69d-public-channel-collection";
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
            fixture.actorId, Permission, "backend-a"));
        assert(fixture.grantRepository.ensureGrant(
            fixture.actorId, Permission, "backend-b"));

        const SecurityGateDecision decision =
            fixture.gate.evaluate(
                browserGet(
                    fixture,
                    std::string(Route) +
                        "?backendId=backend-b&backendId=backend-a&limit=50"));
        assert(decision.allowed);
        assert(!decision.protectedMutation);
        assert(decision.authorizedBackendIds ==
            (std::vector<std::string>{"backend-a", "backend-b"}));
        assert(hasDecisionEvent(
            fixture.accountabilityRepository,
            "permission_granted",
            "backend-a"));
        assert(hasDecisionEvent(
            fixture.accountabilityRepository,
            "permission_granted",
            "backend-b"));
    }

    {
        SecurityHttpGateBrowserTestFixture fixture;
        assert(fixture.grantRepository.ensureGrant(
            fixture.actorId, Permission, "backend-a"));

        const SecurityGateDecision decision =
            fixture.gate.evaluate(
                browserGet(
                    fixture,
                    std::string(Route) +
                        "?backendId=backend-a&backendId=backend-b"));
        assert(!decision.allowed);
        assert(decision.rejection.statusCode == 403);
        assert(decision.rejection.body.find(
            "\"code\":\"forbidden\"") != std::string::npos);
        assert(hasDecisionEvent(
            fixture.accountabilityRepository,
            "backend_scope_denied",
            "backend-b"));
        assert(decision.authorizedBackendIds.empty());
    }

    // Live TV on one backend includes its channel listing without a
    // separately stored channels.view grant.
    {
        SecurityHttpGateBrowserTestFixture fixture;
        assert(fixture.grantRepository.ensureGrant(
            fixture.actorId, "media.live.play", "backend-a"));
        const auto allowed = fixture.gate.evaluate(browserGet(
            fixture, std::string(Route) + "?backendId=backend-a"));
        assert(allowed.allowed);
        assert(allowed.authorizedBackendIds ==
            (std::vector<std::string>{"backend-a"}));
    }
    // A Live grant must never reveal a second, unauthorized backend.
    {
        SecurityHttpGateBrowserTestFixture fixture;
        assert(fixture.grantRepository.ensureGrant(
            fixture.actorId, "media.live.play", "backend-a"));
        const auto denied = fixture.gate.evaluate(browserGet(
            fixture, std::string(Route) +
                "?backendId=backend-a&backendId=backend-b"));
        assert(!denied.allowed);
        assert(denied.rejection.statusCode == 403);
        assert(denied.authorizedBackendIds.empty());
    }
    // Different grants may authorize different backends in one request;
    // neither implies playback permission on the other backend.
    {
        SecurityHttpGateBrowserTestFixture fixture;
        assert(fixture.grantRepository.ensureGrant(
            fixture.actorId, "media.live.play", "backend-a"));
        assert(fixture.grantRepository.ensureGrant(
            fixture.actorId, Permission, "backend-b"));
        const auto allowed = fixture.gate.evaluate(browserGet(
            fixture, std::string(Route) +
                "?backendId=backend-b&backendId=backend-a"));
        assert(allowed.allowed);
        assert(allowed.authorizedBackendIds ==
            (std::vector<std::string>{"backend-a", "backend-b"}));
    }

    {
        SecurityHttpGateBrowserTestFixture fixture;
        assert(fixture.grantRepository.ensureGrant(
            fixture.actorId, Permission, "*"));

        const SecurityGateDecision decision =
            fixture.gate.evaluate(
                browserGet(
                    fixture,
                    std::string(Route) +
                        "?backendId=backend-z&backendId=backend-a"));
        assert(decision.allowed);
        assert(decision.authorizedBackendIds ==
            (std::vector<std::string>{"backend-a", "backend-z"}));
    }

    {
        SecurityHttpGateBrowserTestFixture fixture;
        for (const std::string& target : {
                 std::string(Route),
                 std::string(Route) + "?backendId=",
                 std::string(Route) +
                     "?backendId=backend-a&backendId=backend-a",
                 std::string(Route) +
                     "?backendId=backend%2Funsafe"})
        {
            const SecurityGateDecision decision =
                fixture.gate.evaluate(browserGet(fixture, target));
            assert(!decision.allowed);
            assert(decision.rejection.statusCode == 400);
            assert(decision.rejection.body.find(
                "\"code\":\"invalid_request\"") != std::string::npos);
            assert(hasDecisionEvent(
                fixture.accountabilityRepository,
                "invalid_backend_scope",
                "*"));
        }
    }

    // A neighbor query is the SAME protected Public-v1 Channel resource,
    // not a bypass for unknown or other backend scope.
    {
        SecurityHttpGateBrowserTestFixture fixture;
        assert(fixture.grantRepository.ensureGrant(
            fixture.actorId, Permission, "backend-a"));
        const std::string neighbor =
            "?limit=1&relativeTo=S19.2E-1-1019-10301&direction=up";
        const SecurityGateDecision allowed = fixture.gate.evaluate(
            browserGet(fixture,
                std::string(Route) + "?backendId=backend-a&" +
                neighbor.substr(1U)));
        assert(allowed.allowed);
        assert(!allowed.protectedMutation);
        assert(allowed.authorizedBackendIds ==
            (std::vector<std::string>{"backend-a"}));

        const SecurityGateDecision denied = fixture.gate.evaluate(
            browserGet(fixture,
                std::string(Route) + "?backendId=backend-b&" +
                neighbor.substr(1U)));
        assert(!denied.allowed);
        assert(denied.rejection.statusCode == 403);
        assert(denied.authorizedBackendIds.empty());
    }

    {
        SecurityHttpGateBrowserTestFixture fixture;
        HttpServerRequest anonymous;
        anonymous.method = "GET";
        anonymous.path =
            std::string(Route) + "?backendId=backend-a";
        const SecurityGateDecision decision =
            fixture.gate.evaluate(anonymous);
        assert(!decision.allowed);
        assert(decision.rejection.statusCode == 401);
    }

    return 0;
}
