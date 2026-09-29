#!/usr/bin/env python3
"""Guard the thirteenth bounded Phase 69.F durable Operation reference client."""

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
    "function operationItemPath(options)",
    "getOperation(options)",
    "'/api/v1/operations/'",
    "requestRevisioned(",
    "headers['If-None-Match'] = requestOptions.ifNoneMatch",
    "response.status === 304",
):
    require(token in client, "Operation reference client contract drifted: " + token)

for forbidden in (
    "setTimeout(",
    "setInterval(",
    "/api/v1/operations?",
    "/api/vdr/operations",
    "cancelOperation",
    "retryOperation",
):
    require(forbidden not in client, "polling/collection/mutation fallback entered Operation reference client: " + forbidden)

server_test = read("api/rest/tests/test_public_operation_resource.cpp")
for token in (
    "/api/v1/operations/op-1",
    'read.headers.at("ETag")',
    "notModified.statusCode == 304",
    "notModified.body.empty()",
    "malformed.statusCode == 400",
    "hidden.statusCode == 404",
    "missing.statusCode == 404",
    "unavailable.statusCode == 503",
    'result.operation.state = "queued";',
):
    require(token in server_test, "accepted Operation server contract drifted: " + token)

matrix = json.loads(read("docs/development/phase-69f-client-contract-matrix.json"))
require(len(matrix.get("publicV1Resources", [])) == 9, "stable public-v1 resource count drifted")
reference = next(
    (
        r for r in matrix.get("publicClientReferences", [])
        if r.get("id") == "reference-js-operation-item"
    ),
    None,
)
require(reference is not None, "Operation reference slice disappeared")
require(reference.get("path") == "clients/reference-js/public-v1-client.js", "Operation reference path drifted")
require(reference.get("status") == "accepted", "Operation reference acceptance drifted")
require(
    reference.get("resources") == ["GET /api/v1/operations/{operationId}"],
    "Operation reference resource drifted",
)

public_contracts = {
    resource["method"] + " " + resource["template"]
    for resource in matrix.get("publicV1Resources", [])
}
reference_contracts = {
    contract
    for item in matrix.get("publicClientReferences", [])
    for contract in item.get("resources", [])
}
require(reference_contracts == public_contracts, "reference/public-v1 exact coverage drifted")
require(len(reference_contracts) == 8, "reference client must cover exactly eight stable contracts")

candidate = matrix.get("derivedNextRuntimeCandidate", {})
require(
    candidate.get("domain") in {"phase69f-closeout-audit", "phase69-complete"},
    "Operation coverage must lead to or remain inside Phase-69 closeout",
)
require(candidate.get("proposedTemplate") is None, "closeout state must not invent another public route")

test = read("clients/reference-js/tests/test_public_v1_operation_client.js")
for token in (
    "getOperation({",
    "state, 'outcome_unknown'",
    "notModified.status, 304",
    "options.headers['If-None-Match']",
    "invalid_request",
    "not_found",
    "service_unavailable",
    "assert.strictEqual(requests.length, beforeInvalid + 3)",
):
    require(token in test, "Operation reference regression drifted: " + token)

doc = read("docs/development/phase-69f-public-v1-operation-reference-client.md")
for token in (
    "main=afd0e04708622d99a3ee82532e7d8608495d3f6b",
    "PR #380",
    "GET /api/v1/operations/{operationId}",
    "8 stable contracts",
    "0 uncovered stable contracts",
    "phase-69 closeout audit",
):
    require(token.lower() in doc.lower(), "Operation reference documentation drifted: " + token)

link = "[Phase 69.F Public-v1 Operation Reference Client]"
require(
    link + "(development/phase-69f-public-v1-operation-reference-client.md)" in read("docs/CURRENT.md"),
    "CURRENT must link Operation reference slice",
)
for path in (
    "docs/development/phase-69-public-api-kickoff.md",
    "docs/development/index.md",
    "docs/development/web-client-api-contract-snapshot.md",
):
    require(
        link + "(phase-69f-public-v1-operation-reference-client.md)" in read(path),
        path + " must link Operation reference slice",
    )

phase_make = read("mk/phase69-public-api-tests.mk")
require(
    "test-phase69f-public-v1-operation-reference-client:" in phase_make,
    "Operation reference target missing",
)
require("test_public_v1_operation_client.js" in phase_make, "Operation Node regression missing")
require(
    "check_phase69f_public_v1_operation_reference_client.py" in phase_make,
    "Operation reference guard missing",
)
require(
    "python3 tools/check_phase69f_public_v1_operation_reference_client.py"
    in read("mk/maintenance-tests.mk"),
    "architecture target must run Operation guard",
)
require(
    "test-phase69f-public-v1-operation-reference-client"
    in read("mk/test-groups.mk"),
    "fast CI must include Operation reference slice",
)

print("Phase 69.F public-v1 durable Operation reference-client guard passed.")
print("Reference coverage equals all eight stable public-v1 contracts exactly.")
print("Next justified step: fresh 69.F/Phase-69 closeout audit.")
