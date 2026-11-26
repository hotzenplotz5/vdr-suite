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


HttpServerRequest hierarchyRequestFor(
    const SecurityHttpGateBrowserTestFixture& fixture,
    const std::string& backendId,
    bool csrf)
{
    HttpServerRequest request;
    request.method = "POST";
    request.path =
        "/api/backends/" +
        backendId +
        "/recordings/series-hierarchy";
    request.body =
        "{\"operationId\":\"series-hierarchy-security-test\","
        "\"backendId\":\"body-must-not-authorize\","
        "\"operation\":\"set\","
        "\"resourceKey\":\"recording-test\"}";
    request.headers["X-Request-ID"] =
        "series-hierarchy-request";
    request.headers["X-Correlation-ID"] =
        "series-hierarchy-correlation";
    fixture.addBrowserAuthentication(
        request,
        csrf);
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
    {
        SecurityHttpGateBrowserTestFixture readOnlyFixture;
        HttpServerRequest request = requestFor(
            readOnlyFixture,
            "living-room",
            "assign",
            true);
        const SecurityGateDecision decision =
            readOnlyFixture.gate.evaluate(request);
        assert(!decision.allowed);
        assert(decision.rejection.statusCode == 403);
        assert(decision.rejection.body.find("permission_denied") !=
            std::string::npos);
        const std::vector<AccountabilityEvent> events =
            readOnlyFixture.accountabilityRepository.listAll();
        assert(hasEvent(
            events,
            "authorization.denied",
            "metadata.recording.assign",
            "living-room",
            "metadata.recording.assign",
            "dispatch_denied"));
    }


    {
        SecurityHttpGateBrowserTestFixture readOnlyFixture;
        HttpServerRequest request =
            hierarchyRequestFor(
                readOnlyFixture,
                "living-room",
                true);
        const SecurityGateDecision decision =
            readOnlyFixture.gate.evaluate(request);

        assert(!decision.allowed);
        assert(decision.rejection.statusCode == 403);
        assert(
            decision.rejection.body.find(
                "permission_denied") !=
            std::string::npos);
    }

    SecurityHttpGateBrowserTestFixture fixture;
    assert(fixture.grantRepository.ensureGrant(
        fixture.actorId,
        "metadata.recording.assign",
        "living-room"));

    // Public genre reads inherit the existing recordings.view scope, not
    // the write grant and not a new role.
    {
        SecurityHttpGateBrowserTestFixture reader;
        auto request = requestFor(reader, "living-room", "genre", false);
        request.method = "GET";
        request.path.insert(4, "/v1");
        request.path += "?recordingId=rec_0123456789abcdef0123456789abcdef";
        request.body.clear();
        assert(!reader.gate.evaluate(request).allowed);
        assert(reader.grantRepository.ensureGrant(reader.actorId, "recordings.view", "living-room"));
        const auto allowed = reader.gate.evaluate(request);
        assert(allowed.allowed);
        assert(allowed.authorizationDecision.permission == "recordings.view");
        assert(allowed.authorizedBackendIds == std::vector<std::string>{"living-room"});
        request.path.replace(request.path.find("living-room"), std::string("living-room").size(), "bedroom");
        assert(!reader.gate.evaluate(request).allowed);
    }

    // The public variants use the exact same scoped permission and CSRF.
    for (const std::string operation : {"search", "seasons", "episodes", "assign", "withdraw", "genre"})
    {
        auto request = requestFor(fixture, "living-room", operation, true);
        request.path.insert(4, "/v1");
        const auto decision = fixture.gate.evaluate(request);
        assert(decision.allowed);
        assert(decision.authorizationDecision.permission == "metadata.recording.assign");
        assert(decision.authorizationDecision.backendId == "living-room");
        auto wrongBackend = requestFor(fixture, "bedroom", operation, true);
        wrongBackend.path.insert(4, "/v1");
        assert(!fixture.gate.evaluate(wrongBackend).allowed);
        auto noCsrf = requestFor(fixture, "living-room", operation, false);
        noCsrf.path.insert(4, "/v1");
        assert(!fixture.gate.evaluate(noCsrf).allowed);
    }

    for (const std::string operation : {
             "search", "seasons", "episodes", "assign", "withdraw", "genre"})
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
        HttpServerRequest request =
            hierarchyRequestFor(
                fixture,
                "living-room",
                true);
        const SecurityGateDecision decision =
            fixture.gate.evaluate(request);

        assert(decision.allowed);
        assert(decision.protectedMutation);
        assert(decision.authorizationDecision.allowed);
        assert(
            decision.authorizationDecision.permission ==
            "metadata.recording.assign");
        assert(
            decision.authorizationDecision.backendId ==
            "living-room");
        assert(
            decision.authorizationDecision.action ==
            "metadata.recording.hierarchy");
        assert(
            decision.operationId ==
            "series-hierarchy-security-test");
        assert(
            fixture.gate.appendProtectedMutationOutcome(
                decision,
                200));
    }

    {
        HttpServerRequest request =
            hierarchyRequestFor(
                fixture,
                "living-room",
                false);
        const SecurityGateDecision decision =
            fixture.gate.evaluate(request);

        assert(!decision.allowed);
        assert(decision.rejection.statusCode == 403);
        assert(
            decision.rejection.body.find(
                "csrf_validation_failed") !=
            std::string::npos);
    }

    {
        HttpServerRequest request =
            hierarchyRequestFor(
                fixture,
                "bedroom",
                true);
        const SecurityGateDecision decision =
            fixture.gate.evaluate(request);

        assert(!decision.allowed);
        assert(decision.rejection.statusCode == 403);
        assert(
            decision.rejection.body.find(
                "backend_scope_denied") !=
            std::string::npos);
    }

    {
        HttpServerRequest request =
            hierarchyRequestFor(
                fixture,
                "living%2Froom",
                true);
        const SecurityGateDecision decision =
            fixture.gate.evaluate(request);

        assert(!decision.allowed);
        assert(decision.rejection.statusCode == 503);
        assert(
            decision.rejection.body.find(
                "security_policy_not_migrated") !=
            std::string::npos);
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


    assert(hasEvent(
        events,
        "authorization.allowed",
        "metadata.recording.assign",
        "living-room",
        "metadata.recording.hierarchy",
        "dispatch_authorized"));
    assert(hasEvent(
        events,
        "operation.succeeded",
        "metadata.recording.assign",
        "living-room",
        "metadata.recording.hierarchy",
        "succeeded"));

    return 0;
}
