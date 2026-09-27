#!/usr/bin/env python3
"""Guard the ninth bounded Phase 69.F public-v1 Channel reference client."""

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
    "function channelQuery(query)",
    "getChannels(options)",
    "'/api/v1/channels'",
    "backendIds",
    "backendIds.length > 16",
    "backendId,channelId",
    "cursor",
    "order=asc",
):
    require(token in client, "Channel reference client contract drifted: " + token)

for forbidden in (
    "/api/vdr/channels",
    "fetchClientChannels",
    "requestJsonWithFallback",
    "requestJsonWithFallbacks",
):
    require(forbidden not in client, "legacy/browser Channel surface entered public reference client: " + forbidden)

server_test = read("api/rest/tests/test_public_channel_collection.cpp")
for token in (
    "/api/v1/channels?backendId=backend-c&backendId=backend-a&backendId=backend-b&limit=2",
    '\"partial\":true',
    '\"code\":\"cursor_expired\"',
    '"sort=channelId"',
    '"order=desc"',
):
    require(token in server_test, "accepted server Channel contract drifted: " + token)

matrix = json.loads(read("docs/development/phase-69f-client-contract-matrix.json"))
require(len(matrix.get("publicV1Resources", [])) == 8, "stable public-v1 resource count drifted")
channel_group = next(
    (g for g in matrix.get("browserClientGroups", []) if g.get("domain") == "channels"),
    None,
)
require(
    channel_group and channel_group.get("publicV1Relation") == "partial-non-equivalent",
    "browser/public Channel non-equivalence drifted",
)
references = matrix.get("publicClientReferences", [])
reference = next(
    (r for r in references if r.get("id") == "reference-js-channel-collection"),
    None,
)
require(reference is not None, "Channel reference slice disappeared")
require(reference.get("path") == "clients/reference-js/public-v1-client.js", "Channel reference path drifted")
require(
    reference.get("resources") == ["GET /api/v1/channels?backendId={backendId}"],
    "Channel reference resource drifted",
)

test = read("clients/reference-js/tests/test_public_v1_channel_client.js")
for token in (
    "backendId=backend-c&backendId=backend-a&backendId=backend-b",
    "sort=backendId,channelId&order=asc",
    "first.meta.partial",
    "backend_unavailable",
    "cursor_expired",
    "assert.strictEqual(requests.length, beforeInvalid + 1)",
):
    require(token in test, "Channel reference regression drifted: " + token)

doc = read("docs/development/phase-69f-public-v1-channel-reference-client.md")
for token in (
    "main=88b7e01d5fb327fea3f54d43f31202dc7e303d98",
    "PR #376",
    "GET /api/v1/channels?backendId=...",
    "1..16",
    "cursor_expired",
    "TimerAssignment and Operation client methods therefore remain separate slices.",
):
    require(token in doc, "Channel reference documentation drifted: " + token)

link = "[Phase 69.F Public-v1 Channel Reference Client]"
require(
    link + "(development/phase-69f-public-v1-channel-reference-client.md)" in read("docs/CURRENT.md"),
    "CURRENT must link Channel reference slice",
)
for path in (
    "docs/development/phase-69-public-api-kickoff.md",
    "docs/development/index.md",
    "docs/development/web-client-api-contract-snapshot.md",
):
    require(
        link + "(phase-69f-public-v1-channel-reference-client.md)" in read(path),
        path + " must link Channel reference slice",
    )

phase_make = read("mk/phase69-public-api-tests.mk")
require("test-phase69f-public-v1-channel-reference-client:" in phase_make, "Channel test target missing")
require("test_public_v1_channel_client.js" in phase_make, "Channel Node regression missing from target")
require("check_phase69f_public_v1_channel_reference_client.py" in phase_make, "Channel guard missing from target")
require(
    "python3 tools/check_phase69f_public_v1_channel_reference_client.py" in read("mk/maintenance-tests.mk"),
    "architecture target must run Channel guard",
)
require(
    "test-phase69f-public-v1-channel-reference-client" in read("mk/test-groups.mk"),
    "fast CI must include Channel reference slice",
)

print("Phase 69.F public-v1 Channel reference-client guard passed.")
print("Reference client preserves explicit federated Channel source semantics.")
print("Bundled browser Channel wrapper remains deliberately non-equivalent.")
