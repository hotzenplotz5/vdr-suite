#!/usr/bin/env python3
"""Guard the tenth bounded Phase 69.F TimerAssignment collection reference client."""

import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

def read(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")

def require(condition: bool, message: str) -> None:
    if not condition:
        raise SystemExit(message)

client = read("clients/reference-js/public-v1-client.js")
for token in (
    "function timerAssignmentCollectionQuery(query)",
    "getTimerAssignments(options)",
    "'/api/v1/timer-assignments'",
    "params.set('backend', query.backendId)",
    "timerAssignmentId",
    "params.set('cursor', query.cursor)",
    "params.set('order', query.order)",
):
    require(token in client, "TimerAssignment collection client contract drifted: " + token)

for forbidden in (
    "/api/vdr/timers",
    "/api/vdr/timers/live",
    "NativeTimerBinding",
    "requestJsonWithFallback",
    "requestJsonWithFallbacks",
):
    require(forbidden not in client, "native/legacy Timer surface entered public reference client: " + forbidden)

server_test = read("api/rest/tests/test_public_timer_assignment_collection.cpp")
for token in (
    "/api/v1/timer-assignments?backend=backend-one&limit=2",
    r'\"partial\":false',
    'first.headers.find("ETag") == first.headers.end()',
    '"sort=state"',
    '"order=desc"',
    '"offset=1"',
    r'\"code\":\"service_unavailable\"',
):
    require(token in server_test, "accepted TimerAssignment collection contract drifted: " + token)

matrix = json.loads(read("docs/development/phase-69f-client-contract-matrix.json"))
require(len(matrix.get("publicV1Resources", [])) == 8, "stable public-v1 resource count drifted")
timer_group = next(
    (g for g in matrix.get("browserClientGroups", []) if g.get("domain") == "timers"),
    None,
)
require(
    timer_group and timer_group.get("publicV1Relation") == "partial-non-equivalent",
    "browser/native Timer public non-equivalence drifted",
)
reference = next(
    (
        r for r in matrix.get("publicClientReferences", [])
        if r.get("id") == "reference-js-timer-assignment-collection"
    ),
    None,
)
require(reference is not None, "TimerAssignment collection reference disappeared")
require(reference.get("path") == "clients/reference-js/public-v1-client.js", "TimerAssignment collection reference path drifted")
require(reference.get("status") == "accepted", "TimerAssignment collection reference acceptance drifted")
require(
    reference.get("resources") == ["GET /api/v1/timer-assignments?backend={backendId}"],
    "TimerAssignment collection reference resource drifted",
)

test = read("clients/reference-js/tests/test_public_v1_timer_assignment_collection_client.js")
for token in (
    "backend=backend-one&limit=2&cursor=ta1_after-previous&sort=timerAssignmentId&order=asc",
    "first.meta.partial",
    "service_unavailable",
    "assert.strictEqual(requests.length, beforeInvalid + 1)",
):
    require(token in test, "TimerAssignment collection regression drifted: " + token)

doc = read("docs/development/phase-69f-public-v1-timer-assignment-collection-reference-client.md")
for token in (
    "main=7704d3826aa426a4cb5e48c4ba07e372614b86fe",
    "PR #377",
    "GET /api/v1/timer-assignments?backend={backendId}",
    "no collection ETag",
    "TimerAssignment item/ETag/conditional GET",
    "add Operation reads",
):
    require(token in doc, "TimerAssignment collection documentation drifted: " + token)

link = "[Phase 69.F Public-v1 TimerAssignment Collection Reference Client]"
require(
    link + "(development/phase-69f-public-v1-timer-assignment-collection-reference-client.md)" in read("docs/CURRENT.md"),
    "CURRENT must link TimerAssignment collection reference slice",
)
for path in (
    "docs/development/phase-69-public-api-kickoff.md",
    "docs/development/index.md",
    "docs/development/web-client-api-contract-snapshot.md",
):
    require(
        link + "(phase-69f-public-v1-timer-assignment-collection-reference-client.md)" in read(path),
        path + " must link TimerAssignment collection reference slice",
    )

phase_make = read("mk/phase69-public-api-tests.mk")
require(
    "test-phase69f-public-v1-timer-assignment-collection-reference-client:" in phase_make,
    "TimerAssignment collection test target missing",
)
require(
    "test_public_v1_timer_assignment_collection_client.js" in phase_make,
    "TimerAssignment collection Node regression missing",
)
require(
    "check_phase69f_public_v1_timer_assignment_collection_reference_client.py" in phase_make,
    "TimerAssignment collection guard missing",
)
require(
    "python3 tools/check_phase69f_public_v1_timer_assignment_collection_reference_client.py"
    in read("mk/maintenance-tests.mk"),
    "architecture target must run TimerAssignment collection guard",
)
require(
    "test-phase69f-public-v1-timer-assignment-collection-reference-client"
    in read("mk/test-groups.mk"),
    "fast CI must include TimerAssignment collection slice",
)

print("Phase 69.F public-v1 TimerAssignment collection reference-client guard passed.")
print("Reference client preserves the single-backend Suite-owned keyset collection.")
print("Native/browser Timer semantics remain deliberately non-equivalent.")
