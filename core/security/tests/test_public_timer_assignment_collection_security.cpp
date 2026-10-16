#include "SecurityHttpGateBrowserTestFixture.h"

#include <cassert>
#include <string>

namespace
{
constexpr const char* Permission = "timers.view";
constexpr const char* Route = "/api/v1/timer-assignments";

HttpServerRequest browserGet(
    SecurityHttpGateBrowserTestFixture& fixture,
    const std::string& target)
{
    HttpServerRequest request;
    request.method = "GET";
    request.path = target;
    request.headers["X-Request-ID"] =
        "phase69d-public-timer-assignment-collection";
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
            return true;
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
            "backend-one"));

        const SecurityGateDecision decision =
            fixture.gate.evaluate(
                browserGet(
                    fixture,
                    std::string(Route) +
                        "?backend=backend-one&limit=50"));
        assert(decision.allowed);
        assert(!decision.protectedMutation);
        assert(decision.authorizationDecision.allowed);
        assert(decision.authorizationDecision.permission ==
            Permission);
        assert(decision.authorizationDecision.backendId ==
            "backend-one");
        assert(decision.authorizationDecision.action ==
            "timers.view");
        assert(hasDecisionEvent(
            fixture.accountabilityRepository,
            "permission_granted",
            "backend-one"));
    }

    {
        SecurityHttpGateBrowserTestFixture fixture;
        assert(fixture.grantRepository.ensureGrant(
            fixture.actorId,
            Permission,
            "backend-one"));

        const SecurityGateDecision decision =
            fixture.gate.evaluate(
                browserGet(
                    fixture,
                    std::string(Route) +
                        "?backend=backend-two"));
        assert(!decision.allowed);
        assert(decision.rejection.statusCode == 403);
        assert(decision.rejection.body.find(
            "backend_scope_denied") !=
            std::string::npos);
    }

    {
        SecurityHttpGateBrowserTestFixture fixture;

        const SecurityGateDecision missing =
            fixture.gate.evaluate(
                browserGet(fixture, Route));
        assert(!missing.allowed);
        assert(missing.rejection.statusCode == 400);
        assert(missing.rejection.body.find(
            "invalid_backend_scope") !=
            std::string::npos);

        const SecurityGateDecision unsafe =
            fixture.gate.evaluate(
                browserGet(
                    fixture,
                    std::string(Route) +
                        "?backend=backend%2Fone"));
        assert(!unsafe.allowed);
        assert(unsafe.rejection.statusCode == 400);
    }

    {
        SecurityHttpGateBrowserTestFixture fixture;
        const SecurityGateDecision decision =
            fixture.gate.evaluate(
                browserGet(
                    fixture,
                    std::string(Route) +
                        "?backend=backend-one"));
        assert(!decision.allowed);
        assert(decision.rejection.statusCode == 403);
        assert(decision.rejection.body.find(
            "permission_denied") !=
            std::string::npos);
    }

    {
        SecurityHttpGateBrowserTestFixture fixture;
        HttpServerRequest anonymous;
        anonymous.method = "GET";
        anonymous.path =
            std::string(Route) +
            "?backend=backend-one";
        anonymous.headers["X-Request-ID"] =
            "phase69d-public-timer-assignment-collection-anonymous";
        const SecurityGateDecision decision =
            fixture.gate.evaluate(anonymous);
        assert(!decision.allowed);
        assert(decision.rejection.statusCode == 401);
    }

    {
        // A valid Browser session and even an explicit SearchTimer grant
        // cannot bypass the independent paired-Device credential boundary.
        SecurityHttpGateBrowserTestFixture fixture;
        assert(fixture.grantRepository.ensureGrant(
            fixture.actorId, "searchtimers.view", "backend-one"));
        const auto cookieDenied = fixture.gate.evaluate(
            browserGet(fixture,
                "/api/v1/search-timers?backend=backend-one"));
        assert(!cookieDenied.allowed);
        assert(cookieDenied.rejection.statusCode == 403);
        // Public-v1 maps internal security reasons to stable Problem Details.
        // The browser must still be rejected by the paired-Device boundary.
        assert(cookieDenied.rejection.headers.at("Content-Type") ==
            "application/problem+json");
        assert(cookieDenied.rejection.body.find(
            "\"code\":\"forbidden\"") != std::string::npos);
        assert(cookieDenied.rejection.body.find(
            "A verified Device credential is required") !=
            std::string::npos);
    }
    return 0;
}
