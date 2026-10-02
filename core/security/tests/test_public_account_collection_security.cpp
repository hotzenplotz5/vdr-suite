#include "SecurityHttpGateBrowserTestFixture.h"

#include <cassert>
#include <string>

namespace
{
constexpr const char* Permission = "accounts.view";
constexpr const char* Route = "/api/v1/accounts";

HttpServerRequest browserGet(
    SecurityHttpGateBrowserTestFixture& fixture,
    const std::string& route = Route)
{
    HttpServerRequest request;
    request.method = "GET";
    request.path = route;
    request.headers["X-Request-ID"] =
        "p2-public-account-collection";
    fixture.addBrowserAuthentication(request);
    return request;
}

HttpServerRequest browserCreate(
    SecurityHttpGateBrowserTestFixture& fixture,
    bool includeCsrf = true)
{
    HttpServerRequest request;
    request.method = "POST";
    request.path = "/api/v1/accounts";
    request.body =
        "{\"loginName\":\"new-viewer\","
        "\"password\":\"test-value-1\","
        "\"displayName\":\"New Viewer\"}";
    request.headers["Content-Type"] =
        "application/json";
    request.headers["Idempotency-Key"] =
        "idem-account-create-security-1";
    request.headers["X-Request-ID"] =
        "mu6d-public-account-create";
    fixture.addBrowserAuthentication(
        request,
        includeCsrf);
    return request;
}

HttpServerRequest browserPost(
    SecurityHttpGateBrowserTestFixture& fixture,
    const std::string& body,
    bool includeCsrf = true)
{
    HttpServerRequest request;
    request.method = "POST";
    request.path = "/api/v1/accounts/account-a";
    request.body = body;
    request.headers["Content-Type"] =
        "application/json";
    request.headers["If-Match"] =
        "\"vdr-suite-account:7\"";
    request.headers["X-Request-ID"] =
        "mu6c-public-account-mutation";
    fixture.addBrowserAuthentication(
        request,
        includeCsrf);
    return request;
}

bool hasDecisionEvent(
    const AccountabilityEventRepository& repository,
    const std::string& reasonCode,
    const std::string& permission = Permission)
{
    for (const AccountabilityEvent& event : repository.listAll())
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

        const SecurityGateDecision itemDecision =
            fixture.gate.evaluate(
                browserGet(fixture, "/api/v1/accounts/account-a"));
        assert(itemDecision.allowed);
        assert(!itemDecision.protectedMutation);
        assert(itemDecision.authorizationDecision.allowed);
        assert(itemDecision.authorizationDecision.permission == Permission);
        assert(itemDecision.authorizationDecision.backendId == "*");
        assert(itemDecision.authorizationDecision.action == "accounts.view");
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
            "\"code\":\"forbidden\"") != std::string::npos);
        assert(hasDecisionEvent(
            fixture.accountabilityRepository,
            "backend_scope_denied"));

        const SecurityGateDecision itemDecision =
            fixture.gate.evaluate(
                browserGet(fixture, "/api/v1/accounts/account-a"));
        assert(!itemDecision.allowed);
        assert(itemDecision.rejection.statusCode == 403);
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

        const SecurityGateDecision itemDecision =
            fixture.gate.evaluate(
                browserGet(fixture, "/api/v1/accounts/account-a"));
        assert(itemDecision.allowed);
        assert(itemDecision.authorizationDecision.reasonCode ==
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
        assert(decision.rejection.body.find(
            "\"code\":\"forbidden\"") != std::string::npos);
        assert(hasDecisionEvent(
            fixture.accountabilityRepository,
            "backend_scope_denied"));

        const SecurityGateDecision itemDecision =
            fixture.gate.evaluate(
                browserGet(fixture, "/api/v1/accounts/account-a"));
        assert(!itemDecision.allowed);
        assert(itemDecision.rejection.statusCode == 403);
    }

    {
        SecurityHttpGateBrowserTestFixture fixture;
        const SecurityGateDecision decision =
            fixture.gate.evaluate(browserGet(fixture));
        assert(!decision.allowed);
        assert(decision.rejection.statusCode == 403);
        assert(decision.rejection.body.find(
            "\"code\":\"forbidden\"") != std::string::npos);
        assert(hasDecisionEvent(
            fixture.accountabilityRepository,
            "permission_denied"));

        const SecurityGateDecision itemDecision =
            fixture.gate.evaluate(
                browserGet(fixture, "/api/v1/accounts/account-a"));
        assert(!itemDecision.allowed);
        assert(itemDecision.rejection.statusCode == 403);
    }

    {
        SecurityHttpGateBrowserTestFixture fixture;
        assert(fixture.grantRepository.ensureGrant(
            fixture.actorId,
            "accounts.create",
            "*"));

        const SecurityGateDecision decision =
            fixture.gate.evaluate(
                browserCreate(fixture));
        assert(decision.allowed);
        assert(decision.protectedMutation);
        assert(decision.authorizationDecision.permission ==
            "accounts.create");
        assert(decision.authorizationDecision.backendId == "*");
        assert(decision.authorizationDecision.action ==
            "accounts.create");
        assert(hasDecisionEvent(
            fixture.accountabilityRepository,
            "permission_granted",
            "accounts.create"));
    }

    {
        SecurityHttpGateBrowserTestFixture fixture;
        assert(fixture.grantRepository.ensureGrant(
            fixture.actorId,
            "role.admin",
            "*"));

        const SecurityGateDecision decision =
            fixture.gate.evaluate(
                browserCreate(fixture));
        assert(decision.allowed);
        assert(decision.protectedMutation);
        assert(decision.authorizationDecision.permission ==
            "accounts.create");
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
            fixture.gate.evaluate(
                browserCreate(fixture));
        assert(!decision.allowed);
        assert(decision.rejection.statusCode == 403);
        assert(hasDecisionEvent(
            fixture.accountabilityRepository,
            "backend_scope_denied",
            "accounts.create"));
    }

    {
        SecurityHttpGateBrowserTestFixture fixture;
        assert(fixture.grantRepository.ensureGrant(
            fixture.actorId,
            "accounts.create",
            "*"));

        const SecurityGateDecision decision =
            fixture.gate.evaluate(
                browserCreate(fixture, false));
        assert(!decision.allowed);
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

        const SecurityGateDecision decision =
            fixture.gate.evaluate(
                browserCreate(fixture));
        assert(!decision.allowed);
        assert(decision.rejection.statusCode == 403);
        assert(hasDecisionEvent(
            fixture.accountabilityRepository,
            "role_read_only",
            "accounts.create"));
    }

    {
        SecurityHttpGateBrowserTestFixture fixture;
        assert(fixture.grantRepository.ensureGrant(
            fixture.actorId,
            "accounts.modify",
            "*"));

        const SecurityGateDecision decision =
            fixture.gate.evaluate(
                browserPost(
                    fixture,
                    "{\"displayName\":\"Renamed Admin\"}"));
        assert(decision.allowed);
        assert(decision.protectedMutation);
        assert(decision.authorizationDecision.permission ==
            "accounts.modify");
        assert(decision.authorizationDecision.backendId == "*");
        assert(decision.authorizationDecision.action ==
            "accounts.modify");
        assert(hasDecisionEvent(
            fixture.accountabilityRepository,
            "permission_granted",
            "accounts.modify"));
    }

    {
        SecurityHttpGateBrowserTestFixture fixture;
        assert(fixture.grantRepository.ensureGrant(
            fixture.actorId,
            "accounts.activate",
            "*"));

        const SecurityGateDecision decision =
            fixture.gate.evaluate(
                browserPost(
                    fixture,
                    "{\"active\":true}"));
        assert(decision.allowed);
        assert(decision.protectedMutation);
        assert(decision.authorizationDecision.permission ==
            "accounts.activate");
        assert(decision.authorizationDecision.action ==
            "accounts.activate");
    }

    {
        SecurityHttpGateBrowserTestFixture fixture;
        assert(fixture.grantRepository.ensureGrant(
            fixture.actorId,
            "accounts.deactivate",
            "*"));

        const SecurityGateDecision decision =
            fixture.gate.evaluate(
                browserPost(
                    fixture,
                    "{\"active\":false}"));
        assert(decision.allowed);
        assert(decision.protectedMutation);
        assert(decision.authorizationDecision.permission ==
            "accounts.deactivate");
        assert(decision.authorizationDecision.action ==
            "accounts.deactivate");
    }

    {
        SecurityHttpGateBrowserTestFixture fixture;
        assert(fixture.grantRepository.ensureGrant(
            fixture.actorId,
            "role.admin",
            "*"));

        const SecurityGateDecision decision =
            fixture.gate.evaluate(
                browserPost(
                    fixture,
                    "{\"active\":false}"));
        assert(decision.allowed);
        assert(decision.authorizationDecision.reasonCode ==
            "role_permission_granted");
        assert(decision.authorizationDecision.permission ==
            "accounts.deactivate");
    }

    {
        SecurityHttpGateBrowserTestFixture fixture;
        assert(fixture.grantRepository.ensureGrant(
            fixture.actorId,
            "role.admin",
            "default"));

        const SecurityGateDecision decision =
            fixture.gate.evaluate(
                browserPost(
                    fixture,
                    "{\"displayName\":\"Renamed Admin\"}"));
        assert(!decision.allowed);
        assert(decision.rejection.statusCode == 403);
        assert(hasDecisionEvent(
            fixture.accountabilityRepository,
            "backend_scope_denied",
            "accounts.modify"));
    }

    {
        SecurityHttpGateBrowserTestFixture fixture;
        assert(fixture.grantRepository.ensureGrant(
            fixture.actorId,
            "accounts.modify",
            "*"));

        const SecurityGateDecision decision =
            fixture.gate.evaluate(
                browserPost(
                    fixture,
                    "{\"displayName\":\"Renamed Admin\"}",
                    false));
        assert(!decision.allowed);
        assert(decision.rejection.statusCode == 403);
        assert(decision.rejection.body.find(
            "\"code\":\"forbidden\"") !=
            std::string::npos);
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

        const SecurityGateDecision decision =
            fixture.gate.evaluate(
                browserPost(
                    fixture,
                    "{\"active\":false}"));
        assert(!decision.allowed);
        assert(decision.rejection.statusCode == 403);
        assert(hasDecisionEvent(
            fixture.accountabilityRepository,
            "role_read_only",
            "accounts.deactivate"));
    }

    {
        SecurityHttpGateBrowserTestFixture fixture;
        assert(fixture.grantRepository.ensureGrant(
            fixture.actorId,
            "role.admin",
            "*"));

        const SecurityGateDecision decision =
            fixture.gate.evaluate(
                browserPost(
                    fixture,
                    "{\"displayName\":\"Admin\",\"active\":false}"));
        assert(!decision.allowed);
        assert(decision.protectedMutation);
        assert(decision.rejection.statusCode == 400);
        assert(decision.rejection.body.find(
            "\"code\":\"invalid_request\"") !=
            std::string::npos);
    }

    {
        SecurityHttpGateBrowserTestFixture fixture;
        HttpServerRequest anonymousCreate;
        anonymousCreate.method = "POST";
        anonymousCreate.path = "/api/v1/accounts";
        anonymousCreate.body =
            "{\"loginName\":\"new-viewer\","
            "\"password\":\"test-value-1\","
            "\"displayName\":\"New Viewer\"}";
        const SecurityGateDecision createDecision =
            fixture.gate.evaluate(anonymousCreate);
        assert(!createDecision.allowed);
        assert(createDecision.rejection.statusCode == 401);
    }

    {
        SecurityHttpGateBrowserTestFixture fixture;
        HttpServerRequest anonymousMutation;
        anonymousMutation.method = "POST";
        anonymousMutation.path =
            "/api/v1/accounts/account-a";
        anonymousMutation.body =
            "{\"active\":true}";
        const SecurityGateDecision mutationDecision =
            fixture.gate.evaluate(
                anonymousMutation);
        assert(!mutationDecision.allowed);
        assert(mutationDecision.rejection.statusCode == 401);
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

        anonymous.path = "/api/v1/accounts/account-a";
        const SecurityGateDecision itemDecision =
            fixture.gate.evaluate(anonymous);
        assert(!itemDecision.allowed);
        assert(itemDecision.rejection.statusCode == 401);
    }

    return 0;
}
