#include "SecurityHttpGateBrowserTestFixture.h"

#include <cassert>
#include <string>

namespace
{
constexpr const char* Permission = "accounts.view";
constexpr const char* Route = "/api/v1/accounts";

HttpServerRequest browserGet(
    SecurityHttpGateBrowserTestFixture& fixture)
{
    HttpServerRequest request;
    request.method = "GET";
    request.path = Route;
    request.headers["X-Request-ID"] =
        "p2-public-account-collection";
    fixture.addBrowserAuthentication(request);
    return request;
}

bool hasDecisionEvent(
    const AccountabilityEventRepository& repository,
    const std::string& reasonCode)
{
    for (const AccountabilityEvent& event : repository.listAll())
    {
        if (event.permission == Permission &&
            event.backendId == "*" &&
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
            "*"));

        const SecurityGateDecision decision =
            fixture.gate.evaluate(browserGet(fixture));
        assert(decision.allowed);
        assert(!decision.protectedMutation);
        assert(decision.authorizationDecision.allowed);
        assert(decision.authorizationDecision.permission == Permission);
        assert(decision.authorizationDecision.backendId == "*");
        assert(decision.authorizationDecision.action == "accounts.view");
        assert(hasDecisionEvent(
            fixture.accountabilityRepository,
            "permission_granted"));
    }

    {
        SecurityHttpGateBrowserTestFixture fixture;
        assert(fixture.grantRepository.ensureGrant(
            fixture.actorId,
            Permission,
            "default"));

        const SecurityGateDecision decision =
            fixture.gate.evaluate(browserGet(fixture));
        assert(!decision.allowed);
        assert(decision.rejection.statusCode == 403);
        assert(decision.rejection.body.find(
            "backend_scope_denied") != std::string::npos);
    }

    {
        SecurityHttpGateBrowserTestFixture fixture;
        assert(fixture.grantRepository.ensureGrant(
            fixture.actorId,
            "role.admin",
            "*"));

        const SecurityGateDecision decision =
            fixture.gate.evaluate(browserGet(fixture));
        assert(decision.allowed);
        assert(decision.authorizationDecision.reasonCode ==
            "role_permission_granted");
        assert(hasDecisionEvent(
            fixture.accountabilityRepository,
            "role_permission_granted"));
    }

    {
        SecurityHttpGateBrowserTestFixture fixture;
        assert(fixture.grantRepository.ensureGrant(
            fixture.actorId,
            "role.admin",
            "default"));

        const SecurityGateDecision decision =
            fixture.gate.evaluate(browserGet(fixture));
        assert(!decision.allowed);
        assert(decision.rejection.statusCode == 403);
        assert(decision.rejection.body.find(
            "backend_scope_denied") != std::string::npos);
    }

    {
        SecurityHttpGateBrowserTestFixture fixture;
        const SecurityGateDecision decision =
            fixture.gate.evaluate(browserGet(fixture));
        assert(!decision.allowed);
        assert(decision.rejection.statusCode == 403);
        assert(decision.rejection.body.find(
            "permission_denied") != std::string::npos);
    }

    {
        SecurityHttpGateBrowserTestFixture fixture;
        HttpServerRequest anonymous;
        anonymous.method = "GET";
        anonymous.path = Route;
        anonymous.headers["X-Request-ID"] =
            "p2-public-account-collection-anonymous";
        const SecurityGateDecision decision =
            fixture.gate.evaluate(anonymous);
        assert(!decision.allowed);
        assert(decision.rejection.statusCode == 401);
    }

    return 0;
}
