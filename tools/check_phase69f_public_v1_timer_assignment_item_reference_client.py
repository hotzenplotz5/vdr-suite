#!/usr/bin/env python3
"""Guard the eleventh bounded Phase 69.F TimerAssignment item reference client."""

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
    "function timerAssignmentItemPath(options)",
    "function requestRevisioned(path, options)",
    "getTimerAssignment(options)",
    "'/api/v1/timer-assignments/'",
    "headers['If-None-Match'] = requestOptions.ifNoneMatch",
    "response.status === 304",
    "etag: entityTag",
    "data: null",
):
    require(token in client, "TimerAssignment item client contract drifted: " + token)

for forbidden in (
    "/api/vdr/timers",
    "/api/vdr/timers/live",
    "NativeTimerBinding",
    "requestJsonWithFallback",
    "requestJsonWithFallbacks",
):
    require(forbidden not in client, "native/legacy surface entered public reference client: " + forbidden)

revisioned_start = client.find("function requestRevisioned(path, options)")
revisioned_end = client.find("function requestTimerCreate(path, options)", revisioned_start)
require(revisioned_start >= 0 and revisioned_end > revisioned_start, "revisioned GET helper boundary drifted")
revisioned_get = client[revisioned_start:revisioned_end]
require("If-Match" not in revisioned_get, "TimerAssignment GET must not send mutation If-Match")

server_test = read("api/rest/tests/test_public_timer_assignment_resource.cpp")
for token in (
    "/api/v1/timer-assignments/assignment:one?backend=backend-one",
    'resource.headers.at("ETag")',
    "notModified.statusCode == 304",
    "notModified.body.empty()",
    "malformedCondition.statusCode == 400",
    "hiddenWrongBackend.statusCode == 404",
):
    require(token in server_test, "accepted TimerAssignment item server contract drifted: " + token)

matrix = json.loads(read("docs/development/phase-69f-client-contract-matrix.json"))
require(len(matrix.get("publicV1Resources", [])) >= 8, "Phase-69 stable public-v1 baseline disappeared")
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
        if r.get("id") == "reference-js-timer-assignment-item"
    ),
    None,
)
require(reference is not None, "TimerAssignment item reference disappeared")
require(reference.get("path") == "clients/reference-js/public-v1-client.js", "TimerAssignment item reference path drifted")
require(reference.get("status") == "accepted", "TimerAssignment item reference acceptance drifted")
require(
    reference.get("resources") == [
        "GET /api/v1/timer-assignments/{timerAssignmentId}?backend={backendId}"
    ],
    "TimerAssignment item reference resource drifted",
)

test = read("clients/reference-js/tests/test_public_v1_timer_assignment_item_client.js")
for token in (
    "https://suite.example/api/v1/timer-assignments/assignment:one?backend=backend-one",
    "first.etag",
    "requests[1].options.headers['If-None-Match']",
    "notModified.status, 304",
    "invalid_request",
    "not_found",
    "assert.strictEqual(requests.length, beforeInvalid + 2)",
):
    require(token in test, "TimerAssignment item regression drifted: " + token)

doc = read("docs/development/phase-69f-public-v1-timer-assignment-item-reference-client.md")
for token in (
    "main=7a2c8f651ed895148d5520f2ac5820781b917e49",
    "PR #378",
    "If-None-Match",
    "{status: 304, etag, data: null}",
    "add mutation `If-Match`",
    "read durable Operations",
):
    require(token in doc, "TimerAssignment item documentation drifted: " + token)

link = "[Phase 69.F Public-v1 TimerAssignment Item Reference Client]"
require(
    link + "(development/phase-69f-public-v1-timer-assignment-item-reference-client.md)" in read("docs/CURRENT.md"),
    "CURRENT must link TimerAssignment item reference slice",
)
for path in (
    "docs/development/phase-69-public-api-kickoff.md",
    "docs/development/index.md",
    "docs/development/web-client-api-contract-snapshot.md",
):
    require(
        link + "(phase-69f-public-v1-timer-assignment-item-reference-client.md)" in read(path),
        path + " must link TimerAssignment item reference slice",
    )

phase_make = read("mk/phase69-public-api-tests.mk")
require(
    "test-phase69f-public-v1-timer-assignment-item-reference-client:" in phase_make,
    "TimerAssignment item test target missing",
)
require(
    "test_public_v1_timer_assignment_item_client.js" in phase_make,
    "TimerAssignment item Node regression missing",
)
require(
    "check_phase69f_public_v1_timer_assignment_item_reference_client.py" in phase_make,
    "TimerAssignment item guard missing",
)
require(
    "python3 tools/check_phase69f_public_v1_timer_assignment_item_reference_client.py"
    in read("mk/maintenance-tests.mk"),
    "architecture target must run TimerAssignment item guard",
)
require(
    "test-phase69f-public-v1-timer-assignment-item-reference-client"
    in read("mk/test-groups.mk"),
    "fast CI must include TimerAssignment item slice",
)

print("Phase 69.F public-v1 TimerAssignment item reference-client guard passed.")
print("Reference client preserves opaque ETag and If-None-Match conditional-read semantics.")
print("Mutation and native/browser Timer semantics remain outside this slice.")
