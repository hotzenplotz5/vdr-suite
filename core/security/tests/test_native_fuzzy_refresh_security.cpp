#include "SecurityHttpGateBrowserTestFixture.h"

#include <cassert>
#include <string>
#include <vector>

namespace
{
const std::vector<std::string> kNativeFuzzyRoutes = {
    "/api/epgsearch/native-fuzzy/refresh",
    "/api/vdr/epgsearch/native-fuzzy/refresh"
};

const std::vector<std::string> kNativeFuzzyStaleProbeDeleteRoutes = {
    "/api/epgsearch/native-fuzzy/stale-probes/delete",
    "/api/vdr/epgsearch/native-fuzzy/stale-probes/delete"
};

const std::string kNativeFuzzyStaleProbeDeletePermission =
    "epgsearch.native-fuzzy.stale-probes.delete";

struct QueryCacheRoute
{
    std::string path;
    std::string permission;
    std::string action;
    std::string otherPermission;
};

const std::vector<QueryCacheRoute> kQueryCacheRoutes = {
    {
        "/api/searchtimers/preview/cache/refresh",
        "searchtimers.preview-cache.refresh",
        "searchtimers.preview-cache.refresh",
        "epg.cache.refresh"
    },
    {
        "/api/vdr/searchtimers/preview/cache/refresh",
        "searchtimers.preview-cache.refresh",
        "searchtimers.preview-cache.refresh",
        "epg.cache.refresh"
    },
    {
        "/api/epg/cache/refresh",
        "epg.cache.refresh",
        "epg.cache.refresh",
        "searchtimers.preview-cache.refresh"
    }
};

void assertDenied(
    const SecurityGateDecision& decision,
    int statusCode,
    const std::string& reason)
{
    assert(!decision.allowed);
    assert(decision.rejection.statusCode == statusCode);
    assert(decision.rejection.body.find(reason) != std::string::npos);
}

void testNativeFuzzyRefresh()
{
    SecurityHttpGateBrowserTestFixture fixture;

    for (const std::string& route : kNativeFuzzyRoutes)
    {
        HttpServerRequest legacy =
            fixture.mutationRequest(route, "default");
        fixture.addLegacyAuthentication(legacy);

        const SecurityGateDecision legacyDecision =
            fixture.gate.evaluate(legacy);
        assert(legacyDecision.allowed);
        assert(legacyDecision.protectedMutation);

        HttpServerRequest missingCsrf =
            fixture.mutationRequest(route, "default");
        fixture.addBrowserAuthentication(missingCsrf);
        assertDenied(
            fixture.gate.evaluate(missingCsrf),
            403,
            "csrf_validation_failed");

        HttpServerRequest noPermission =
            fixture.mutationRequest(route, "default");
        fixture.addBrowserAuthentication(noPermission, true);
        assertDenied(
            fixture.gate.evaluate(noPermission),
            403,
            "permission_denied");

        assert(fixture.grantRepository.ensureGrant(
            fixture.actorId,
            "epgsearch.native-fuzzy.refresh",
            "default"));

        const SecurityGateDecision directAllowed =
            fixture.gate.evaluate(noPermission);
        assert(directAllowed.allowed);
        assert(directAllowed.protectedMutation);

        HttpServerRequest defaultBackend =
            fixture.mutationRequest(route, "", false);
        fixture.addBrowserAuthentication(defaultBackend, true);
        assert(fixture.gate.evaluate(defaultBackend).allowed);

        HttpServerRequest query =
            fixture.mutationRequest(
                route + "?source=browser",
                "default");
        fixture.addBrowserAuthentication(query, true);
        assert(fixture.gate.evaluate(query).allowed);

        HttpServerRequest wrongScope =
            fixture.mutationRequest(route, "house-b");
        fixture.addBrowserAuthentication(wrongScope, true);
        assertDenied(
            fixture.gate.evaluate(wrongScope),
            403,
            "backend_scope_denied");

        assert(fixture.grantRepository.revokeGrant(
            fixture.actorId,
            "epgsearch.native-fuzzy.refresh",
            "default"));

        HttpServerRequest trailingSlash =
            fixture.mutationRequest(route + "/", "default");
        fixture.addBrowserAuthentication(trailingSlash, true);
        assertDenied(
            fixture.gate.evaluate(trailingSlash),
            503,
            "security_policy_not_migrated");
    }

    assert(fixture.grantRepository.ensureGrant(
        fixture.actorId,
        "role.admin",
        "*"));

    for (const std::string& route : kNativeFuzzyRoutes)
    {
        HttpServerRequest wildcardAdmin =
            fixture.mutationRequest(route, "default");
        fixture.addBrowserAuthentication(wildcardAdmin, true);
        assertDenied(
            fixture.gate.evaluate(wildcardAdmin),
            403,
            "backend_scope_denied");
    }

    assert(fixture.grantRepository.revokeGrant(
        fixture.actorId,
        "role.admin",
        "*"));
    assert(fixture.grantRepository.ensureGrant(
        fixture.actorId,
        "role.admin",
        "default"));

    for (const std::string& route : kNativeFuzzyRoutes)
    {
        HttpServerRequest admin =
            fixture.mutationRequest(route, "default");
        fixture.addBrowserAuthentication(admin, true);
        assert(fixture.gate.evaluate(admin).allowed);
    }

    assert(fixture.grantRepository.ensureGrant(
        fixture.actorId,
        "role.read-only",
        "default"));

    for (const std::string& route : kNativeFuzzyRoutes)
    {
        HttpServerRequest readOnly =
            fixture.mutationRequest(route, "default");
        fixture.addBrowserAuthentication(readOnly, true);
        assertDenied(
            fixture.gate.evaluate(readOnly),
            403,
            "role_read_only");
    }

    bool sawCsrf = false;
    bool sawAllowed = false;

    for (const AccountabilityEvent& event :
         fixture.accountabilityRepository.listAll())
    {
        if (event.permission == "epgsearch.native-fuzzy.refresh")
        {
            assert(event.action == "epgsearch.native-fuzzy.refresh");
            sawCsrf = sawCsrf ||
                (event.reasonCode == "csrf_validation_failed" &&
                 event.outcome == "dispatch_denied");
            sawAllowed = sawAllowed ||
                event.outcome == "dispatch_authorized";
        }

        assert(event.permission.find("Basic ") == std::string::npos);
        assert(event.reasonCode.find(
            SecurityHttpGateBrowserTestFixture::sessionSecret) ==
            std::string::npos);
        assert(event.reasonCode.find(
            SecurityHttpGateBrowserTestFixture::csrfSecret) ==
            std::string::npos);
    }

    assert(sawCsrf);
    assert(sawAllowed);
}

void testNativeFuzzyStaleProbeDelete()
{
    SecurityHttpGateBrowserTestFixture fixture;

    for (const std::string& route : kNativeFuzzyStaleProbeDeleteRoutes)
    {
        HttpServerRequest legacy =
            fixture.mutationRequest(route, "body-scope-must-be-ignored");
        fixture.addLegacyAuthentication(legacy);
        const SecurityGateDecision legacyDecision =
            fixture.gate.evaluate(legacy);
        assert(legacyDecision.allowed);
        assert(legacyDecision.protectedMutation);

        HttpServerRequest missingCsrf =
            fixture.mutationRequest(route, "body-scope-must-be-ignored");
        fixture.addBrowserAuthentication(missingCsrf);
        assertDenied(
            fixture.gate.evaluate(missingCsrf),
            403,
            "csrf_validation_failed");

        HttpServerRequest noPermission =
            fixture.mutationRequest(route, "body-scope-must-be-ignored");
        fixture.addBrowserAuthentication(noPermission, true);
        assertDenied(
            fixture.gate.evaluate(noPermission),
            403,
            "permission_denied");
    }

    assert(fixture.grantRepository.ensureGrant(
        fixture.actorId,
        kNativeFuzzyStaleProbeDeletePermission,
        "default"));

    for (const std::string& route : kNativeFuzzyStaleProbeDeleteRoutes)
    {
        HttpServerRequest concreteScope =
            fixture.mutationRequest(route, "default");
        fixture.addBrowserAuthentication(concreteScope, true);
        assertDenied(
            fixture.gate.evaluate(concreteScope),
            403,
            "backend_scope_denied");
    }

    assert(fixture.grantRepository.revokeGrant(
        fixture.actorId,
        kNativeFuzzyStaleProbeDeletePermission,
        "default"));
    assert(fixture.grantRepository.ensureGrant(
        fixture.actorId,
        "epgsearch.native-fuzzy.refresh",
        "*"));

    for (const std::string& route : kNativeFuzzyStaleProbeDeleteRoutes)
    {
        HttpServerRequest otherPermission =
            fixture.mutationRequest(route, "default");
        fixture.addBrowserAuthentication(otherPermission, true);
        assertDenied(
            fixture.gate.evaluate(otherPermission),
            403,
            "permission_denied");
    }

    assert(fixture.grantRepository.revokeGrant(
        fixture.actorId,
        "epgsearch.native-fuzzy.refresh",
        "*"));
    assert(fixture.grantRepository.ensureGrant(
        fixture.actorId,
        kNativeFuzzyStaleProbeDeletePermission,
        "*"));

    for (const std::string& route : kNativeFuzzyStaleProbeDeleteRoutes)
    {
        HttpServerRequest direct =
            fixture.mutationRequest(route, "body-scope-must-be-ignored");
        fixture.addBrowserAuthentication(direct, true);
        const SecurityGateDecision directDecision =
            fixture.gate.evaluate(direct);
        assert(directDecision.allowed);
        assert(directDecision.protectedMutation);

        HttpServerRequest query = fixture.mutationRequest(
            route + "?backend=default&source=browser",
            "default");
        fixture.addBrowserAuthentication(query, true);
        assert(fixture.gate.evaluate(query).allowed);

        HttpServerRequest trailingSlash = fixture.mutationRequest(
            route + "/?source=browser",
            "default");
        fixture.addBrowserAuthentication(trailingSlash, true);
        assertDenied(
            fixture.gate.evaluate(trailingSlash),
            503,
            "security_policy_not_migrated");
    }

    assert(fixture.grantRepository.revokeGrant(
        fixture.actorId,
        kNativeFuzzyStaleProbeDeletePermission,
        "*"));
    assert(fixture.grantRepository.ensureGrant(
        fixture.actorId,
        "role.admin",
        "default"));

    for (const std::string& route : kNativeFuzzyStaleProbeDeleteRoutes)
    {
        HttpServerRequest concreteAdmin =
            fixture.mutationRequest(route, "default");
        fixture.addBrowserAuthentication(concreteAdmin, true);
        assertDenied(
            fixture.gate.evaluate(concreteAdmin),
            403,
            "backend_scope_denied");
    }

    assert(fixture.grantRepository.revokeGrant(
        fixture.actorId,
        "role.admin",
        "default"));
    assert(fixture.grantRepository.ensureGrant(
        fixture.actorId,
        "role.admin",
        "*"));

    for (const std::string& route : kNativeFuzzyStaleProbeDeleteRoutes)
    {
        HttpServerRequest globalAdmin =
            fixture.mutationRequest(route, "default");
        fixture.addBrowserAuthentication(globalAdmin, true);
        assert(fixture.gate.evaluate(globalAdmin).allowed);
    }

    assert(fixture.grantRepository.ensureGrant(
        fixture.actorId,
        "role.read-only",
        "default"));

    for (const std::string& route : kNativeFuzzyStaleProbeDeleteRoutes)
    {
        HttpServerRequest concreteReadOnly =
            fixture.mutationRequest(route, "default");
        fixture.addBrowserAuthentication(concreteReadOnly, true);
        assert(fixture.gate.evaluate(concreteReadOnly).allowed);
    }

    assert(fixture.grantRepository.revokeGrant(
        fixture.actorId,
        "role.read-only",
        "default"));
    assert(fixture.grantRepository.ensureGrant(
        fixture.actorId,
        "role.read-only",
        "*"));

    for (const std::string& route : kNativeFuzzyStaleProbeDeleteRoutes)
    {
        HttpServerRequest globalReadOnly =
            fixture.mutationRequest(route, "default");
        fixture.addBrowserAuthentication(globalReadOnly, true);
        assertDenied(
            fixture.gate.evaluate(globalReadOnly),
            403,
            "role_read_only");
    }

    bool sawGlobalScope = false;
    bool sawCsrf = false;
    bool sawScopeDenial = false;
    bool sawReadOnly = false;
    bool sawAllowed = false;

    for (const AccountabilityEvent& event :
         fixture.accountabilityRepository.listAll())
    {
        if (event.permission == kNativeFuzzyStaleProbeDeletePermission)
        {
            assert(event.action ==
                kNativeFuzzyStaleProbeDeletePermission);
            assert(event.backendId == "*");
            sawGlobalScope = true;
            sawCsrf = sawCsrf ||
                (event.reasonCode == "csrf_validation_failed" &&
                 event.outcome == "dispatch_denied");
            sawScopeDenial = sawScopeDenial ||
                (event.reasonCode == "backend_scope_denied" &&
                 event.outcome == "dispatch_denied");
            sawReadOnly = sawReadOnly ||
                (event.reasonCode == "role_read_only" &&
                 event.outcome == "dispatch_denied");
            sawAllowed = sawAllowed ||
                event.outcome == "dispatch_authorized";
        }

        assert(event.permission.find("Basic ") == std::string::npos);
        assert(event.reasonCode.find(
            SecurityHttpGateBrowserTestFixture::sessionSecret) ==
            std::string::npos);
        assert(event.reasonCode.find(
            SecurityHttpGateBrowserTestFixture::csrfSecret) ==
            std::string::npos);
    }

    assert(sawGlobalScope);
    assert(sawCsrf);
    assert(sawScopeDenial);
    assert(sawReadOnly);
    assert(sawAllowed);
}

void testQueryCacheRefreshRoute(const QueryCacheRoute& contract)
{
    SecurityHttpGateBrowserTestFixture fixture;
    const std::string scopedPath =
        contract.path + "?backend=house-a";

    HttpServerRequest legacy = fixture.mutationRequest(
        scopedPath,
        "body-must-not-control-scope");
    fixture.addLegacyAuthentication(legacy);
    const SecurityGateDecision legacyDecision =
        fixture.gate.evaluate(legacy);
    assert(legacyDecision.allowed);
    assert(legacyDecision.protectedMutation);

    HttpServerRequest missingCsrf = fixture.mutationRequest(
        scopedPath,
        "body-must-not-control-scope");
    fixture.addBrowserAuthentication(missingCsrf);
    assertDenied(
        fixture.gate.evaluate(missingCsrf),
        403,
        "csrf_validation_failed");

    HttpServerRequest request = fixture.mutationRequest(
        scopedPath,
        "body-must-not-control-scope");
    fixture.addBrowserAuthentication(request, true);
    assertDenied(
        fixture.gate.evaluate(request),
        403,
        "permission_denied");

    assert(fixture.grantRepository.ensureGrant(
        fixture.actorId,
        contract.permission,
        "house-a"));

    const SecurityGateDecision directAllowed =
        fixture.gate.evaluate(request);
    assert(directAllowed.allowed);
    assert(directAllowed.protectedMutation);

    HttpServerRequest encoded = fixture.mutationRequest(
        contract.path + "?backend=house%2Da",
        "house-b");
    fixture.addBrowserAuthentication(encoded, true);
    assert(fixture.gate.evaluate(encoded).allowed);

    HttpServerRequest duplicate = fixture.mutationRequest(
        contract.path + "?backend=house-b&backend=house-a",
        "house-b");
    fixture.addBrowserAuthentication(duplicate, true);
    assert(fixture.gate.evaluate(duplicate).allowed);

    HttpServerRequest wrongScope = fixture.mutationRequest(
        contract.path + "?backend=house-b",
        "house-a");
    fixture.addBrowserAuthentication(wrongScope, true);
    assertDenied(
        fixture.gate.evaluate(wrongScope),
        403,
        "backend_scope_denied");

    assert(fixture.grantRepository.ensureGrant(
        fixture.actorId,
        contract.permission,
        "default"));
    HttpServerRequest defaultBackend = fixture.mutationRequest(
        contract.path,
        "body-must-not-control-scope");
    fixture.addBrowserAuthentication(defaultBackend, true);
    assert(fixture.gate.evaluate(defaultBackend).allowed);

    assert(fixture.grantRepository.revokeGrant(
        fixture.actorId,
        contract.permission,
        "house-a"));
    assert(fixture.grantRepository.revokeGrant(
        fixture.actorId,
        contract.permission,
        "default"));
    assert(fixture.grantRepository.ensureGrant(
        fixture.actorId,
        contract.otherPermission,
        "house-a"));
    assertDenied(
        fixture.gate.evaluate(request),
        403,
        "permission_denied");
    assert(fixture.grantRepository.revokeGrant(
        fixture.actorId,
        contract.otherPermission,
        "house-a"));

    HttpServerRequest trailingSlash = fixture.mutationRequest(
        contract.path + "/?backend=house-a",
        "house-a");
    fixture.addBrowserAuthentication(trailingSlash, true);
    assertDenied(
        fixture.gate.evaluate(trailingSlash),
        503,
        "security_policy_not_migrated");

    assert(fixture.grantRepository.ensureGrant(
        fixture.actorId,
        "role.admin",
        "*"));
    assertDenied(
        fixture.gate.evaluate(request),
        403,
        "backend_scope_denied");
    assert(fixture.grantRepository.revokeGrant(
        fixture.actorId,
        "role.admin",
        "*"));

    assert(fixture.grantRepository.ensureGrant(
        fixture.actorId,
        "role.admin",
        "house-a"));
    assert(fixture.gate.evaluate(request).allowed);

    assert(fixture.grantRepository.ensureGrant(
        fixture.actorId,
        "role.read-only",
        "house-a"));
    assertDenied(
        fixture.gate.evaluate(request),
        403,
        "role_read_only");

    bool sawScopedEvent = false;
    bool sawCsrf = false;
    bool sawAllowed = false;

    for (const AccountabilityEvent& event :
         fixture.accountabilityRepository.listAll())
    {
        if (event.permission == contract.permission)
        {
            assert(event.action == contract.action);
            if (event.backendId == "house-a")
            {
                sawScopedEvent = true;
            }
            sawCsrf = sawCsrf ||
                (event.reasonCode == "csrf_validation_failed" &&
                 event.outcome == "dispatch_denied");
            sawAllowed = sawAllowed ||
                event.outcome == "dispatch_authorized";
        }

        assert(event.permission.find("Basic ") == std::string::npos);
        assert(event.reasonCode.find(
            SecurityHttpGateBrowserTestFixture::sessionSecret) ==
            std::string::npos);
        assert(event.reasonCode.find(
            SecurityHttpGateBrowserTestFixture::csrfSecret) ==
            std::string::npos);
    }

    assert(sawScopedEvent);
    assert(sawCsrf);
    assert(sawAllowed);
}

void testQueryCacheRefresh()
{
    for (const QueryCacheRoute& contract : kQueryCacheRoutes)
    {
        testQueryCacheRefreshRoute(contract);
    }

    SecurityHttpGateBrowserTestFixture fixture;
    HttpServerRequest excluded = fixture.mutationRequest(
        "/api/phase62/unmapped-mutation",
        "house-a");
    fixture.addBrowserAuthentication(excluded, true);
    assertDenied(
        fixture.gate.evaluate(excluded),
        503,
        "security_policy_not_migrated");
}
}

int main()
{
    testNativeFuzzyRefresh();
    testNativeFuzzyStaleProbeDelete();
    testQueryCacheRefresh();
    return 0;
}
