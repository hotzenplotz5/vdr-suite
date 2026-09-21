#include "SecurityHttpGateBrowserTestFixture.h"

#include <cassert>
#include <string>
#include <vector>

namespace
{
struct Route
{
    std::string path;
    std::string permission;
};

const std::vector<Route> kRoutes = {
    {"/api/searchtimers/update", "searchtimers.modify"},
    {"/api/vdr/searchtimers/update", "searchtimers.modify"},
    {"/api/searchtimers/delete", "searchtimers.delete"},
    {"/api/vdr/searchtimers/delete", "searchtimers.delete"}
};
}

int main()
{
    SecurityHttpGateBrowserTestFixture fixture;

    for (const Route& route : kRoutes)
    {
        HttpServerRequest legacy =
            fixture.mutationRequest(
                route.path,
                "default");
        fixture.addLegacyAuthentication(legacy);

        const SecurityGateDecision legacyDecision =
            fixture.gate.evaluate(legacy);
        assert(legacyDecision.allowed);
        assert(legacyDecision.protectedMutation);

        HttpServerRequest missingCsrf =
            fixture.mutationRequest(
                route.path,
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
                route.path,
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
            route.permission,
            "default"));

        const SecurityGateDecision directAllowed =
            fixture.gate.evaluate(noPermission);
        assert(directAllowed.allowed);
        assert(directAllowed.protectedMutation);

        HttpServerRequest defaultBackend =
            fixture.mutationRequest(
                route.path,
                "",
                false);
        fixture.addBrowserAuthentication(
            defaultBackend,
            true);
        assert(fixture.gate.evaluate(
            defaultBackend).allowed);

        HttpServerRequest query =
            fixture.mutationRequest(
                route.path + "?source=browser",
                "default");
        fixture.addBrowserAuthentication(
            query,
            true);
        assert(fixture.gate.evaluate(query).allowed);

        HttpServerRequest wrongScope =
            fixture.mutationRequest(
                route.path,
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
            route.permission,
            "default"));

        HttpServerRequest trailingSlash =
            fixture.mutationRequest(
                route.path + "/",
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

    for (const Route& route : kRoutes)
    {
        HttpServerRequest wildcardAdmin =
            fixture.mutationRequest(
                route.path,
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

    for (const Route& route : kRoutes)
    {
        HttpServerRequest admin =
            fixture.mutationRequest(
                route.path,
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

    for (const Route& route : kRoutes)
    {
        HttpServerRequest readOnly =
            fixture.mutationRequest(
                route.path,
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

    bool sawModifyCsrf = false;
    bool sawModifyAllowed = false;
    bool sawDeleteCsrf = false;
    bool sawDeleteAllowed = false;

    for (const AccountabilityEvent& event :
         fixture.accountabilityRepository.listAll())
    {
        sawModifyCsrf = sawModifyCsrf ||
            (event.permission ==
                 "searchtimers.modify" &&
             event.reasonCode ==
                 "csrf_validation_failed" &&
             event.outcome ==
                 "dispatch_denied");
        sawModifyAllowed = sawModifyAllowed ||
            (event.permission ==
                 "searchtimers.modify" &&
             event.outcome ==
                 "dispatch_authorized");
        sawDeleteCsrf = sawDeleteCsrf ||
            (event.permission ==
                 "searchtimers.delete" &&
             event.reasonCode ==
                 "csrf_validation_failed" &&
             event.outcome ==
                 "dispatch_denied");
        sawDeleteAllowed = sawDeleteAllowed ||
            (event.permission ==
                 "searchtimers.delete" &&
             event.outcome ==
                 "dispatch_authorized");

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

    assert(sawModifyCsrf);
    assert(sawModifyAllowed);
    assert(sawDeleteCsrf);
    assert(sawDeleteAllowed);

    return 0;
}
