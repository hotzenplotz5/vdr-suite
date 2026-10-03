#include "SecurityHttpGateBrowserTestFixture.h"

#include <cassert>
#include <string>

namespace
{
HttpServerRequest browserGet(
    const SecurityHttpGateBrowserTestFixture& fixture,
    const std::string& suffix)
{
    HttpServerRequest request;
    request.method = "GET";
    request.path = "/api/v1/accounts/account-a/" + suffix;
    request.headers["X-Request-ID"] = "mu8a-" + suffix;
    fixture.addBrowserAuthentication(request);
    return request;
}
}

int main()
{
    {
        SecurityHttpGateBrowserTestFixture fixture;
        assert(fixture.grantRepository.ensureGrant(
            fixture.actorId, "accounts.credentials.view", "*"));
        const auto decision =
            fixture.gate.evaluate(browserGet(fixture, "credentials"));
        assert(decision.allowed);
        assert(!decision.protectedMutation);
        assert(decision.authorizationDecision.permission ==
            "accounts.credentials.view");
        assert(decision.authorizationDecision.backendId == "*");
    }

    {
        SecurityHttpGateBrowserTestFixture fixture;
        assert(fixture.grantRepository.ensureGrant(
            fixture.actorId, "accounts.sessions.view", "*"));
        const auto decision =
            fixture.gate.evaluate(browserGet(fixture, "sessions"));
        assert(decision.allowed);
        assert(decision.authorizationDecision.permission ==
            "accounts.sessions.view");
    }

    {
        SecurityHttpGateBrowserTestFixture fixture;
        assert(fixture.grantRepository.ensureGrant(
            fixture.actorId, "role.admin", "*"));
        assert(fixture.gate.evaluate(
            browserGet(fixture, "credentials")).allowed);
        assert(fixture.gate.evaluate(
            browserGet(fixture, "sessions")).allowed);
    }

    {
        SecurityHttpGateBrowserTestFixture fixture;
        assert(fixture.grantRepository.ensureGrant(
            fixture.actorId, "role.admin", "default"));
        const auto decision =
            fixture.gate.evaluate(browserGet(fixture, "credentials"));
        assert(!decision.allowed);
        assert(decision.rejection.statusCode == 403);
        assert(decision.authorizationDecision.reasonCode ==
            "backend_scope_denied");
    }

    return 0;
}
