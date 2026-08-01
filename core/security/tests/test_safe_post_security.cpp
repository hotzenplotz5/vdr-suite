#include "SecurityHttpGateBrowserTestFixture.h"

#include <cassert>
#include <string>
#include <vector>

int main()
{
    SecurityHttpGateBrowserTestFixture fixture;

    const std::vector<std::string> safeRoutes = {
        "/api/recordings/actions/validate",
        "/api/vdr/recordings/actions/validate",
        "/api/recordings/actions/preview",
        "/api/vdr/recordings/actions/preview",
        "/api/searchtimers/validate",
        "/api/vdr/searchtimers/validate",
        "/api/searchtimers/plan",
        "/api/vdr/searchtimers/plan"
    };

    for (const std::string& route : safeRoutes)
    {
        const std::size_t evidenceBefore =
            fixture.accountabilityRepository
                .listAll()
                .size();

        HttpServerRequest legacy =
            fixture.mutationRequest(
                route,
                "default");
        fixture.addLegacyAuthentication(legacy);

        const SecurityGateDecision legacyDecision =
            fixture.gate.evaluate(legacy);
        assert(legacyDecision.allowed);
        assert(!legacyDecision.protectedMutation);

        HttpServerRequest browser =
            fixture.mutationRequest(
                route,
                "default");
        fixture.addBrowserAuthentication(browser);

        const SecurityGateDecision browserDecision =
            fixture.gate.evaluate(browser);
        assert(browserDecision.allowed);
        assert(browserDecision.browserAuthenticated);
        assert(!browserDecision.protectedMutation);

        HttpServerRequest query =
            fixture.mutationRequest(
                route + "?source=browser",
                "default");
        fixture.addBrowserAuthentication(query);

        const SecurityGateDecision queryDecision =
            fixture.gate.evaluate(query);
        assert(queryDecision.allowed);
        assert(!queryDecision.protectedMutation);

        assert(
            fixture.accountabilityRepository
                .listAll()
                .size() == evidenceBefore);

        HttpServerRequest trailingSlash =
            fixture.mutationRequest(
                route + "/",
                "default");
        fixture.addBrowserAuthentication(
            trailingSlash);

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

    const std::vector<std::string> protectedMutations = {
        "/api/searchtimers/execute",
        "/api/vdr/searchtimers/execute",
        "/api/searchtimers/real-test",
        "/api/vdr/searchtimers/real-test",
        "/api/epgsearch/native-fuzzy/refresh",
        "/api/vdr/epgsearch/native-fuzzy/refresh",
        "/api/searchtimers/preview/cache/refresh",
        "/api/vdr/searchtimers/preview/cache/refresh",
        "/api/epg/cache/refresh",
        "/api/epgsearch/native-fuzzy/stale-probes/delete",
        "/api/vdr/epgsearch/native-fuzzy/stale-probes/delete"
    };

    for (const std::string& route : protectedMutations)
    {
        HttpServerRequest request =
            fixture.mutationRequest(
                route,
                "default");
        fixture.addBrowserAuthentication(request);

        const SecurityGateDecision decision =
            fixture.gate.evaluate(request);
        assert(!decision.allowed);
        assert(decision.protectedMutation);
        assert(
            decision.rejection.statusCode == 403);
        assert(decision.rejection.body.find(
            "csrf_validation_failed") !=
            std::string::npos);
    }

    const std::vector<std::string> stillUnmigrated = {
        "/api/phase62/unmapped-mutation",
        "/api/vdr/phase62/unmapped-mutation"
    };

    for (const std::string& route : stillUnmigrated)
    {
        HttpServerRequest request =
            fixture.mutationRequest(
                route,
                "default");
        fixture.addBrowserAuthentication(request);

        const SecurityGateDecision decision =
            fixture.gate.evaluate(request);
        assert(!decision.allowed);
        assert(
            decision.rejection.statusCode == 503);
        assert(decision.rejection.body.find(
            "security_policy_not_migrated") !=
            std::string::npos);
    }

    return 0;
}
