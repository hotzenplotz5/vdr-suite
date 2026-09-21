#include "SecurityHttpGateBrowserTestFixture.h"

#include <cassert>
#include <string>

namespace
{
constexpr const char* Permission = "broadcast.teletext.view";
constexpr const char* ServiceRoute =
    "/api/vdr/broadcast/teletext/service";
constexpr const char* PageRoute =
    "/api/vdr/broadcast/teletext/page";

HttpServerRequest browserGet(
    SecurityHttpGateBrowserTestFixture& fixture,
    const std::string& target)
{
    HttpServerRequest request;
    request.method = "GET";
    request.path = target;
    request.headers["X-Request-ID"] = "phase67-teletext-read";
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
            "role.admin",
            "backend-b"));

        const SecurityGateDecision decision = fixture.gate.evaluate(
            browserGet(
                fixture,
                std::string(ServiceRoute) +
                    "?backend=backend-b&channel=C-1-1051-10301"));
        assert(decision.allowed);
        assert(!decision.protectedMutation);
        assert(decision.authorizationDecision.allowed);
        assert(decision.authorizationDecision.permission == Permission);
        assert(decision.authorizationDecision.backendId == "backend-b");
        assert(hasDecisionEvent(
            fixture.accountabilityRepository,
            "role_permission_granted",
            "backend-b"));
    }

    {
        SecurityHttpGateBrowserTestFixture fixture;
        assert(fixture.grantRepository.ensureGrant(
            fixture.actorId,
            Permission,
            "backend-a"));

        const SecurityGateDecision decision = fixture.gate.evaluate(
            browserGet(
                fixture,
                std::string(PageRoute) +
                    "?backend=backend-b&channel=C-1-1051-10301&page=100"));
        assert(!decision.allowed);
        assert(decision.rejection.statusCode == 403);
        assert(decision.rejection.body.find(
            "backend_scope_denied") != std::string::npos);
        assert(hasDecisionEvent(
            fixture.accountabilityRepository,
            "backend_scope_denied",
            "backend-b"));
    }

    {
        SecurityHttpGateBrowserTestFixture fixture;
        assert(fixture.grantRepository.ensureGrant(
            fixture.actorId,
            "role.read-only",
            "backend-b"));
        assert(fixture.grantRepository.ensureGrant(
            fixture.actorId,
            Permission,
            "backend-b"));

        const SecurityGateDecision decision = fixture.gate.evaluate(
            browserGet(
                fixture,
                std::string(PageRoute) +
                    "?backend=backend-b&channel=C-1-1051-10301&page=100"));
        assert(decision.allowed);
        assert(decision.authorizationDecision.permission == Permission);
        assert(decision.authorizationDecision.reasonCode ==
            "permission_granted");
    }

    {
        SecurityHttpGateBrowserTestFixture fixture;

        const SecurityGateDecision missingBackend = fixture.gate.evaluate(
            browserGet(
                fixture,
                std::string(ServiceRoute) +
                    "?channel=C-1-1051-10301"));
        assert(!missingBackend.allowed);
        assert(missingBackend.rejection.statusCode == 400);
        assert(missingBackend.rejection.body.find(
            "invalid_backend_scope") != std::string::npos);

        const SecurityGateDecision unsafeBackend = fixture.gate.evaluate(
            browserGet(
                fixture,
                std::string(ServiceRoute) +
                    "?backend=backend%2Fb&channel=C-1-1051-10301"));
        assert(!unsafeBackend.allowed);
        assert(unsafeBackend.rejection.statusCode == 400);
        assert(unsafeBackend.rejection.body.find(
            "invalid_backend_scope") != std::string::npos);
    }

    {
        SecurityHttpGateBrowserTestFixture fixture;

        const SecurityGateDecision decision = fixture.gate.evaluate(
            browserGet(
                fixture,
                std::string(ServiceRoute) +
                    "?backend=backend-b&channel=C-1-1051-10301"));
        assert(!decision.allowed);
        assert(decision.rejection.statusCode == 403);
        assert(decision.rejection.body.find(
            "permission_denied") != std::string::npos);
    }

    return 0;
}
