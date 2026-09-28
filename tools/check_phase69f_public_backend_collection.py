#!/usr/bin/env python3
"""Guard the fifth bounded Phase 69.F public Backend collection slice."""

import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

def read(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")

def require(condition: bool, message: str) -> None:
    if not condition:
        raise SystemExit(message)

runtime_h = read("api/rest/include/PublicApiRuntime.h")
runtime = read("api/rest/src/PublicApiRuntime.cpp")
security = read("core/security/include/SecurityHttpGate.h")
daemon = read("core/daemon/src/DaemonRuntimeInitialization.cpp")
shutdown = read("core/daemon/src/DaemonRuntimeShutdown.cpp")
legacy_serializer = read("core/vdr/src/BackendRegistryJsonSerializer.cpp")
inventory = read("tools/check_phase69_public_api_inventory.py")
legacy_guard = read("tools/check_phase69e_legacy_route_classification.py")
matrix = json.loads(read("docs/development/phase-69f-client-contract-matrix.json"))
matrix_guard = read("tools/check_phase69f_client_contract_matrix.py")
doc = read("docs/development/phase-69f-public-backend-collection.md")

for token in (
    "PublicBackendCollectionStatus",
    "PublicBackendCollectionItem",
    "PublicBackendCollectionRequest",
    "PublicBackendCollectionResult",
    "BackendCollectionLookup",
    "registerBackendCollectionLookup",
    "resetBackendCollectionLookup",
    "backendCollectionLookupConfigured",
):
    require(token in runtime_h, "public Backend runtime header drifted: " + token)

for token in (
    'PublicBackendCollectionPath =\n    "/api/v1/backends"',
    'PublicBackendDefaultLimit = 50U',
    'PublicBackendMaximumLimit = 100U',
    'PublicBackendCollectionSort = "backendId"',
    'PublicBackendCollectionOrder = "asc"',
    'PublicBackendCursorPrefix = "be1_"',
    'PublicBackendCursorPayloadVersion =\n    "backends/1|"',
    "parsePublicBackendCollectionQuery",
    "normalizedPublicBackendAuthorizationScope",
    "publicBackendCursor",
    "decodePublicBackendCursor",
    "publicBackendCollectionTarget",
    "publicBackendCollectionResponse",
    "registerBackendCollectionLookup",
    "lookupBackendCollection",
    "public-api.backends-read",
    "backends-read",
):
    require(token in runtime, "public Backend runtime drifted: " + token)

response_start = runtime.find("ApiResponse publicBackendCollectionResponse(")
response_end = runtime.find("ApiResponse publicChannelCollectionResponse(", response_start)
require(response_start >= 0 and response_end > response_start, "cannot bound public Backend response")
response_body = runtime[response_start:response_end]
for required in (
    "backendId",
    "name",
    "enabled",
    "online",
    "partial",
):
    require(required in response_body, "public Backend representation misses: " + required)
for forbidden in (
    "frontendSelector",
    "backendType",
    "restfulapi",
    "accessMode",
    "canWrite",
    "capabilities",
    "connection",
    "host",
    "port",
):
    require(forbidden not in response_body, "public Backend response leaked legacy/private field: " + forbidden)
require("ETag" not in response_body, "Backend collection must not invent an ETag")

for token in (
    'isPublicBackendCollection =\n            path == "/api/v1/backends"',
    "isPublicBackendRead",
    'grant.permission == "role.read-only"',
    'decision.action = "backends.discover"',
    "gate.authorizedBackendIds =",
):
    require(token in security, "Backend discovery security boundary drifted: " + token)
require('"backends.view"' not in security, "Backend discovery must not invent backends.view")
require(
    'grant.permission == "role.read-only" ||\n                    grant.backendId.empty()' in security,
    "role.read-only must remain excluded from positive Backend discovery scopes",
)

for token in (
    "registerBackendCollectionLookup",
    "backendRegistryService_->listBackends()",
    "request.authorizedBackendIds",
    'std::string("*")',
    "item.backendId = backend.backendId",
    "item.name = backend.backendName",
    "item.enabled = backend.enabled",
    "item.online = backend.online",
):
    require(token in daemon, "Backend collection daemon composition drifted: " + token)

require(
    "resetBackendCollectionLookup()" in shutdown,
    "Backend collection lookup must reset during daemon shutdown",
)

for token in (
    "frontendSelector",
    "accessMode",
    "canWrite",
    "capabilities",
):
    require(token in legacy_serializer, "legacy Backend serializer proof drifted: " + token)
require(
    "BackendRegistryJsonSerializer" not in response_body,
    "public Backend response must not reuse legacy serializer",
)

require(
    '"/api/v1/backends",' in inventory,
    "Phase 69 inventory must include /api/v1/backends",
)
require(
    'len(EXPECTED_ROUTE_LITERALS) == 125' in legacy_guard
    and 'len(EXPECTED_PUBLIC_V1_ROUTE_LITERALS) == 7' in legacy_guard,
    "69.E successor-aware route counts drifted",
)

resources = {
    (item.get("method"), item.get("template"))
    for item in matrix.get("publicV1Resources", [])
}
require(
    ("GET", "/api/v1/backends") in resources,
    "client matrix must include stable public Backend collection",
)
backend_group = next(
    (group for group in matrix.get("browserClientGroups", [])
     if group.get("domain") == "backends"),
    None,
)
require(
    backend_group is not None
    and backend_group.get("publicV1Relation") == "partial-non-equivalent",
    "browser Backend group must remain non-equivalent to public v1",
)
candidate = matrix.get("derivedNextRuntimeCandidate", {})
require(
    candidate.get("domain") in {"phase69f-closeout-audit", "phase69-complete"}
    and candidate.get("proposedTemplate") is None,
    "complete public-v1 reference coverage must allow closeout/final state without a preselected route",
)
require(
    "Stable public-v1 method/resource contracts: 8." in matrix_guard,
    "client matrix guard must count the Backend contract",
)

for token in (
    "main=ca7ddce8c1f4dcd94960c87d49c87533c5b0630b",
    "PR #372",
    "GET /api/v1/backends",
    "does **not** invent a",
    "backends.view",
    "role.read-only",
    "backendId ASC",
    "be1_",
    "409 cursor_expired",
    "frontendSelector",
    "deprecatedAliases",
    "Real-yaVDR",
):
    require(token in doc, "public Backend documentation drifted: " + token)

runtime_test = read("api/rest/tests/test_public_backend_collection.cpp")
for token in (
    "backend-a",
    "backend-b",
    "backend-c",
    "be1_",
    "cursor_expired",
    "frontendSelector",
    "backendType",
    "restfulapi",
    "accessMode",
    "capabilities",
    "ETag",
    "statusCode == 405",
):
    require(token in runtime_test, "public Backend runtime test drifted: " + token)

security_test = read("core/security/tests/test_public_backend_collection_security.cpp")
for token in (
    '"channels.view", "backend-b"',
    '"timers.view", "backend-a"',
    '"role.read-only", "backend-c"',
    '"role.admin", "backend-d"',
    'std::vector<std::string>{"*"}',
    "statusCode == 401",
):
    require(token in security_test, "public Backend security test drifted: " + token)

current = read("docs/CURRENT.md")
kickoff = read("docs/development/phase-69-public-api-kickoff.md")
index = read("docs/development/index.md")
snapshot = read("docs/development/web-client-api-contract-snapshot.md")
link = "[Phase 69.F Public Backend Collection](development/phase-69f-public-backend-collection.md)"
require(link in current, "CURRENT must link public Backend slice")
require(
    "[Phase 69.F Public Backend Collection](phase-69f-public-backend-collection.md)" in kickoff,
    "kickoff must link public Backend slice",
)
require(
    "[Phase 69.F Public Backend Collection](phase-69f-public-backend-collection.md)" in index,
    "development index must link public Backend slice",
)
require(
    "[Phase 69.F Public Backend Collection](phase-69f-public-backend-collection.md)" in snapshot,
    "Web Client snapshot must link public Backend slice",
)

phase_make = read("mk/phase69-public-api-tests.mk")
for token in (
    "test-phase69f-public-backend-collection:",
    "test_public_backend_collection.cpp",
    "test_public_backend_collection_security.cpp",
    "check_phase69f_public_backend_collection.py",
):
    require(token in phase_make, "public Backend Make wiring drifted: " + token)

maintenance = read("mk/maintenance-tests.mk")
require(
    "python3 tools/check_phase69f_public_backend_collection.py" in maintenance,
    "architecture group must run public Backend guard",
)
require(
    "test-phase69f-public-backend-collection" in maintenance,
    "phase group must include public Backend collection",
)

groups = read("mk/test-groups.mk")
require(
    "test-phase69f-public-backend-collection" in groups,
    "fast CI must include public Backend collection",
)

print("Phase 69.F public Backend collection guard passed.")
print("Authorization: existing positive backend grant scopes; no backends.view.")
print("Representation: backendId/name/enabled/online only; provider type remains private.")
print("Legacy /api/backends remains retained pre-v1 and non-equivalent.")
