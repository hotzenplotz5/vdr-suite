#include "SecurityHttpGateBrowserTestFixture.h"

#include <cassert>
#include <string>

namespace
{
constexpr const char* Route =
    "/api/v1/accounts/account-a/sessions/session-a";

HttpServerRequest browserGet(
    SecurityHttpGateBrowserTestFixture& fixture)
{
    HttpServerRequest request;
    request.method = "GET";
    request.path = Route;
    request.headers["X-Request-ID"] =
        "mu8b-session-read";
    fixture.addBrowserAuthentication(request);
    return request;
}

HttpServerRequest browserPost(
    SecurityHttpGateBrowserTestFixture& fixture,
    bool includeCsrf = true)
{
    HttpServerRequest request;
    request.method = "POST";
    request.path = Route;
    request.body = "{}";
    request.headers["Content-Type"] =
        "application/json";
    request.headers["If-Match"] =
        "\"vsr-test\"";
    request.headers["X-Request-ID"] =
        "mu8b-session-revoke";
    fixture.addBrowserAuthentication(
        request,
        includeCsrf);
    return request;
}

bool hasDecisionEvent(
    const AccountabilityEventRepository& repository,
    const std::string& permission,
    const std::string& reasonCode)
{
    for (const AccountabilityEvent& event :
         repository.listAll())
    {
        if (event.permission == permission &&
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
            "accounts.sessions.view",
            "*"));

        const SecurityGateDecision decision =
            fixture.gate.evaluate(browserGet(fixture));
        assert(decision.allowed);
        assert(!decision.protectedMutation);
        assert(decision.authorizationDecision.permission ==
            "accounts.sessions.view");
        assert(decision.authorizationDecision.backendId ==
            "*");
    }

    {
        SecurityHttpGateBrowserTestFixture fixture;
        assert(fixture.grantRepository.ensureGrant(
            fixture.actorId,
            "accounts.sessions.revoke",
            "*"));

        const SecurityGateDecision decision =
            fixture.gate.evaluate(browserPost(fixture));
        assert(decision.allowed);
        assert(decision.protectedMutation);
        assert(decision.authorizationDecision.permission ==
            "accounts.sessions.revoke");
        assert(decision.authorizationDecision.backendId ==
            "*");
        assert(decision.authorizationDecision.action ==
            "accounts.sessions.revoke");
        assert(hasDecisionEvent(
            fixture.accountabilityRepository,
            "accounts.sessions.revoke",
            "permission_granted"));
    }

    {
        SecurityHttpGateBrowserTestFixture fixture;
        assert(fixture.grantRepository.ensureGrant(
            fixture.actorId,
            "role.admin",
            "*"));

        const SecurityGateDecision decision =
            fixture.gate.evaluate(browserPost(fixture));
        assert(decision.allowed);
        assert(decision.protectedMutation);
        assert(decision.authorizationDecision.reasonCode ==
            "role_permission_granted");
    }

    {
        SecurityHttpGateBrowserTestFixture fixture;
        assert(fixture.grantRepository.ensureGrant(
            fixture.actorId,
            "role.admin",
            "default"));

        const SecurityGateDecision decision =
            fixture.gate.evaluate(browserPost(fixture));
        assert(!decision.allowed);
        assert(decision.rejection.statusCode == 403);
        assert(hasDecisionEvent(
            fixture.accountabilityRepository,
            "accounts.sessions.revoke",
            "backend_scope_denied"));
    }

    {
        SecurityHttpGateBrowserTestFixture fixture;
        assert(fixture.grantRepository.ensureGrant(
            fixture.actorId,
            "accounts.sessions.revoke",
            "*"));

        const SecurityGateDecision decision =
            fixture.gate.evaluate(
                browserPost(fixture, false));
        assert(!decision.allowed);
        assert(decision.protectedMutation);
        assert(decision.rejection.statusCode == 403);
    }

    {
        SecurityHttpGateBrowserTestFixture fixture;
        assert(fixture.grantRepository.ensureGrant(
            fixture.actorId,
            "role.admin",
            "*"));
        assert(fixture.grantRepository.ensureGrant(
            fixture.actorId,
            "role.read-only",
            "*"));

        assert(fixture.gate.evaluate(
            browserGet(fixture)).allowed);

        const SecurityGateDecision decision =
            fixture.gate.evaluate(browserPost(fixture));
        assert(!decision.allowed);
        assert(decision.rejection.statusCode == 403);
        assert(hasDecisionEvent(
            fixture.accountabilityRepository,
            "accounts.sessions.revoke",
            "role_read_only"));
    }

    {
        SecurityHttpGateBrowserTestFixture fixture;
        HttpServerRequest anonymous;
        anonymous.method = "POST";
        anonymous.path = Route;
        anonymous.body = "{}";
        const SecurityGateDecision decision =
            fixture.gate.evaluate(anonymous);
        assert(!decision.allowed);
        assert(decision.rejection.statusCode == 401);
    }

    return 0;
}
