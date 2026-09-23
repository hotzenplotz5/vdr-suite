#include "SecurityHttpGateBrowserTestFixture.h"

#include <cassert>
#include <string>

namespace
{
constexpr const char* Permission = "timers.create";
constexpr const char* Route =
    "/api/v1/timer-assignments/assignment:one?backend=backend-one";

HttpServerRequest browserPost(
    SecurityHttpGateBrowserTestFixture& fixture,
    const std::string& target)
{
    HttpServerRequest request;
    request.method = "POST";
    request.path = target;
    request.body = "{\"nativeTimer\":{}}";
    request.headers["X-Request-ID"] =
        "phase69c-public-timer-create";
    fixture.addBrowserAuthentication(request, true);
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
                browserPost(fixture, Route));
        assert(decision.allowed);
        assert(decision.protectedMutation);
        assert(decision.authorizationDecision.allowed);
        assert(decision.authorizationDecision.permission ==
            Permission);
        assert(decision.authorizationDecision.backendId ==
            "backend-one");
        assert(decision.authorizationDecision.action ==
            "timers.create");
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
                browserPost(
                    fixture,
                    "/api/v1/timer-assignments/assignment:one?backend=backend-two"));
        assert(!decision.allowed);
        assert(decision.rejection.statusCode == 403);
        assert(hasDecisionEvent(
            fixture.accountabilityRepository,
            "backend_scope_denied",
            "backend-two"));
    }

    {
        SecurityHttpGateBrowserTestFixture fixture;
        const SecurityGateDecision missing =
            fixture.gate.evaluate(
                browserPost(
                    fixture,
                    "/api/v1/timer-assignments/assignment:one"));
        assert(!missing.allowed);
        assert(missing.rejection.statusCode == 400);

        const SecurityGateDecision unsafe =
            fixture.gate.evaluate(
                browserPost(
                    fixture,
                    "/api/v1/timer-assignments/assignment:one?backend=backend%2Fone"));
        assert(!unsafe.allowed);
        assert(unsafe.rejection.statusCode == 400);
    }

    {
        SecurityHttpGateBrowserTestFixture fixture;
        const SecurityGateDecision denied =
            fixture.gate.evaluate(
                browserPost(fixture, Route));
        assert(!denied.allowed);
        assert(denied.rejection.statusCode == 403);
    }

    {
        SecurityHttpGateBrowserTestFixture fixture;
        HttpServerRequest anonymous;
        anonymous.method = "POST";
        anonymous.path = Route;
        anonymous.body = "{\"nativeTimer\":{}}";
        const SecurityGateDecision denied =
            fixture.gate.evaluate(anonymous);
        assert(!denied.allowed);
        assert(denied.rejection.statusCode == 401);
    }

    return 0;
}
