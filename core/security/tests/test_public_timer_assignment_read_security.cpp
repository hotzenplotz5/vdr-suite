#include "SecurityHttpGateBrowserTestFixture.h"

#include <cassert>
#include <string>

namespace
{
constexpr const char* Permission = "timers.view";
constexpr const char* Route =
    "/api/v1/timer-assignments/assignment:one";

HttpServerRequest browserGet(
    SecurityHttpGateBrowserTestFixture& fixture,
    const std::string& target)
{
    HttpServerRequest request;
    request.method = "GET";
    request.path = target;
    request.headers["X-Request-ID"] =
        "phase69c-public-timer-assignment-read";
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
        {
            return true;
        }
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
                        "?backend=backend-one"));
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
        assert(hasDecisionEvent(
            fixture.accountabilityRepository,
            "backend_scope_denied",
            "backend-two"));
    }

    {
        SecurityHttpGateBrowserTestFixture fixture;

        const SecurityGateDecision missingBackend =
            fixture.gate.evaluate(
                browserGet(fixture, Route));
        assert(!missingBackend.allowed);
        assert(missingBackend.rejection.statusCode == 400);
        assert(missingBackend.rejection.body.find(
            "invalid_backend_scope") !=
            std::string::npos);

        const SecurityGateDecision unsafeBackend =
            fixture.gate.evaluate(
                browserGet(
                    fixture,
                    std::string(Route) +
                        "?backend=backend%2Fone"));
        assert(!unsafeBackend.allowed);
        assert(unsafeBackend.rejection.statusCode == 400);
        assert(unsafeBackend.rejection.body.find(
            "invalid_backend_scope") !=
            std::string::npos);
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
            "phase69c-public-timer-assignment-anonymous";

        const SecurityGateDecision decision =
            fixture.gate.evaluate(anonymous);
        assert(!decision.allowed);
        assert(decision.rejection.statusCode == 401);
    }

    return 0;
}
