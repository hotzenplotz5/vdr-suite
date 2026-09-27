#!/usr/bin/env python3
"""Guard the second bounded Phase 69.E legacy-route classification slice."""
from pathlib import Path
import re
from check_phase69_public_api_inventory import (
    EXPECTED_DEPRECATED_LEGACY_ALIASES,
    EXPECTED_PUBLIC_V1_ROUTE_LITERALS,
    EXPECTED_ROUTE_LITERALS,
    LEGACY_ALIAS_GROUPS,
    LEGACY_TRANSITION_ROUTE_LITERALS,
)
ROOT = Path(__file__).resolve().parents[1]

def read(relative):
    path = ROOT / relative
    if not path.is_file():
        raise SystemExit(f"missing Phase 69.E classification file: {relative}")
    return path.read_text(encoding="utf-8")

def require(condition, message):
    if not condition:
        raise SystemExit(message)

def compact(text):
    return re.sub(r"\s+", " ", text)

def function_body(source, function_name, next_function_name):
    start = source.find(f"function {function_name}(options)")
    end = source.find(f"function {next_function_name}(options)", start)
    require(start >= 0, f"missing client fallback owner: {function_name}")
    require(end > start, f"cannot bound client fallback owner: {function_name}")
    return source[start:end]

require(len(EXPECTED_ROUTE_LITERALS) == 124, "expected route inventory count drifted")
require(len(EXPECTED_PUBLIC_V1_ROUTE_LITERALS) == 6, "public-v1 route count drifted")
require(len(LEGACY_ALIAS_GROUPS) == 27, "same-handler alias group count drifted")
require(len({r for g in LEGACY_ALIAS_GROUPS for r in g["routes"]}) == 54, "legacy alias member count drifted")
require(len(LEGACY_TRANSITION_ROUTE_LITERALS) == 64, "standalone transition route count drifted")
require(not EXPECTED_DEPRECATED_LEGACY_ALIASES, "this slice must not classify a legacy alias as deprecated")
require("/api/v1/search-timers" not in EXPECTED_PUBLIC_V1_ROUTE_LITERALS, "SearchTimer must not be promoted just to create a successor")
require("/api/vdr/channels" in LEGACY_TRANSITION_ROUTE_LITERALS, "legacy channels must remain transition surface")

cache = {}
for group in LEGACY_ALIAS_GROUPS:
    require(group["lifecycle"] == "transition", f'{group["id"]} must remain transition')
    require(group["publicV1Successor"] is None, f'{group["id"]} must not invent a public-v1 successor')
    source = cache.setdefault(group["source"], compact(read(group["source"])))
    first, second = group["routes"]
    same_branch = (
        f'path == "{first}" || path == "{second}"' in source or
        f'path == "{second}" || path == "{first}"' in source or
        f'path != "{first}" && path != "{second}"' in source or
        f'path != "{second}" && path != "{first}"' in source
    )
    require(same_branch, f'{group["id"]} is no longer a proven same-handler alias pair')

public_runtime = read("api/rest/src/PublicApiRuntime.cpp")
require(r'\"deprecatedAliases\":[]' in public_runtime, "public capabilities must keep deprecatedAliases empty")

client_api = read("web/frontend/api/client-api.js")
require(
    "function requestJsonWithFallback(path, fallbackPath, options)" in client_api and
    "return requestJson(path, options).catch(function (error)" in client_api and
    "return requestJson(fallbackPath, options);" in client_api,
    "single fallback helper is no longer the inventoried catch-all behavior",
)
require(
    "function requestJsonWithFallbacks(paths, options)" in client_api and
    "return requestJson(candidates[index], options).catch(function (error)" in client_api and
    "return tryNext(index + 1);" in client_api,
    "multi fallback helper is no longer the inventoried catch-all behavior",
)

