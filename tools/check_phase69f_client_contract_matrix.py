#!/usr/bin/env python3
"""Guard the fourth bounded Phase 69.F client-contract matrix slice."""

import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
MATRIX_PATH = ROOT / "docs/development/phase-69f-client-contract-matrix.json"

def read(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")

def require(condition: bool, message: str) -> None:
    if not condition:
        raise SystemExit(message)

matrix = json.loads(MATRIX_PATH.read_text(encoding="utf-8"))
require(matrix.get("schemaVersion") == 1, "client-contract matrix schemaVersion drifted")
require(matrix.get("phase") == "69.F", "client-contract matrix phase drifted")
require(
    matrix.get("baselineMain") == "83bd7c3522d4cdaef6d24b529fc46c44994275b9",
    "client-contract matrix baseline drifted",
)

required_families = {"browser", "tv", "mobile", "desktop", "kodi", "automation"}
require(set(matrix.get("clientFamilies", [])) == required_families, "client family coverage drifted")

allowed_classifications = {
    "pre-v1-transition",
    "first-party-private",
    "bounded-domain-plane",
}

base_client = read("web/frontend/api/client-api.js")
genre_client = read("web/frontend/api/genre-client-api.js")
remote_client = read("web/frontend/api/live-remote-client-api.js")

base_operations = set(re.findall(r"function\s+(fetchClient[A-Za-z0-9_]+)\s*\(", base_client))
genre_operations = set(re.findall(r"function\s+(fetchClient[A-Za-z0-9_]+)\s*\(", genre_client))
remote_operations = set(re.findall(r"function\s+(fetchClient[A-Za-z0-9_]+)\s*\(", remote_client))
require("function createClientLiveUpdateSource()" in remote_client, "Live Remote update-source operation disappeared")
remote_operations.add("createClientLiveUpdateSource")

require(len(base_operations) == 56, f"expected 56 base client operations, found {len(base_operations)}")
require(len(genre_operations) == 3, f"expected 3 Genre client operations, found {len(genre_operations)}")
require(len(remote_operations) == 3, f"expected 3 Live Remote operations, found {len(remote_operations)}")

discovered_operations = base_operations | genre_operations | remote_operations
require(len(discovered_operations) == 62, f"expected 62 classified browser Client API operations, found {len(discovered_operations)}")

classified = []
groups = matrix.get("browserClientGroups", [])
require(groups, "client-contract matrix has no browserClientGroups")
for group in groups:
    classification = group.get("classification")
    require(classification in allowed_classifications, f"unknown client classification: {classification}")
    exports = group.get("exports", [])
    require(exports, f"empty client group: {group.get('domain')}")
    classified.extend(exports)

require(len(classified) == len(set(classified)), "client-contract matrix classifies at least one operation more than once")
require(
    set(classified) == discovered_operations,
    "client-contract matrix operation coverage drifted: "
    f"missing={sorted(discovered_operations - set(classified))} "
    f"unknown={sorted(set(classified) - discovered_operations)}",
)

for source_name, source in (
    ("base", base_client),
    ("genre", genre_client),
    ("live-remote", remote_client),
):
    require("/api/v1" not in source, f"{source_name} browser Client API started consuming /api/v1 without matrix migration")

resources = matrix.get("publicV1Resources", [])
resource_pairs = {(item.get("method"), item.get("template")) for item in resources}
expected_resource_pairs = {
    ("GET", "/api/v1"),
    ("GET", "/api/v1/capabilities"),
    ("GET", "/api/v1/operations/{operationId}"),
    ("GET", "/api/v1/timer-assignments?backend={backendId}"),
    ("GET", "/api/v1/timer-assignments/{timerAssignmentId}?backend={backendId}"),
    ("POST", "/api/v1/timer-assignments/{timerAssignmentId}?backend={backendId}"),
    ("GET", "/api/v1/channels?backendId={backendId}"),
}
require(resource_pairs == expected_resource_pairs, "declared stable public-v1 resource/method set drifted")

public_runtime = read("api/rest/src/PublicApiRuntime.cpp")
for token in (
    'constexpr const char* PublicApiV1Root = "/api/v1";',
    'constexpr const char* PublicOperationPrefix = "/api/v1/operations/";',
    '"/api/v1/timer-assignments"',
    '"/api/v1/channels"',
    'if (path == "/api/v1/capabilities")',
):
    require(token in public_runtime, "accepted public-v1 runtime token drifted: " + token)

for absent in (
    "/api/v1/backends",
    "/api/v1/recordings",
    "/api/v1/program-events",
    "/api/v1/search-timers",
):
    require(absent not in public_runtime, f"new public resource requires an intentional matrix successor slice: {absent}")

api_router = read("api/rest/src/ApiRouter.cpp")
require('if (path == "/api/backends")' in api_router, "pre-v1 backend discovery route disappeared")
require('if (path == "/api/backends/default")' in api_router, "pre-v1 default-backend route disappeared")

candidate = matrix.get("derivedNextRuntimeCandidate", {})
require(candidate.get("domain") == "backends", "next runtime candidate must remain backend discovery")
require(candidate.get("proposedTemplate") == "/api/v1/backends", "next runtime candidate template drifted")

timer_group = next((g for g in groups if g.get("domain") == "timers"), None)
channel_group = next((g for g in groups if g.get("domain") == "channels"), None)
cap_group = next((g for g in groups if g.get("domain") == "vdr-capabilities-runtime"), None)
require(timer_group and timer_group.get("publicV1Relation") == "partial-non-equivalent", "Timer non-equivalence classification drifted")
require(channel_group and channel_group.get("publicV1Relation") == "partial-non-equivalent", "Channel non-equivalence classification drifted")
require(cap_group and cap_group.get("publicV1Relation") == "non-equivalent", "capability non-equivalence classification drifted")

require(base_client.count("return requestJsonWithFallback(") == 1, "matrix expects exactly one remaining base wrapper fallback")
timer_start = base_client.find("function fetchClientTimers(options)")
timer_end = base_client.find("function fetchClientTimerConflicts(options)", timer_start)
timer_body = base_client[timer_start:timer_end]
require(
    "/api/vdr/timers/live" in timer_body
    and "/api/vdr/timers" in timer_body
    and "requestJsonWithFallback(" in timer_body,
    "Timer live/snapshot deferred fallback drifted",
)

epg_cache = read("web/frontend/epg-cache.js")
epg_start = epg_cache.find("function loadLiveNowNextEvents()")
epg_end = epg_cache.find("function loadCachedNowNextEvents", epg_start)
require(epg_start >= 0 and epg_end > epg_start, "cannot bound Home EPG fallback")
epg_body = epg_cache[epg_start:epg_end]
require(
    "fetch('/api/epg/now-next?from=-1')" in epg_body
    and "return fetch('/api/vdr/events')" in epg_body,
    "Home-sensitive EPG deferred fallback drifted",
)

for route in (
    "/api/security/browser-sessions",
    "/api/security/browser-sessions/current",
    "/api/security/browser-sessions/logout",
):
    require(route in remote_client, "browser-session private route drifted: " + route)

doc = read("docs/development/phase-69f-client-contract-matrix.md")
for token in (
    "56 base ",
    "fetchClient*",
    "browser, TV, mobile, desktop and Kodi",
    "GET /api/v1/backends",
    "does not exist",
    "first justified runtime candidate",
    "Home/LiveTV-sensitive",
):
    require(token in doc, "client-contract matrix documentation drifted: " + token)

snapshot = read("docs/development/web-client-api-contract-snapshot.md")
require(
    "[Phase 69.F Client Contract Matrix](phase-69f-client-contract-matrix.md)" in snapshot,
    "Web Client API snapshot must link the 69.F client-contract matrix",
)

current = read("docs/CURRENT.md")
require(
    "[Phase 69.F Client Contract Matrix](development/phase-69f-client-contract-matrix.md)" in current,
    "CURRENT must link the fourth 69.F slice",
)

kickoff = read("docs/development/phase-69-public-api-kickoff.md")
require(
    "[Phase 69.F Client Contract Matrix](phase-69f-client-contract-matrix.md)" in kickoff,
    "Phase-69 kickoff must link the fourth 69.F slice",
)

development_index = read("docs/development/index.md")
require(
    "[Phase 69.F Client Contract Matrix](phase-69f-client-contract-matrix.md)" in development_index,
    "development index must link the client-contract matrix",
)

roadmap = read("docs/planning/roadmap.md")
for token in (
    "client wrappers consume stable v1 contracts",
    "browser, TV, mobile, desktop and Kodi integrations receive a documented stable boundary",
):
    require(token in roadmap, "roadmap 69.F acceptance direction drifted: " + token)

phase_make = read("mk/phase69-public-api-tests.mk")
for token in (
    "test-phase69f-client-contract-matrix:",
    "python3 tools/check_phase69f_client_contract_matrix.py",
):
    require(token in phase_make, "matrix Make wiring drifted: " + token)

maintenance = read("mk/maintenance-tests.mk")
require("python3 tools/check_phase69f_client_contract_matrix.py" in maintenance, "architecture group must run client-contract matrix guard")
require("test-phase69f-client-contract-matrix" in maintenance, "phase group must include client-contract matrix")

test_groups = read("mk/test-groups.mk")
require("test-phase69f-client-contract-matrix" in test_groups, "fast CI must include client-contract matrix")

print("Phase 69.F client-contract matrix guard passed.")
print("Classified browser Client API operations: 62 exactly once.")
print("Stable public-v1 method/resource contracts: 7.")
print("Derived next runtime candidate: read-only /api/v1/backends audit.")
