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
            "backend_scope_denied") != std::string::npos ||
            decision.rejection.body.find(
                "permission_denied") != std::string::npos);
        assert(decision.authorizedBackendIds.empty());
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
                "invalid_backend_scope") != std::string::npos);
        }
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
