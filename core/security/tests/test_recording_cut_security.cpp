#include "SecurityHttpGateBrowserTestFixture.h"

#include <algorithm>
#include <cassert>
#include <string>
#include <vector>

namespace
{
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

    HttpServerRequest unauthenticated = fixture.mutationRequest(
        "/api/vdr/recordings/cut",
        "default");
    const SecurityGateDecision unauthenticatedDecision =
        fixture.gate.evaluate(unauthenticated);
    assert(!unauthenticatedDecision.allowed);
    assert(unauthenticatedDecision.rejection.statusCode == 401);
    assert(unauthenticatedDecision.rejection.body.find(
        "authentication_required") != std::string::npos);

    HttpServerRequest missingCsrf = fixture.mutationRequest(
        "/api/vdr/recordings/cut",
        "default");
    fixture.addBrowserAuthentication(missingCsrf);

    const SecurityGateDecision missingCsrfDecision =
        fixture.gate.evaluate(missingCsrf);
    assert(!missingCsrfDecision.allowed);
    assert(missingCsrfDecision.protectedMutation);
    assert(missingCsrfDecision.rejection.statusCode == 403);
    assert(missingCsrfDecision.rejection.body.find(
        "csrf_validation_failed") != std::string::npos);

    HttpServerRequest missingPermission = fixture.mutationRequest(
        "/api/vdr/recordings/cut",
        "default");
    fixture.addBrowserAuthentication(missingPermission, true);

    const SecurityGateDecision missingPermissionDecision =
        fixture.gate.evaluate(missingPermission);
    assert(!missingPermissionDecision.allowed);
    assert(missingPermissionDecision.protectedMutation);
    assert(missingPermissionDecision.rejection.statusCode == 403);
    assert(missingPermissionDecision.rejection.body.find(
        "permission_denied") != std::string::npos);

    assert(fixture.grantRepository.ensureGrant(
        fixture.actorId,
        "recordings.cut",
        "default"));

    HttpServerRequest allowed = fixture.mutationRequest(
        "/api/vdr/recordings/cut",
        "default");
    fixture.addBrowserAuthentication(allowed, true);

    const SecurityGateDecision allowedDecision =
        fixture.gate.evaluate(allowed);
    assert(allowedDecision.allowed);
    assert(allowedDecision.protectedMutation);
    assert(allowedDecision.authorizationDecision.allowed);
    assert(allowedDecision.authorizationDecision.permission ==
        "recordings.cut");
    assert(allowedDecision.authorizationDecision.backendId == "default");
    assert(allowedDecision.authorizationDecision.action == "recordings.cut");
    assert(allowedDecision.operationId == "phase62-test-operation");
    assert(fixture.gate.appendProtectedMutationOutcome(allowedDecision, 202));

    const std::vector<AccountabilityEvent> allowedEvents =
        fixture.accountabilityRepository.listAll();
    assert(hasEvent(
        allowedEvents,
        "authorization.allowed",
        "recordings.cut",
        "default",
        "recordings.cut",
        "dispatch_authorized"));
    assert(hasEvent(
        allowedEvents,
        "operation.succeeded",
        "recordings.cut",
        "default",
        "recordings.cut",
        "succeeded"));

    HttpServerRequest wrongScope = fixture.mutationRequest(
        "/api/vdr/recordings/cut",
        "house-b");
    fixture.addBrowserAuthentication(wrongScope, true);

    const SecurityGateDecision wrongScopeDecision =
        fixture.gate.evaluate(wrongScope);
    assert(!wrongScopeDecision.allowed);
    assert(wrongScopeDecision.rejection.statusCode == 403);
    assert(wrongScopeDecision.rejection.body.find(
        "backend_scope_denied") != std::string::npos);

    assert(fixture.grantRepository.ensureGrant(
        fixture.actorId,
        "role.read-only",
        "default"));

    HttpServerRequest readOnly = fixture.mutationRequest(
        "/api/vdr/recordings/cut",
        "default");
    fixture.addBrowserAuthentication(readOnly, true);

    const SecurityGateDecision readOnlyDecision =
        fixture.gate.evaluate(readOnly);
    assert(!readOnlyDecision.allowed);
    assert(readOnlyDecision.protectedMutation);
    assert(readOnlyDecision.rejection.statusCode == 403);
    assert(readOnlyDecision.rejection.body.find(
        "role_read_only") != std::string::npos);

    const std::vector<AccountabilityEvent> finalEvents =
        fixture.accountabilityRepository.listAll();
    assert(hasEvent(
        finalEvents,
        "authorization.denied",
        "recordings.cut",
        "house-b",
        "recordings.cut",
        "dispatch_denied"));
    assert(hasEvent(
        finalEvents,
        "authorization.denied",
        "recordings.cut",
        "default",
        "recordings.cut",
        "dispatch_denied"));

    // A single recordings.edit grant covers the public native edit action.
    // It does not remove CSRF, scope or explicit per-action audit checks.
    SecurityHttpGateBrowserTestFixture editorFixture;
    assert(editorFixture.grantRepository.ensureGrant(
        editorFixture.actorId, "recordings.edit", "default"));
    HttpServerRequest publicMutation = editorFixture.mutationRequest(
        "/api/v1/recordings/rec_0123456789abcdef0123456789abcdef/cut?backendId=default",
        "default");
    editorFixture.addBrowserAuthentication(publicMutation, true);
    const SecurityGateDecision publicAllowed =
        editorFixture.gate.evaluate(publicMutation);
    assert(publicAllowed.allowed);
    assert(publicAllowed.protectedMutation);
    assert(publicAllowed.authorizationDecision.permission ==
        "recordings.cut");
    assert(publicAllowed.authorizationDecision.backendId == "default");

    HttpServerRequest noCsrf = editorFixture.mutationRequest(
        "/api/v1/recordings/rec_0123456789abcdef0123456789abcdef/cut?backendId=default",
        "default");
    editorFixture.addBrowserAuthentication(noCsrf);
    const auto csrfDenied = editorFixture.gate.evaluate(noCsrf);
    assert(!csrfDenied.allowed);
    assert(csrfDenied.rejection.statusCode == 403);

    HttpServerRequest differentBackend = editorFixture.mutationRequest(
        "/api/v1/recordings/rec_0123456789abcdef0123456789abcdef/cut?backendId=house-b",
        "house-b");
    editorFixture.addBrowserAuthentication(differentBackend, true);
    const auto wrongBackend = editorFixture.gate.evaluate(differentBackend);
    assert(!wrongBackend.allowed);
    assert(wrongBackend.rejection.statusCode == 403);

    HttpServerRequest publicRead;
    publicRead.method = "GET";
    publicRead.path =
        "/api/v1/recordings/rec_0123456789abcdef0123456789abcdef/cut?backendId=default";
    editorFixture.addBrowserAuthentication(publicRead, true);
    assert(editorFixture.gate.evaluate(publicRead).allowed);

    return 0;
}
