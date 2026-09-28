#!/usr/bin/env python3
"""Guard the eighth bounded Phase 69.F public-v1 discovery reference client."""

import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

def read(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")

def require(condition: bool, message: str) -> None:
    if not condition:
        raise SystemExit(message)

client = read("clients/reference-js/public-v1-client.js")
routes = set(re.findall(r"['\"](/api/[^'\"]+)['\"]", client))
require(
    {"/api/v1", "/api/v1/capabilities", "/api/v1/backends"}.issubset(routes),
    "accepted discovery routes disappeared: " + repr(sorted(routes)),
)
require(
    all(route.startswith("/api/v1") for route in routes),
    "non-public-v1 route entered reference client: " + repr(sorted(routes)),
)
for forbidden in (
    "/api/vdr/",
    "/api/security/browser-sessions",
    "/api/epg/",
    "/api/recordings/",
    "requestJsonWithFallback",
    "requestJsonWithFallbacks",
):
    require(forbidden not in client, "private/legacy fallback surface entered reference client: " + forbidden)

for token in (
    "createClient",
    "VdrSuitePublicClientError",
    "isPublicClientError",
    "normalized.fetch",
    "X-Correlation-ID",
    "limit",
    "cursor",
    "backendId",
    "order",
):
    require(token in client, "reference client contract drifted: " + token)

runtime = read("api/rest/src/PublicApiRuntime.cpp")
for route in ("/api/v1", "/api/v1/capabilities", "/api/v1/backends"):
    require(route in runtime, "reference client route missing from public runtime: " + route)

matrix = json.loads(read("docs/development/phase-69f-client-contract-matrix.json"))
require(len(matrix.get("publicV1Resources", [])) == 8, "stable public-v1 contract count drifted")
references = matrix.get("publicClientReferences", [])
reference = next((r for r in references if r.get("id") == "reference-js-discovery"), None)
require(reference is not None, "accepted discovery reference disappeared")
require(reference.get("path") == "clients/reference-js/public-v1-client.js", "reference path drifted")
require(reference.get("status") == "accepted", "discovery reference acceptance drifted")
require(
    set(reference.get("resources", [])) == {
        "GET /api/v1",
        "GET /api/v1/capabilities",
        "GET /api/v1/backends",
    },
    "reference discovery resource set drifted",
)

browser_client = read("web/frontend/api/client-api.js")
require("/api/v1" not in browser_client, "non-equivalent bundled browser was silently migrated to public-v1")

test = read("clients/reference-js/tests/test_public_v1_discovery_client.js")
for token in (
    "https://suite.example/api/v1",
    "https://suite.example/api/v1/capabilities",
    "https://suite.example/api/v1/backends?limit=25&cursor=be1_after-a&sort=backendId&order=asc",
    "backend_unavailable",
    "assert.strictEqual(requests.length, beforeInvalid + 1)",
):
    require(token in test, "reference-client regression drifted: " + token)

doc = read("docs/development/phase-69f-public-v1-discovery-reference-client.md")
for token in (
    "main=19077e7110fdb8111aead43e15b6962767ac6234",
    "PR #375",
    "GET /api/v1",
    "GET /api/v1/capabilities",
    "GET /api/v1/backends",
    "reference client",
    "No browser Web Client API migration.",
):
    require(token in doc, "reference-client documentation drifted: " + token)

link = "[Phase 69.F Public-v1 Discovery Reference Client]"
require(link + "(development/phase-69f-public-v1-discovery-reference-client.md)" in read("docs/CURRENT.md"), "CURRENT must link reference client slice")
for path in (
    "docs/development/phase-69-public-api-kickoff.md",
    "docs/development/index.md",
    "docs/development/web-client-api-contract-snapshot.md",
):
    require(link + "(phase-69f-public-v1-discovery-reference-client.md)" in read(path), path + " must link reference client slice")

phase_make = read("mk/phase69-public-api-tests.mk")
for token in (
    "test-phase69f-public-v1-discovery-reference-client:",
    "test_public_v1_discovery_client.js",
    "check_phase69f_public_v1_discovery_reference_client.py",
):
    require(token in phase_make, "reference-client Make wiring drifted: " + token)

maintenance = read("mk/maintenance-tests.mk")
require("python3 tools/check_phase69f_public_v1_discovery_reference_client.py" in maintenance, "architecture group must run reference-client guard")
require("test-phase69f-public-v1-discovery-reference-client" in maintenance, "phase group must include reference-client slice")
require("test-phase69f-public-v1-discovery-reference-client" in read("mk/test-groups.mk"), "fast CI must include reference-client slice")

print("Phase 69.F public-v1 discovery reference-client guard passed.")
print("Reference client consumes only root, capabilities and Backend discovery.")
print("Bundled browser remains on its explicitly non-equivalent transition contracts.")
