#include "SecurityHttpGateBrowserTestFixture.h"

#include <cassert>
#include <string>

namespace
{
constexpr const char* Permission = "timers.create";
constexpr const char* Route =
    "/api/v1/timer-assignments/assignment:one";

HttpServerRequest browserPost(
    SecurityHttpGateBrowserTestFixture& fixture,
    const std::string& target,
    bool csrf = true)
{
    HttpServerRequest request;
    request.method = "POST";
    request.path = target;
    request.body = "{}";
    request.headers["Content-Type"] = "application/json";
    request.headers["If-Match"] = "\"vsr-31\"";
    request.headers["Idempotency-Key"] = "idem-security-create-1";
    request.headers["X-Request-ID"] =
        "phase69c-public-timer-create";
    fixture.addBrowserAuthentication(request, csrf);
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
            event.action == Permission &&
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
            "backend-one"));

        const SecurityGateDecision decision =
            fixture.gate.evaluate(
                browserPost(
                    fixture,
                    std::string(Route) +
                        "?backend=backend-one"));
        assert(decision.allowed);
        assert(decision.protectedMutation);
        assert(decision.authorizationDecision.allowed);
        assert(decision.authorizationDecision.permission ==
            Permission);
        assert(decision.authorizationDecision.action ==
            Permission);
        assert(decision.authorizationDecision.backendId ==
            "backend-one");
        assert(hasDecisionEvent(
            fixture.accountabilityRepository,
            "permission_granted",
            "backend-one"));

        // Public CREATE does not accept a caller operationId. The Suite issues
        // it only after authorization, so the outer Phase-62 HTTP outcome may
        // legitimately have no operation correlation at gate time.
        assert(decision.operationId.empty());
        assert(fixture.gate.appendProtectedMutationOutcome(
            decision,
            202));

        bool sawOutcome = false;
        for (const AccountabilityEvent& event :
             fixture.accountabilityRepository.listAll())
        {
            if (event.eventType == "operation.succeeded" &&
                event.permission == Permission &&
                event.backendId == "backend-one" &&
                event.reasonCode == "http_status_202")
            {
                sawOutcome = true;
                assert(event.operationId.empty());
            }
        }
        assert(sawOutcome);
    }

    {
        SecurityHttpGateBrowserTestFixture fixture;
        assert(fixture.grantRepository.ensureGrant(
            fixture.actorId,
            Permission,
            "backend-one"));

        const SecurityGateDecision missingCsrf =
            fixture.gate.evaluate(
                browserPost(
                    fixture,
                    std::string(Route) +
                        "?backend=backend-one",
                    false));
        assert(!missingCsrf.allowed);
        assert(missingCsrf.protectedMutation);
        assert(missingCsrf.rejection.statusCode == 403);
        assert(missingCsrf.rejection.body.find(
            "csrf_validation_failed") !=
            std::string::npos);
    }

    {
        SecurityHttpGateBrowserTestFixture fixture;
        assert(fixture.grantRepository.ensureGrant(
            fixture.actorId,
            Permission,
            "backend-one"));

        const SecurityGateDecision wrongBackend =
            fixture.gate.evaluate(
                browserPost(
                    fixture,
                    std::string(Route) +
                        "?backend=backend-two"));
        assert(!wrongBackend.allowed);
        assert(wrongBackend.rejection.statusCode == 403);
        assert(wrongBackend.rejection.body.find(
            "backend_scope_denied") !=
            std::string::npos);
        assert(hasDecisionEvent(
            fixture.accountabilityRepository,
            "backend_scope_denied",
            "backend-two"));
    }

    {
        SecurityHttpGateBrowserTestFixture fixture;
        const SecurityGateDecision missingBackend =
            fixture.gate.evaluate(
                browserPost(fixture, Route));
        assert(!missingBackend.allowed);
        assert(missingBackend.rejection.statusCode == 400);
        assert(missingBackend.rejection.body.find(
            "invalid_backend_scope") !=
            std::string::npos);

        const SecurityGateDecision unsafeBackend =
            fixture.gate.evaluate(
                browserPost(
                    fixture,
                    std::string(Route) +
                        "?backend=backend%2Fone"));
        assert(!unsafeBackend.allowed);
        assert(unsafeBackend.rejection.statusCode == 400);
        assert(unsafeBackend.rejection.body.find(
            "invalid_backend_scope") !=
            std::string::npos);
    }

    {
        SecurityHttpGateBrowserTestFixture fixture;
        const SecurityGateDecision denied =
            fixture.gate.evaluate(
                browserPost(
                    fixture,
                    std::string(Route) +
                        "?backend=backend-one"));
        assert(!denied.allowed);
        assert(denied.rejection.statusCode == 403);
        assert(denied.rejection.body.find(
            "permission_denied") !=
            std::string::npos);
    }

    {
        SecurityHttpGateBrowserTestFixture fixture;
        HttpServerRequest anonymous;
        anonymous.method = "POST";
        anonymous.path =
            std::string(Route) +
            "?backend=backend-one";
        anonymous.body = "{}";
        anonymous.headers["Content-Type"] = "application/json";
        anonymous.headers["If-Match"] = "\"vsr-31\"";
        anonymous.headers["Idempotency-Key"] = "idem-anonymous";

        const SecurityGateDecision decision =
            fixture.gate.evaluate(anonymous);
        assert(!decision.allowed);
        assert(decision.rejection.statusCode == 401);
    }

    return 0;
}
