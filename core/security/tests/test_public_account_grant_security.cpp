#include "SecurityHttpGateBrowserTestFixture.h"

#include <cassert>
#include <string>

namespace
{
HttpServerRequest grantRead(
    SecurityHttpGateBrowserTestFixture& fixture)
{
    HttpServerRequest request;
    request.method = "GET";
    request.path =
        "/api/v1/accounts/account-a/grants";
    request.headers["X-Request-ID"] =
        "mu7-account-grants-read";
    fixture.addBrowserAuthentication(request);
    return request;
}

HttpServerRequest grantMutation(
    SecurityHttpGateBrowserTestFixture& fixture,
    bool includeCsrf = true)
{
    HttpServerRequest request;
    request.method = "POST";
    request.path =
        "/api/v1/accounts/account-a/grants";
    request.body =
        "{\"permission\":\"channels.view\",\"backendId\":\"default\",\"active\":true}";
    request.headers["Content-Type"] =
        "application/json";
    request.headers["If-Match"] =
        "\"vsr-6163636f756e742d6772616e74732d76317c\"";
    request.headers["X-Request-ID"] =
        "mu7-account-grants-mutation";
    fixture.addBrowserAuthentication(
        request,
        includeCsrf);
    return request;
}

bool eventFor(
    const AccountabilityEventRepository& repository,
    const std::string& permission,
    const std::string& reason)
{
    for (const AccountabilityEvent& event :
         repository.listAll())
    {
        if (event.permission == permission &&
            event.backendId == "*" &&
            event.reasonCode == reason)
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
            "accounts.grants.view",
            "*"));
        const auto decision =
            fixture.gate.evaluate(grantRead(fixture));
        assert(decision.allowed);
        assert(!decision.protectedMutation);
        assert(decision.authorizationDecision.permission ==
            "accounts.grants.view");
        assert(decision.authorizationDecision.backendId == "*");
        assert(eventFor(
            fixture.accountabilityRepository,
            "accounts.grants.view",
            "permission_granted"));
    }

    {
        SecurityHttpGateBrowserTestFixture fixture;
        assert(fixture.grantRepository.ensureGrant(
            fixture.actorId,
            "accounts.grants.view",
            "default"));
        const auto decision =
            fixture.gate.evaluate(grantRead(fixture));
        assert(!decision.allowed);
        assert(decision.rejection.statusCode == 403);
        assert(eventFor(
            fixture.accountabilityRepository,
            "accounts.grants.view",
            "backend_scope_denied"));
    }

    {
        SecurityHttpGateBrowserTestFixture fixture;
        assert(fixture.grantRepository.ensureGrant(
            fixture.actorId,
            "role.admin",
            "*"));
        const auto readDecision =
            fixture.gate.evaluate(grantRead(fixture));
        assert(readDecision.allowed);
        assert(readDecision.authorizationDecision.reasonCode ==
            "role_permission_granted");

        const auto mutationDecision =
            fixture.gate.evaluate(grantMutation(fixture));
        assert(mutationDecision.allowed);
        assert(mutationDecision.protectedMutation);
        assert(mutationDecision.authorizationDecision.permission ==
            "accounts.grants.modify");
        assert(mutationDecision.authorizationDecision.backendId == "*");
        assert(mutationDecision.authorizationDecision.reasonCode ==
            "role_permission_granted");
    }

    {
        SecurityHttpGateBrowserTestFixture fixture;
        assert(fixture.grantRepository.ensureGrant(
            fixture.actorId,
            "role.admin",
            "default"));
        assert(!fixture.gate.evaluate(
            grantRead(fixture)).allowed);
        const auto mutationDecision =
            fixture.gate.evaluate(grantMutation(fixture));
        assert(!mutationDecision.allowed);
        assert(mutationDecision.rejection.statusCode == 403);
    }

    {
        SecurityHttpGateBrowserTestFixture fixture;
        assert(fixture.grantRepository.ensureGrant(
            fixture.actorId,
            "accounts.grants.modify",
            "*"));
        const auto decision =
            fixture.gate.evaluate(grantMutation(fixture));
        assert(decision.allowed);
        assert(decision.protectedMutation);
        assert(eventFor(
            fixture.accountabilityRepository,
            "accounts.grants.modify",
            "permission_granted"));
    }

    {
        SecurityHttpGateBrowserTestFixture fixture;
        assert(fixture.grantRepository.ensureGrant(
            fixture.actorId,
            "accounts.grants.modify",
            "*"));
        const auto decision =
            fixture.gate.evaluate(
                grantMutation(fixture, false));
        assert(!decision.allowed);
        assert(decision.rejection.statusCode == 403);
        assert(decision.rejection.body.find(
            "csrf") != std::string::npos);
    }

    return 0;
}
