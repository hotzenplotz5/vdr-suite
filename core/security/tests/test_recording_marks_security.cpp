#include "SecurityHttpGateBrowserTestFixture.h"

#include <cassert>
#include <string>

int main()
{
    SecurityHttpGateBrowserTestFixture fixture;

    HttpServerRequest missingCsrf = fixture.mutationRequest(
        "/api/vdr/recordings/marks",
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
        "/api/vdr/recordings/marks",
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
        "recordings.marks.modify",
        "default"));

    HttpServerRequest allowed = fixture.mutationRequest(
        "/api/vdr/recordings/marks",
        "default");
    fixture.addBrowserAuthentication(allowed, true);

    const SecurityGateDecision allowedDecision =
        fixture.gate.evaluate(allowed);
    assert(allowedDecision.allowed);
    assert(allowedDecision.protectedMutation);
    assert(allowedDecision.authorizationDecision.allowed);
    assert(allowedDecision.authorizationDecision.permission ==
        "recordings.marks.modify");
    assert(allowedDecision.authorizationDecision.backendId == "default");
    assert(allowedDecision.authorizationDecision.action ==
        "recordings.marks.modify");
    assert(allowedDecision.operationId == "phase62-test-operation");

    HttpServerRequest wrongScope = fixture.mutationRequest(
        "/api/vdr/recordings/marks",
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
        "/api/vdr/recordings/marks",
        "default");
    fixture.addBrowserAuthentication(readOnly, true);

    const SecurityGateDecision readOnlyDecision =
        fixture.gate.evaluate(readOnly);
    assert(!readOnlyDecision.allowed);
    assert(readOnlyDecision.protectedMutation);
    assert(readOnlyDecision.rejection.statusCode == 403);
    assert(readOnlyDecision.rejection.body.find(
        "role_read_only") != std::string::npos);

    return 0;
}