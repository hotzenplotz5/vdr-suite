#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

FILES = {
    "runtime_h": ROOT / "api/rest/include/PublicApiRuntime.h",
    "runtime_cpp": ROOT / "api/rest/src/PublicApiRuntime.cpp",
    "repository_h": ROOT / "core/timers/include/TimerAssignmentRepository.h",
    "repository_cpp": ROOT / "core/timers/src/TimerAssignmentRepository.cpp",
    "read_h": ROOT / "core/timers/include/TimerAssignmentReadService.h",
    "read_cpp": ROOT / "core/timers/src/TimerAssignmentReadService.cpp",
    "security": ROOT / "core/security/include/SecurityHttpGate.h",
    "daemon_init": ROOT / "core/daemon/src/DaemonRuntimeInitialization.cpp",
    "daemon_shutdown": ROOT / "core/daemon/src/DaemonRuntimeShutdown.cpp",
    "runtime_test": ROOT / "api/rest/tests/test_public_timer_assignment_collection.cpp",
    "security_test": ROOT / "core/security/tests/test_public_timer_assignment_collection_security.cpp",
    "domain_test": ROOT / "core/timers/tests/test_timer_assignment_collection_read_service.cpp",
    "inventory": ROOT / "tools/check_phase69_public_api_inventory.py",
    "doc": ROOT / "docs/development/phase-69d-timer-assignment-collection.md",
    "current": ROOT / "docs/CURRENT.md",
}

def read(name):
    path = FILES[name]
    if not path.is_file():
        raise SystemExit(f"missing Phase 69.D collection file: {path.relative_to(ROOT)}")
    return path.read_text(encoding="utf-8")

def require(text, token, label):
    if token not in text:
        raise SystemExit(f"missing {label}: {token}")

def forbid(text, token, label):
    if token in text:
        raise SystemExit(f"forbidden {label}: {token}")

runtime_h = read("runtime_h")
for token in (
    "PublicTimerAssignmentCollectionRequest",
    "PublicTimerAssignmentCollectionResult",
    "registerTimerAssignmentCollectionLookup",
    "resetTimerAssignmentCollectionLookup",
):
    require(runtime_h, token, "PublicApiRuntime collection callback")

runtime_cpp = read("runtime_cpp")
for token in (
    'PublicTimerAssignmentCollectionPath =\n    "/api/v1/timer-assignments"',
    "PublicTimerAssignmentDefaultLimit = 50U",
    "PublicTimerAssignmentMaximumLimit = 100U",
    'PublicTimerAssignmentCollectionSort =\n    "timerAssignmentId"',
    'PublicTimerAssignmentCollectionOrder = "asc"',
    'PublicTimerAssignmentCursorPrefix = "ta1_"',
    "decodePublicTimerAssignmentCursor",
    r'\"meta\":{\"partial\":false}',
    "lookupTimerAssignmentCollection(request)",
    "The collection cursor is invalid for this actor, backend, or ordering.",
):
    require(runtime_cpp, token, "public collection contract")
forbid(runtime_cpp, "PublicTimerAssignmentCollectionOffset", "offset pagination authority")

repository_cpp = read("repository_cpp")
for token in (
    "listForBackendAfter",
    '"WHERE backend_id=? "',
    '"AND timer_assignment_id>? "',
    '"ORDER BY timer_assignment_id ASC LIMIT ?;"',
    "kMaximumBoundedPageFetch = 101U",
):
    require(repository_cpp, token, "repository keyset boundary")

read_cpp = read("read_cpp")
for token in (
    "listForBackend(",
    "limit + 1U",
    "result.assignments.resize(limit)",
    "result.hasMore = true",
):
    require(read_cpp, token, "read facade bounded look-ahead")

security = read("security")
for token in (
    'publicTimerAssignmentCollection =\n            "/api/v1/timer-assignments"',
    "isPublicTimerAssignmentCollection",
    'timerReadRequest.permission = "timers.view"',
):
    require(security, token, "existing backend-scoped security reuse")

daemon_init = read("daemon_init")
require(
    daemon_init,
    "registerTimerAssignmentCollectionLookup",
    "daemon collection composition",
)
daemon_shutdown = read("daemon_shutdown")
require(
    daemon_shutdown,
    "resetTimerAssignmentCollectionLookup",
    "daemon collection reset",
)

inventory = read("inventory")
require(
    inventory,
    '"/api/v1/timer-assignments",',
    "stable collection inventory",
)

runtime_test = read("runtime_test")
for token in (
    r'\"hasMore\":true',
    r'\"partial\":false',
    "wrongActor",
    "wrongBackend",
    "malformedCursor",
    '"offset=1"',
    'first.headers.find("ETag") == first.headers.end()',
):
    require(runtime_test, token, "runtime regression coverage")

security_test = read("security_test")
for token in (
    'Permission = "timers.view"',
    "backend_scope_denied",
    "invalid_backend_scope",
):
    require(security_test, token, "security regression coverage")

domain_test = read("domain_test")
for token in (
    '"assignment:a"',
    '"assignment:b"',
    '"assignment:c"',
    "first.hasMore",
    "!second.hasMore",
):
    require(domain_test, token, "domain pagination coverage")

doc = read("doc")
for token in (
    "GET /api/v1/timer-assignments",
    "timerAssignmentId ASC",
    "default limit = 50",
    "maximum limit = 100",
    "opaque",
    "meta.partial=false",
    "single-backend",
    "503 service_unavailable",
    "no collection ETag",
    "not a snapshot-isolation contract",
):
    require(doc, token, "documented Phase 69.D contract")

current = read("current")
require(
    current,
    "[Phase 69.D first public collection]",
    "CURRENT Phase 69.D navigation",
)

print("Phase 69.D TimerAssignment collection guard passed.")
print("Boundary: one backend-scoped Suite-owned keyset collection; no federation, legacy fallback, Timer mutation, Home or LiveTV change.")
