#include "SecurityHttpGateBrowserTestFixture.h"

#include <cassert>
#include <string>
#include <vector>

namespace
{
const std::vector<std::string> kRoutes = {
    "/api/searchtimers/execute",
    "/api/vdr/searchtimers/execute",
    "/api/searchtimers/real-test",
    "/api/vdr/searchtimers/real-test"
};
}

int main()
{
    SecurityHttpGateBrowserTestFixture fixture;

    for (const std::string& route : kRoutes)
    {
        HttpServerRequest legacy =
            fixture.mutationRequest(
                route,
                "default");
        fixture.addLegacyAuthentication(legacy);

        const SecurityGateDecision legacyDecision =
            fixture.gate.evaluate(legacy);
        assert(legacyDecision.allowed);
        assert(legacyDecision.protectedMutation);

        HttpServerRequest missingCsrf =
            fixture.mutationRequest(
                route,
                "default");
        fixture.addBrowserAuthentication(
            missingCsrf);

        const SecurityGateDecision missingCsrfDecision =
            fixture.gate.evaluate(missingCsrf);
        assert(!missingCsrfDecision.allowed);
        assert(missingCsrfDecision.protectedMutation);
        assert(
            missingCsrfDecision.rejection.statusCode ==
            403);
        assert(missingCsrfDecision.rejection.body.find(
            "csrf_validation_failed") !=
            std::string::npos);

        HttpServerRequest noPermission =
            fixture.mutationRequest(
                route,
                "default");
        fixture.addBrowserAuthentication(
            noPermission,
            true);

        const SecurityGateDecision noPermissionDecision =
            fixture.gate.evaluate(noPermission);
        assert(!noPermissionDecision.allowed);
        assert(
            noPermissionDecision.rejection.statusCode ==
            403);
        assert(noPermissionDecision.rejection.body.find(
            "permission_denied") !=
            std::string::npos);

        assert(fixture.grantRepository.ensureGrant(
            fixture.actorId,
            "searchtimers.execute",
            "default"));

        const SecurityGateDecision directAllowed =
            fixture.gate.evaluate(noPermission);
        assert(directAllowed.allowed);
        assert(directAllowed.protectedMutation);

        HttpServerRequest defaultBackend =
            fixture.mutationRequest(
                route,
                "",
                false);
        fixture.addBrowserAuthentication(
            defaultBackend,
            true);
        assert(fixture.gate.evaluate(
            defaultBackend).allowed);

        HttpServerRequest query =
            fixture.mutationRequest(
                route + "?source=browser",
                "default");
        fixture.addBrowserAuthentication(
            query,
            true);
        assert(fixture.gate.evaluate(query).allowed);

        HttpServerRequest wrongScope =
            fixture.mutationRequest(
                route,
                "house-b");
        fixture.addBrowserAuthentication(
            wrongScope,
            true);

        const SecurityGateDecision wrongScopeDecision =
            fixture.gate.evaluate(wrongScope);
        assert(!wrongScopeDecision.allowed);
        assert(
            wrongScopeDecision.rejection.statusCode ==
            403);
        assert(wrongScopeDecision.rejection.body.find(
            "backend_scope_denied") !=
            std::string::npos);

        assert(fixture.grantRepository.revokeGrant(
            fixture.actorId,
            "searchtimers.execute",
            "default"));

        HttpServerRequest trailingSlash =
            fixture.mutationRequest(
                route + "/",
                "default");
        fixture.addBrowserAuthentication(
            trailingSlash,
            true);

        const SecurityGateDecision trailingSlashDecision =
            fixture.gate.evaluate(trailingSlash);
        assert(!trailingSlashDecision.allowed);
        assert(
            trailingSlashDecision.rejection.statusCode ==
            503);
        assert(trailingSlashDecision.rejection.body.find(
            "security_policy_not_migrated") !=
            std::string::npos);
    }

    assert(fixture.grantRepository.ensureGrant(
        fixture.actorId,
        "role.admin",
        "*"));

    for (const std::string& route : kRoutes)
    {
        HttpServerRequest wildcardAdmin =
            fixture.mutationRequest(
                route,
                "default");
        fixture.addBrowserAuthentication(
            wildcardAdmin,
            true);

        const SecurityGateDecision decision =
            fixture.gate.evaluate(wildcardAdmin);
        assert(!decision.allowed);
        assert(decision.rejection.body.find(
            "backend_scope_denied") !=
            std::string::npos);
    }

    assert(fixture.grantRepository.revokeGrant(
        fixture.actorId,
        "role.admin",
        "*"));
    assert(fixture.grantRepository.ensureGrant(
        fixture.actorId,
        "role.admin",
        "default"));

    for (const std::string& route : kRoutes)
    {
        HttpServerRequest admin =
            fixture.mutationRequest(
                route,
                "default");
        fixture.addBrowserAuthentication(
            admin,
            true);
        assert(fixture.gate.evaluate(admin).allowed);
    }

    assert(fixture.grantRepository.ensureGrant(
        fixture.actorId,
        "role.read-only",
        "default"));

    for (const std::string& route : kRoutes)
    {
        HttpServerRequest readOnly =
            fixture.mutationRequest(
                route,
                "default");
        fixture.addBrowserAuthentication(
            readOnly,
            true);

        const SecurityGateDecision decision =
            fixture.gate.evaluate(readOnly);
        assert(!decision.allowed);
        assert(
            decision.rejection.statusCode == 403);
        assert(decision.rejection.body.find(
            "role_read_only") !=
            std::string::npos);
    }

    bool sawCsrf = false;
    bool sawAllowed = false;

    for (const AccountabilityEvent& event :
         fixture.accountabilityRepository.listAll())
    {
        if (event.permission == "searchtimers.execute")
        {
            assert(event.action == "searchtimers.execute");
            sawCsrf = sawCsrf ||
                (event.reasonCode ==
                     "csrf_validation_failed" &&
                 event.outcome ==
                     "dispatch_denied");
            sawAllowed = sawAllowed ||
                event.outcome ==
                    "dispatch_authorized";
        }

        assert(event.permission.find("Basic ") ==
            std::string::npos);
        assert(event.reasonCode.find(
            SecurityHttpGateBrowserTestFixture::
                sessionSecret) ==
            std::string::npos);
        assert(event.reasonCode.find(
            SecurityHttpGateBrowserTestFixture::
                csrfSecret) ==
            std::string::npos);
    }

    assert(sawCsrf);
    assert(sawAllowed);

    return 0;
}
