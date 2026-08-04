#include "SecurityHttpGateBrowserTestFixture.h"

#include <algorithm>
#include <cassert>
#include <string>
#include <vector>

namespace
{
HttpServerRequest requestFor(
    const SecurityHttpGateBrowserTestFixture& fixture,
    const std::string& backendId,
    const std::string& operation,
    bool csrf)
{
    HttpServerRequest request;
    request.method = "POST";
    request.path =
        "/api/backends/" + backendId +
        "/recordings/metadata/" + operation;
    request.body =
        "{\"operationId\":\"manual-metadata-test\","
        "\"backendId\":\"body-must-not-authorize\"}";
    request.headers["X-Request-ID"] = "manual-metadata-request";
    request.headers["X-Correlation-ID"] = "manual-metadata-correlation";
    fixture.addBrowserAuthentication(request, csrf);
    return request;
}

bool hasEvent(
    const std::vector<AccountabilityEvent>& events,
    const std::string& eventType,
    const std::string& permission,
    const std::string& backendId,
    const std::string& action,
    const std::string& outcome)
{
    return std::any_of(events.begin(), events.end(), [&](const auto& event) {
        return event.eventType == eventType &&
            event.permission == permission &&
            event.backendId == backendId &&
            event.action == action &&
            event.outcome == outcome;
    });
}
}

int main()
{
    SecurityHttpGateBrowserTestFixture fixture;
    assert(fixture.grantRepository.ensureGrant(
        fixture.actorId,
        "metadata.recording.assign",
        "living-room"));

    for (const std::string operation : {
             "search", "seasons", "episodes", "assign", "withdraw"})
    {
        HttpServerRequest request = requestFor(
            fixture,
            "living-room",
            operation,
            true);
        const SecurityGateDecision decision = fixture.gate.evaluate(request);
        assert(decision.allowed);
        assert(decision.protectedMutation);
        assert(decision.authorizationDecision.allowed);
        assert(decision.authorizationDecision.permission ==
            "metadata.recording.assign");
        assert(decision.authorizationDecision.backendId == "living-room");
        assert(decision.authorizationDecision.action ==
            "metadata.recording." + operation);
        assert(decision.operationId == "manual-metadata-test");
        assert(fixture.gate.appendProtectedMutationOutcome(decision, 200));
    }

    {
        HttpServerRequest request = requestFor(
            fixture,
            "living-room",
            "assign",
            false);
        const SecurityGateDecision decision = fixture.gate.evaluate(request);
        assert(!decision.allowed);
        assert(decision.rejection.statusCode == 403);
        assert(decision.rejection.body.find("csrf_validation_failed") !=
            std::string::npos);
    }

    {
        HttpServerRequest request = requestFor(
            fixture,
            "bedroom",
            "assign",
            true);
        const SecurityGateDecision decision = fixture.gate.evaluate(request);
        assert(!decision.allowed);
        assert(decision.rejection.statusCode == 403);
        assert(decision.rejection.body.find("backend_scope_denied") !=
            std::string::npos);
    }

    {
        HttpServerRequest request = requestFor(
            fixture,
            "living%2Froom",
            "assign",
            true);
        const SecurityGateDecision decision = fixture.gate.evaluate(request);
        assert(!decision.allowed);
        assert(decision.rejection.statusCode == 503);
        assert(decision.rejection.body.find("security_policy_not_migrated") !=
            std::string::npos);
    }

    {
        HttpServerRequest request = requestFor(
            fixture,
            "living-room",
            "unknown",
            true);
        const SecurityGateDecision decision = fixture.gate.evaluate(request);
        assert(!decision.allowed);
        assert(decision.rejection.statusCode == 503);
    }

    const std::vector<AccountabilityEvent> events =
        fixture.accountabilityRepository.listAll();
    assert(hasEvent(
        events,
        "authorization.allowed",
        "metadata.recording.assign",
        "living-room",
        "metadata.recording.assign",
        "dispatch_authorized"));
    assert(hasEvent(
        events,
        "operation.succeeded",
        "metadata.recording.assign",
        "living-room",
        "metadata.recording.assign",
        "succeeded"));
    assert(hasEvent(
        events,
        "authorization.denied",
        "metadata.recording.assign",
        "bedroom",
        "metadata.recording.assign",
        "dispatch_denied"));

    return 0;
}
