#!/usr/bin/env python3
"""Guard the twelfth bounded Phase 69.F Timer CREATE reference client."""

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
    "function requestTimerCreate(path, options)",
    "submitTimerCreate(options)",
    "headers['If-Match'] = requestOptions.ifMatch",
    "headers['Idempotency-Key'] = requestOptions.idempotencyKey",
    "headers['Content-Type'] = 'application/json'",
    "method: 'POST'",
    "body: '{}'",
    "location: location",
    "etag: entityTag",
):
    require(token in client, "Timer CREATE reference client contract drifted: " + token)

for forbidden in (
    "/api/vdr/timers/actions/create",
    "randomUUID",
    "crypto.random",
    "setTimeout(",
    "setInterval(",
    "requestJsonWithFallback",
    "requestJsonWithFallbacks",
):
    require(forbidden not in client, "retry/generated-key/legacy surface entered Timer CREATE reference client: " + forbidden)

server_test = read("api/rest/tests/test_public_timer_create_admission.cpp")
for token in (
    "accepted.statusCode == 202",
    'accepted.headers.at("Location")',
    'accepted.headers.at("ETag")',
    "captured.expectedAssignmentRevision",
    "captured.idempotencyKey",
    "replay.statusCode == 202",
    "stale.statusCode == 412",
    "revision_conflict",
    "idemConflict.statusCode == 409",
    "idempotency_conflict",
):
    require(token in server_test, "accepted Timer CREATE server contract drifted: " + token)

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
        if r.get("id") == "reference-js-timer-create-admission"
    ),
    None,
)
require(reference is not None, "Timer CREATE reference slice disappeared")
require(reference.get("path") == "clients/reference-js/public-v1-client.js", "Timer CREATE reference path drifted")
require(reference.get("status") == "accepted", "Timer CREATE reference acceptance drifted")
require(
    reference.get("resources") == [
        "POST /api/v1/timer-assignments/{timerAssignmentId}?backend={backendId}"
    ],
    "Timer CREATE reference resource drifted",
)

test = read("clients/reference-js/tests/test_public_v1_timer_create_client.js")
for token in (
    "submitTimerCreate(mutationOptions)",
    "method, 'POST'",
    "options.body, '{}'",
    "options.headers['If-Match']",
    "options.headers['Idempotency-Key']",
    "accepted.location",
    "accepted.data.operationId",
    "revision_conflict",
    "idempotency_conflict",
    "assert.strictEqual(requests.length, beforeInvalid + 2)",
):
    require(token in test, "Timer CREATE reference regression drifted: " + token)

doc = read("docs/development/phase-69f-public-v1-timer-create-reference-client.md")
for token in (
    "main=28eb3301215bea17958c3c1b2f49f1e8c7705445",
    "PR #379",
    "POST /api/v1/timer-assignments/{timerAssignmentId}?backend={backendId}",
    "same If-Match and same Idempotency-Key",
    "does not automatically poll",
    "Operation reconciliation remains",
):
    require(token in doc, "Timer CREATE reference documentation drifted: " + token)

link = "[Phase 69.F Public-v1 Timer CREATE Admission Reference Client]"
require(
    link + "(development/phase-69f-public-v1-timer-create-reference-client.md)" in read("docs/CURRENT.md"),
    "CURRENT must link Timer CREATE reference slice",
)
for path in (
    "docs/development/phase-69-public-api-kickoff.md",
    "docs/development/index.md",
    "docs/development/web-client-api-contract-snapshot.md",
):
    require(
        link + "(phase-69f-public-v1-timer-create-reference-client.md)" in read(path),
        path + " must link Timer CREATE reference slice",
    )

phase_make = read("mk/phase69-public-api-tests.mk")
require(
    "test-phase69f-public-v1-timer-create-reference-client:" in phase_make,
    "Timer CREATE reference target missing",
)
require("test_public_v1_timer_create_client.js" in phase_make, "Timer CREATE Node regression missing")
require(
    "check_phase69f_public_v1_timer_create_reference_client.py" in phase_make,
    "Timer CREATE guard missing",
)
require(
    "python3 tools/check_phase69f_public_v1_timer_create_reference_client.py"
    in read("mk/maintenance-tests.mk"),
    "architecture target must run Timer CREATE reference guard",
)
require(
    "test-phase69f-public-v1-timer-create-reference-client"
    in read("mk/test-groups.mk"),
    "fast CI must include Timer CREATE reference slice",
)

print("Phase 69.F public-v1 Timer CREATE reference-client guard passed.")
print("Mutation safety inputs remain caller-owned and one-shot.")
print("Durable Operation reconciliation remains a separate successor slice.")
