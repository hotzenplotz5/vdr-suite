#include "SecurityHttpGateBrowserTestFixture.h"

#include <cassert>
#include <string>

namespace
{
constexpr const char* Route =
    "/api/v1/accounts/account-a/grants";

HttpServerRequest browserGet(
    SecurityHttpGateBrowserTestFixture& fixture)
{
    HttpServerRequest request;
    request.method = "GET";
    request.path = Route;
    request.headers["X-Request-ID"] =
        "mu7-grant-read";
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
    request.body =
        "{\"permission\":\"channels.view\","
        "\"backendId\":\"default\","
        "\"active\":true}";
    request.headers["Content-Type"] =
        "application/json";
    request.headers["If-Match"] =
        "\"vdr-suite-grant-test\"";
    request.headers["X-Request-ID"] =
        "mu7-grant-mutation";
    fixture.addBrowserAuthentication(
        request,
        includeCsrf);
    return request;
}

bool hasDecisionEvent(
    const AccountabilityEventRepository& repository,
    const std::string& permission,
    const std::string& reasonCode,
    const std::string& backendId)
{
    for (const AccountabilityEvent& event :
         repository.listAll())
    {
        if (event.permission == permission &&
            event.reasonCode == reasonCode &&
            event.backendId == backendId)
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

        const SecurityGateDecision decision =
            fixture.gate.evaluate(browserGet(fixture));
        assert(decision.allowed);
        assert(!decision.protectedMutation);
        assert(decision.authorizationDecision.permission ==
            "accounts.grants.view");
        assert(decision.authorizationDecision.backendId ==
            "*");
        assert(hasDecisionEvent(
            fixture.accountabilityRepository,
            "accounts.grants.view",
            "permission_granted",
            "*"));
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
        assert(hasDecisionEvent(
            fixture.accountabilityRepository,
            "accounts.grants.view",
            "backend_scope_denied",
            "*"));
    }

    {
        SecurityHttpGateBrowserTestFixture fixture;
        assert(fixture.grantRepository.ensureGrant(
            fixture.actorId,
            "accounts.grants.modify",
            "*"));

        const SecurityGateDecision decision =
            fixture.gate.evaluate(browserPost(fixture));
        assert(decision.allowed);
        assert(decision.protectedMutation);
        assert(decision.authorizationDecision.permission ==
            "accounts.grants.modify");
        // The target backend in the JSON body is target data.
        // Administrator authority remains Suite-global.
        assert(decision.authorizationDecision.backendId ==
            "*");
        assert(decision.authorizationDecision.action ==
            "accounts.grants.modify");
        assert(hasDecisionEvent(
            fixture.accountabilityRepository,
            "accounts.grants.modify",
            "permission_granted",
            "*"));
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
            "accounts.grants.modify",
            "backend_scope_denied",
            "*"));
    }

    {
        SecurityHttpGateBrowserTestFixture fixture;
        assert(fixture.grantRepository.ensureGrant(
            fixture.actorId,
            "accounts.grants.modify",
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

        const SecurityGateDecision readDecision =
            fixture.gate.evaluate(browserGet(fixture));
        assert(readDecision.allowed);

        const SecurityGateDecision writeDecision =
            fixture.gate.evaluate(browserPost(fixture));
        assert(!writeDecision.allowed);
        assert(writeDecision.rejection.statusCode == 403);
        assert(hasDecisionEvent(
            fixture.accountabilityRepository,
            "accounts.grants.modify",
            "role_read_only",
            "*"));
    }

    {
        SecurityHttpGateBrowserTestFixture fixture;
        HttpServerRequest anonymousGet;
        anonymousGet.method = "GET";
        anonymousGet.path = Route;
        assert(!fixture.gate.evaluate(
            anonymousGet).allowed);
        assert(fixture.gate.evaluate(
            anonymousGet).rejection.statusCode == 401);

        HttpServerRequest anonymousPost;
        anonymousPost.method = "POST";
        anonymousPost.path = Route;
        anonymousPost.body =
            "{\"permission\":\"channels.view\","
            "\"backendId\":\"default\","
            "\"active\":true}";
        const SecurityGateDecision postDecision =
            fixture.gate.evaluate(anonymousPost);
        assert(!postDecision.allowed);
        assert(postDecision.rejection.statusCode == 401);
    }

    return 0;
}
