#!/usr/bin/env python3

from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

paths = {
    "preconditions": ROOT / "core/http/include/PublicResourcePreconditions.h",
    "preconditions_test": ROOT / "core/http/tests/test_public_resource_preconditions.cpp",
    "public_h": ROOT / "api/rest/include/PublicApiRuntime.h",
    "public_cpp": ROOT / "api/rest/src/PublicApiRuntime.cpp",
    "router": ROOT / "api/rest/include/ApiRouter.h",
    "http": ROOT / "core/http/src/TestHttpServer.cpp",
    "security": ROOT / "core/security/include/SecurityHttpGate.h",
    "daemon_init": ROOT / "core/daemon/src/DaemonRuntimeInitialization.cpp",
    "daemon_shutdown": ROOT / "core/daemon/src/DaemonRuntimeShutdown.cpp",
    "public_test": ROOT / "api/rest/tests/test_public_timer_create_admission.cpp",
    "security_test": ROOT / "core/security/tests/test_public_timer_create_security.cpp",
}

contents = {}
for label, path in paths.items():
    if not path.is_file():
        raise SystemExit(
            f"missing Phase-69.C public Timer CREATE admission file: {path.relative_to(ROOT)}")
    contents[label] = path.read_text(encoding="utf-8")

required = {
    "preconditions": [
        "publicStrongEntityTagResourceRevision",
        "publicStrongEntityTag(decoded) != entityTag",
    ],
    "preconditions_test": [
        "publicStrongEntityTagResourceRevision",
        'publicStrongEntityTag("revision:42")',
        '"W/" + publicStrongEntityTag("7")',
    ],
    "public_h": [
        "PublicTimerCreateAdmissionStatus",
        "PublicTimerCreateAdmissionRequest",
        "PublicTimerCreateAdmissionResult",
        "TimerCreateAdmission",
        "registerTimerCreateAdmission",
        "resetTimerCreateAdmission",
        "timerCreateAdmissionConfigured",
        "ifMatch",
        "idempotencyKey",
        "contentType",
        "authorizedBackendId",
    ],
    "public_cpp": [
        '"/api/v1/timer-assignments/"',
        "applicationJsonContentType",
        "JsonSyntaxValidator",
        "emptyJsonObject",
        "publicIdempotencyKeyValid",
        "publicStrongEntityTagResourceRevision",
        '"precondition_required"',
        '"revision_conflict"',
        '"idempotency_conflict"',
        '"generation_conflict"',
        '"read_only_backend"',
        '"backend_unavailable"',
        '"validation_error"',
        '"invalid_json"',
        '"unsupported_media_type"',
        "response.statusCode = 202",
        'response.headers["Location"] = operationPath',
        '"public-api.timer-create-admission"',
        '"GET, POST"',
    ],
    "router": [
        "ifMatch",
        "idempotencyKey",
        "contentType",
        "authorizedBackendRef",
    ],
    "http": [
        '"If-Match"',
        '"Idempotency-Key"',
        '"Content-Type"',
        "gate.authorizationDecision.backendId",
    ],
    "security": [
        "isPublicTimerAssignmentCreate",
        'requestToAuthorize.permission = "timers.create"',
        'requestToAuthorize.action = "timers.create"',
        "publicTimerAssignmentBackendId",
        "isProtectedMutation",
        "browserSessionAuthenticator_->verifyCsrf(request.headers)",
    ],
    "daemon_init": [
        "registerTimerCreateAdmission",
        "backendAccessPolicy_->canWriteToBackend",
        "nativeTimerCreateAdmissionService_->admit",
        "NativeTimerCreateAdmissionStatus::prepared",
        "NativeTimerCreateAdmissionStatus::replayed",
        "PublicTimerCreateAdmissionStatus::readOnlyBackend",
        "PublicTimerCreateAdmissionStatus::revisionConflict",
        "PublicTimerCreateAdmissionStatus::idempotencyConflict",
    ],
    "daemon_shutdown": [
        "resetTimerCreateAdmission",
        "resetTimerAssignmentLookup",
        "resetOperationLookup",
    ],
    "public_test": [
        "statusCode == 202",
        'headers.at("Location")',
        "expectedAssignmentRevision == \"7\"",
        "statusCode == 428",
        "statusCode == 415",
        "statusCode == 422",
        "PublicTimerCreateAdmissionStatus::replayed",
        "PublicTimerCreateAdmissionStatus::revisionConflict",
        "PublicTimerCreateAdmissionStatus::idempotencyConflict",
    ],
    "security_test": [
        'constexpr const char* Permission = "timers.create"',
        "decision.protectedMutation",
        "csrf_validation_failed",
        "backend_scope_denied",
        "invalid_backend_scope",
        "decision.operationId.empty()",
        "http_status_202",
        "event.operationId.empty()",
    ],
}

