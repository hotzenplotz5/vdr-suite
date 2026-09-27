#!/usr/bin/env python3
"""Guard the second bounded Phase 69.F SearchTimer client-hardening slice."""

from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


def read(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")


def require(condition: bool, message: str) -> None:
    if not condition:
        raise SystemExit(message)


def function_body(source: str, function_name: str, next_name: str | None) -> str:
    start = source.find(f"function {function_name}(options)")
    require(start >= 0, f"missing client function: {function_name}")
    if next_name is None:
        end = source.find("window.VdrSuiteClientApi", start)
    else:
        end = source.find(f"function {next_name}(options)", start)
    require(end > start, f"cannot bound client function: {function_name}")
    return source[start:end]


client_api = read("web/frontend/api/client-api.js")

require(
    "function requestJsonWithFallbacks(paths, options)" not in client_api,
    "multi-path catch-all fallback helper must stay removed",
)
require(
    "/api/vdr/searchtimers/live" not in client_api,
    "speculative SearchTimer live route must stay out of the bundled client",
)
require(
    client_api.count("return requestJsonWithFallback(") == 4,
    "expected exactly four remaining wrapper fallback call sites",
)

remaining_fallbacks = (
    ("fetchClientTimers", "fetchClientTimerConflicts"),
    ("fetchClientVdrOverview", "fetchClientVdrStatus"),
    ("fetchClientPersons", "fetchClientRecordingPersons"),
    ("fetchClientRecordingPersons", "fetchClientRecordingTrailer"),
)
for function_name, next_name in remaining_fallbacks:
    body = function_body(client_api, function_name, next_name)
    require(
        "requestJsonWithFallback(" in body,
        f"{function_name} is no longer one of the four explicit remaining fallbacks",
    )

searchtimer_contracts = (
    ("fetchClientSearchTimers", "fetchClientSearchTimerDiscovery", "/api/vdr/searchtimers", "/api/searchtimers"),
    ("fetchClientSearchTimerDiscovery", "fetchClientSearchTimerPreview", "/api/vdr/searchtimers/discovery", "/api/searchtimers/discovery"),
    ("fetchClientSearchTimerPreview", "fetchClientSearchTimerPreviewCacheRefresh", "/api/vdr/searchtimers/preview", "/api/searchtimers/preview"),
    ("fetchClientSearchTimerPreviewCacheRefresh", "fetchClientSearchTimerPlan", "/api/vdr/searchtimers/preview/cache/refresh", "/api/searchtimers/preview/cache/refresh"),
    ("fetchClientSearchTimerPlan", "fetchClientSearchTimerValidate", "/api/vdr/searchtimers/plan", "/api/searchtimers/plan"),
    ("fetchClientSearchTimerValidate", "fetchClientSearchTimerExecute", "/api/vdr/searchtimers/validate", "/api/searchtimers/validate"),
    ("fetchClientSearchTimerExecute", "fetchClientSearchTimerRealTest", "/api/vdr/searchtimers/execute", "/api/searchtimers/execute"),
    ("fetchClientSearchTimerRealTest", "fetchClientSearchTimerCreateAction", "/api/vdr/searchtimers/real-test", "/api/searchtimers/real-test"),
    ("fetchClientSearchTimerCreateAction", "fetchClientSearchTimerUpdateAction", "/api/vdr/searchtimers", "/api/searchtimers"),
    ("fetchClientSearchTimerUpdateAction", "fetchClientSearchTimerDeleteAction", "/api/vdr/searchtimers/update", "/api/searchtimers/update"),
)
for function_name, next_name, primary_route, alternate_route in searchtimer_contracts:
    body = function_body(client_api, function_name, next_name)
    require(
        "requestJson(" in body and primary_route in body,
        f"{function_name} primary SearchTimer route drifted",
    )
    require(
        "requestJsonWithFallback" not in body,
        f"{function_name} restored SearchTimer fallback probing",
    )
    require(
        alternate_route not in body,
        f"{function_name} restored alternate SearchTimer alias probing",
    )

delete_body = function_body(client_api, "fetchClientSearchTimerDeleteAction", None)
require(
    "requestJson(" in delete_body and "/api/vdr/searchtimers/delete" in delete_body,
    "fetchClientSearchTimerDeleteAction primary route drifted",
)
require(
    "requestJsonWithFallback" not in delete_body and "/api/searchtimers/delete" not in delete_body,
    "fetchClientSearchTimerDeleteAction restored alias fallback",
)

router = read("api/rest/src/ApiRouter.cpp")
for first, second in (
    ("/api/searchtimers", "/api/vdr/searchtimers"),
    ("/api/searchtimers/discovery", "/api/vdr/searchtimers/discovery"),
    ("/api/searchtimers/preview", "/api/vdr/searchtimers/preview"),
    ("/api/searchtimers/preview/cache/refresh", "/api/vdr/searchtimers/preview/cache/refresh"),
    ("/api/searchtimers/plan", "/api/vdr/searchtimers/plan"),
    ("/api/searchtimers/validate", "/api/vdr/searchtimers/validate"),
    ("/api/searchtimers/execute", "/api/vdr/searchtimers/execute"),
    ("/api/searchtimers/real-test", "/api/vdr/searchtimers/real-test"),
    ("/api/searchtimers/update", "/api/vdr/searchtimers/update"),
    ("/api/searchtimers/delete", "/api/vdr/searchtimers/delete"),
):
    same_branch = (
        f'path == "{first}" ||\n        path == "{second}"' in router
        or f'path == "{second}" ||\n        path == "{first}"' in router
    )
    require(same_branch, f"server same-handler alias proof drifted: {first} <-> {second}")

inventory = read("tools/check_phase69_public_api_inventory.py")
require(
    '"/api/vdr/searchtimers/live" not in EXPECTED_ROUTE_LITERALS'
    not in inventory,
    "inventory guard source unexpectedly embeds client-only assertion",
)
require(
    '"/api/vdr/searchtimers/live",' not in inventory,
    "speculative SearchTimer live route unexpectedly became a server route",
)

runtime_test = read("web/frontend/tests/test_phase69f_searchtimer_fallback_removal.js")
for token in (
    "assert.strictEqual(emitted.length, 1)",
    "fetchClientSearchTimers",
    "fetchClientSearchTimerDiscovery",
    "fetchClientSearchTimerPreview",
    "fetchClientSearchTimerPreviewCacheRefresh",
    "fetchClientSearchTimerPlan",
    "fetchClientSearchTimerValidate",
    "fetchClientSearchTimerRealTest",
    "backend_unavailable",
):
    require(token in runtime_test, "focused SearchTimer regression drifted: " + token)

cache_test = read("web/frontend/tests/test_query_cache_refresh_security_runtime.js")
for token in (
    "assert.strictEqual(failedRequests.length, 1)",
    "api.isClientError(previewFailure)",
    "/api/vdr/searchtimers/preview/cache/refresh",
):
    require(token in cache_test, "query-cache one-shot regression drifted: " + token)
require(
    "fallbackRequests.length, 2" not in cache_test,
    "query-cache regression still expects alias fallback",
)

doc = read("docs/development/phase-69f-searchtimer-fallback-removal.md")
for token in (
    "main=587352abfb4ffb04628831a2fbc3b528be3c722b",
    "PR #369",
    "same-handler alias",
    "/api/vdr/searchtimers/live",
    "four wrapper fallback call sites remain",
    "/api/v1/search-timers",
    "Home rail or LiveTV",
):
    require(token in doc, "SearchTimer fallback-removal documentation drifted: " + token)

current = read("docs/CURRENT.md")
require(
    "[Phase 69.F SearchTimer Client Fallback Removal](development/phase-69f-searchtimer-fallback-removal.md)" in current,
    "CURRENT must link the second 69.F slice",
)
kickoff = read("docs/development/phase-69-public-api-kickoff.md")
require(
    "[Phase 69.F SearchTimer Client Fallback Removal](phase-69f-searchtimer-fallback-removal.md)" in kickoff,
    "Phase-69 kickoff must link the second 69.F slice",
)

public_runtime = read("api/rest/src/PublicApiRuntime.cpp")
require(
    r'\"deprecatedAliases\":[]' in public_runtime,
    "SearchTimer client cleanup must not invent server deprecation",
)

phase_make = read("mk/phase69-public-api-tests.mk")
for token in (
    "test-phase69f-searchtimer-fallback-removal:",
    "node web/frontend/tests/test_phase69f_searchtimer_fallback_removal.js",
    "node web/frontend/tests/test_query_cache_refresh_security_runtime.js",
    "python3 tools/check_phase69f_searchtimer_fallback_removal.py",
    "test-frontend-contracts: test-phase69f-searchtimer-fallback-removal",
):
    require(token in phase_make, "SearchTimer fallback-removal Make wiring drifted: " + token)

maintenance = read("mk/maintenance-tests.mk")
require(
    "python3 tools/check_phase69f_searchtimer_fallback_removal.py" in maintenance,
    "architecture group must run the SearchTimer fallback-removal guard",
)
require(
    "test-phase69f-searchtimer-fallback-removal" in maintenance,
    "phase group must include SearchTimer fallback removal",
)

test_groups = read("mk/test-groups.mk")
require(
    "test-phase69f-searchtimer-fallback-removal" in test_groups,
    "fast CI must include SearchTimer fallback removal",
)

print("Phase 69.F SearchTimer fallback-removal guard passed.")
print("SearchTimer client: one route per operation; no alias retry and no speculative live probe.")
print("Remaining wrapper fallback call sites: 4, plus the separate manual EPG fallback.")