fallback_contracts = (
    ("fetchClientTimers","fetchClientTimerConflicts","requestJsonWithFallback",("/api/vdr/timers/live","/api/vdr/timers")),
    ("fetchClientVdrOverview","fetchClientVdrStatus","requestJsonWithFallback",("/api/vdr/overview","/api/vdr")),
    ("fetchClientPersons","fetchClientRecordingPersons","requestJsonWithFallback",("/api/vdr/persons","/api/persons")),
    ("fetchClientRecordingPersons","fetchClientRecordingTrailer","requestJsonWithFallback",("/api/vdr/recordings/persons/search","/api/recordings/persons/search")),
    ("fetchClientSearchTimers","fetchClientSearchTimerDiscovery","requestJsonWithFallbacks",("/api/vdr/searchtimers/live","/api/vdr/searchtimers","/api/searchtimers")),
    ("fetchClientSearchTimerDiscovery","fetchClientSearchTimerPreview","requestJsonWithFallback",("/api/vdr/searchtimers/discovery","/api/searchtimers/discovery")),
    ("fetchClientSearchTimerPreview","fetchClientSearchTimerPreviewCacheRefresh","requestJsonWithFallback",("/api/vdr/searchtimers/preview","/api/searchtimers/preview")),
    ("fetchClientSearchTimerPreviewCacheRefresh","fetchClientSearchTimerPlan","requestJsonWithFallback",("/api/vdr/searchtimers/preview/cache/refresh","/api/searchtimers/preview/cache/refresh")),
    ("fetchClientSearchTimerPlan","fetchClientSearchTimerValidate","requestJsonWithFallback",("/api/vdr/searchtimers/plan","/api/searchtimers/plan")),
    ("fetchClientSearchTimerValidate","fetchClientSearchTimerExecute","requestJsonWithFallback",("/api/vdr/searchtimers/validate","/api/searchtimers/validate")),
    ("fetchClientSearchTimerExecute","fetchClientSearchTimerRealTest","requestJsonWithFallback",("/api/vdr/searchtimers/execute","/api/searchtimers/execute")),
    ("fetchClientSearchTimerRealTest","fetchClientSearchTimerCreateAction","requestJsonWithFallback",("/api/vdr/searchtimers/real-test","/api/searchtimers/real-test")),
    ("fetchClientSearchTimerCreateAction","fetchClientSearchTimerUpdateAction","requestJsonWithFallback",("/api/vdr/searchtimers","/api/searchtimers")),
    ("fetchClientSearchTimerUpdateAction","fetchClientSearchTimerDeleteAction","requestJsonWithFallback",("/api/vdr/searchtimers/update","/api/searchtimers/update")),
)
for function_name, next_name, helper, routes in fallback_contracts:
    body = function_body(client_api, function_name, next_name)
    require(helper in body, f"{function_name} fallback helper classification drifted")
    for route in routes:
        require(route in body, f"{function_name} fallback route disappeared: {route}")

delete_start = client_api.find("function fetchClientSearchTimerDeleteAction(options)")
delete_end = client_api.find("window.VdrSuiteClientApi", delete_start)
delete_body = client_api[delete_start:delete_end]
require(delete_start >= 0 and delete_end > delete_start, "cannot bound SearchTimer delete fallback")
require("requestJsonWithFallback" in delete_body and "/api/vdr/searchtimers/delete" in delete_body and "/api/searchtimers/delete" in delete_body, "SearchTimer delete fallback classification drifted")
require("/api/vdr/searchtimers/live" not in EXPECTED_ROUTE_LITERALS, "speculative SearchTimer live probe unexpectedly became a server route")

for function_name, next_name in (
    ("fetchClientSearchTimerExecute","fetchClientSearchTimerRealTest"),
    ("fetchClientSearchTimerCreateAction","fetchClientSearchTimerUpdateAction"),
    ("fetchClientSearchTimerUpdateAction","fetchClientSearchTimerDeleteAction"),
):
    body = function_body(client_api, function_name, next_name)
    require("jsonPostOptions(options)" in body and "requestJsonWithFallback" in body, f"{function_name} mutation fallback classification drifted")
require("jsonPostOptions(options)" in delete_body and "requestJsonWithFallback" in delete_body, "SearchTimer delete mutation fallback classification drifted")

epg_cache = read("web/frontend/epg-cache.js")
start = epg_cache.find("function loadLiveNowNextEvents()")
end = epg_cache.find("function loadCachedNowNextEvents", start)
body = epg_cache[start:end]
require(
    "fetch('/api/epg/now-next?from=-1')" in body and
    "return fetch('/api/vdr/events')" in body and
    "if (response.ok)" in body,
    "manual EPG route fallback classification drifted",
)

adr = read("docs/adr/ADR-0048-public-api-versioning-error-compatibility-contract.md")
for token in (
    "A legacy alias receives deprecation metadata once the canonical replacement is available.",
    "fallback logic retries after any error rather than only after a proven unsupported route",
    "The preferred v1 behavior is no mutation path fallback at all.",
    "every retained unversioned route is inventoried and classified",
):
    require(token in adr, "ADR-0048 classification authority drifted: " + token)

doc = read("docs/development/phase-69e-legacy-route-classification.md")
for token in (
    "118 retained unversioned route literals","27 same-handler alias groups",
    "54 alias-member route literals","64 standalone transition route literals",
    "0 deprecated aliases","15 fallback call sites",
    "four definite state-changing SearchTimer mutation fallbacks",
    "/api/vdr/searchtimers/live","/api/v1/search-timers",
    "/api/v1/channels","/api/vdr/channels","deprecatedAliases = []","Home","LiveTV",
):
    require(token in doc, "69.E classification doc misses: " + token)

print("Phase 69.E legacy-route classification guard passed.")
print("Routes: 124 total / 6 public-v1 / 118 retained unversioned.")
print("Legacy: 27 same-handler groups / 54 alias members / 64 standalone transition / 0 deprecated.")
print("Client fallbacks: 15 wrapper call sites plus one manual EPG GET fallback; catch-all behavior remains classified, not approved.")
