#!/usr/bin/env python3

from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

paths = {
    "public_h": ROOT / "api/rest/include/PublicApiRuntime.h",
    "public_cpp": ROOT / "api/rest/src/PublicApiRuntime.cpp",
    "router": ROOT / "api/rest/include/ApiRouter.h",
    "http": ROOT / "core/http/src/TestHttpServer.cpp",
    "security": ROOT / "core/security/include/SecurityHttpGate.h",
    "resource_test": ROOT / "api/rest/tests/test_public_timer_assignment_resource.cpp",
    "security_test": ROOT / "core/security/tests/test_public_timer_assignment_read_security.cpp",
    "inventory": ROOT / "tools/check_phase69_public_api_inventory.py",
}

contents = {}
for label, path in paths.items():
    if not path.is_file():
        raise SystemExit(
            f"missing Phase-69.C public TimerAssignment file: {path.relative_to(ROOT)}")
    contents[label] = path.read_text(encoding="utf-8")

required = {
    "public_h": [
        "authorizedBackendId",
        "PublicTimerAssignmentLookupResult",
    ],
    "public_cpp": [
        '"/api/v1/timer-assignments/"',
        "publicTimerAssignmentPath",
        "publicTimerAssignmentResponse",
        "lookupTimerAssignment(",
        "authorizedBackendId",
        "publicStrongEntityTag(",
        "publicEvaluateIfNoneMatch(",
        '"public-api.timer-assignments-read"',
        "PublicTimerAssignmentLookupStatus::notFound",
    ],
    "router": [
        "authorizedBackendRef",
        "ifNoneMatch,",
        "authorizedBackendRef))",
    ],
    "http": [
        'requestHeaderValue(',
        '"If-None-Match"',
        "gate.authorizationDecision.backendId",
    ],
    "security": [
        '"/api/v1/timer-assignments/"',
        'queryStringValue(request.path, "backend")',
        'timerReadRequest.permission = "timers.view"',
        'timerReadRequest.action = "timers.view"',
        "authorizationService_.authorize(",
        'decision.reasonCode == "invalid_backend_scope"',
        "gate.authorizationDecision = decision",
    ],
    "resource_test": [
        "statusCode == 200",
        "statusCode == 304",
        "statusCode == 400",
        "statusCode == 404",
        "statusCode == 405",
        'headers.at("ETag")',
        '"resourceRevision"',
        '"nativeTimerBindingId"',
        '"public-api.timer-assignments-read"',
        "collectionStillClosed",
    ],
    "security_test": [
        'constexpr const char* Permission = "timers.view"',
        "backend_scope_denied",
        "invalid_backend_scope",
        "permission_denied",
        "statusCode == 401",
    ],
    "inventory": [
        '"/api/v1/timer-assignments/"',
    ],
}

for label, tokens in required.items():
    for token in tokens:
        if token not in contents[label]:
            raise SystemExit(
                f"missing public TimerAssignment contract marker in {label}: {token}")

if "queryStringValue" in contents["public_cpp"]:
    raise SystemExit(
        "PublicApiRuntime must consume an authorized backend scope, not reparse query parameters")

for token in ["Idempotency-Key", "timer.create", "timers.create"]:
    if token in contents["public_cpp"] or token in contents["public_h"]:
        raise SystemExit(
            f"public TimerAssignment read slice opened mutation semantics prematurely: {token}")

print("Phase-69.C public TimerAssignment resource check passed")
print("Boundary: authenticated timers.view item read + opaque ETag; collection and mutations closed")