for label, tokens in required.items():
    for token in tokens:
        if token not in contents[label]:
            raise SystemExit(
                f"missing public Timer CREATE admission marker in {label}: {token}")

# Public HTTP consumes a backend-neutral callback contract only. Native/domain
# implementation classes and backend-native desired fields stay below it.
for label in ["public_h", "public_cpp", "security"]:
    for forbidden in [
        "NativeTimerCreateAdmissionService",
        "NativeTimerCreateOperationPreparationService",
        "TimerAssignmentFulfillmentService",
        "NativeTimerBindingRepository",
        "NativeTimerSpecification",
        "nativeTimerBindingId",
        "desiredNativeTimerSpecification",
        "generateMutationOperationId",
        "generateNativeTimerBindingId",
    ]:
        if forbidden in contents[label]:
            raise SystemExit(
                f"native/domain Timer CREATE authority leaked into {label}: {forbidden}")

# Backend authorization must use the explicit validated query scope. Public API
# receives the already-authorized scope and does not reparse or trust the body.
if 'queryStringValue' in contents["public_cpp"]:
    raise SystemExit(
        "PublicApiRuntime reparses backend query scope instead of consuming SecurityHttpGate authority")
if 'jsonStringValue(request.body, "backendId")' in contents["daemon_init"]:
    raise SystemExit("daemon public CREATE callback trusts request-body backend scope")

security = contents["security"]
create_pos = security.find("const bool isPublicTimerAssignmentCreate")
protected_pos = security.find("const bool isProtectedMutation")
permission_pos = security.find(
    'requestToAuthorize.permission = "timers.create"',
    protected_pos)
if not (0 <= create_pos < protected_pos < permission_pos):
    raise SystemExit(
        "public Timer CREATE must be classified before protected-mutation authorization")

# Backend write policy is a separate server-side gate before the atomic domain
# admission. It must not be skipped or evaluated after admission.
daemon = contents["daemon_init"]
registration_pos = daemon.find("registerTimerCreateAdmission")
write_policy_pos = daemon.find(
    "backendAccessPolicy_->canWriteToBackend",
    registration_pos)
admit_pos = daemon.find(
    "nativeTimerCreateAdmissionService_->admit",
    registration_pos)
if not (0 <= registration_pos < write_policy_pos < admit_pos):
    raise SystemExit(
        "public CREATE order must be callback -> backend write policy -> atomic admission")

# This slice deliberately ends at durable admission. Agent command reservation,
# dispatch claim and activation remain dormant.
for forbidden_call in [
    "backendAgentNativeTimerCreateReservationService_->reserve(",
    "nativeTimerCreateDispatchService_->claimAfterReservation(",
    "backendAgentNativeTimerCreateActivationService_->activateDispatching(",
]:
    if forbidden_call in daemon:
        raise SystemExit(
            "public Timer CREATE admission activated native dispatch prematurely: "
            + forbidden_call)

# The stable pre-v1 Timer CREATE surface remains a separate compatibility route;
# it is not renamed or silently declared public-v1.
if 'path == "/api/vdr/timers/actions/create"' not in security:
    raise SystemExit("pre-v1 Timer CREATE compatibility route changed unexpectedly")

# The generated operation ID is intentionally unavailable at the SecurityGate
# authorization point. Phase-62/ADR-0049 permit optional operation correlation
# there; no request/response body parsing is introduced to fake one.
if 'jsonStringValue(request.body, "operationId")' not in security:
    raise SystemExit("existing protected-mutation operation correlation contract changed unexpectedly")
if "decision.operationId.empty()" not in contents["security_test"]:
    raise SystemExit("server-issued operation audit boundary is not regression-tested")

print("Phase-69.C public Timer CREATE admission check passed")
print("Boundary: authorized HTTP admission -> backend write policy -> atomic durable operation; Agent dispatch remains closed")
