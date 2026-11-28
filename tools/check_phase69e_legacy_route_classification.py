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

require(len(EXPECTED_ROUTE_LITERALS) >= 125, "Phase-69 route inventory baseline disappeared")
require(len(EXPECTED_PUBLIC_V1_ROUTE_LITERALS) >= 7, "Phase-69 public-v1 route baseline disappeared")
require(len(LEGACY_ALIAS_GROUPS) == 27, "same-handler alias group count drifted")
require(len({r for g in LEGACY_ALIAS_GROUPS for r in g["routes"]}) == 54, "legacy alias member count drifted")
require(len(LEGACY_TRANSITION_ROUTE_LITERALS) == 64, "standalone transition route count drifted")
require(not EXPECTED_DEPRECATED_LEGACY_ALIASES, "this slice must not classify a legacy alias as deprecated")
# Phase 69.E originally forbade inventing a SearchTimer successor. A
# separately audited post-69 read-only Device contract now exists, but does
# NOT classify the legacy Web SearchTimer aliases as deprecated/equivalent.
require("/api/v1/search-timers" in EXPECTED_PUBLIC_V1_ROUTE_LITERALS,
        "new SearchTimer read route missing from canonical audited inventory")
runtime_for_search_timers = read("api/rest/src/PublicApiRuntime.cpp")
search_timers_security = read("core/security/include/SecurityHttpGate.h")
require('PublicSearchTimerCollectionPath =\n    "/api/v1/search-timers"' in runtime_for_search_timers and
        "if (path == PublicSearchTimerCollectionPath)" in runtime_for_search_timers and
        "lookupSearchTimerCollection(request)" in runtime_for_search_timers,
        "new read-only SearchTimer resource must have an actual scoped GET implementation")
require('isPublicSearchTimerRead' in search_timers_security and
        'read.permission = "searchtimers.view";' in search_timers_security and
        'gate.deviceAuthenticated' in search_timers_security,
        "SearchTimer GET must preserve independent verified Device grant admission")
require(all(group["publicV1Successor"] is None for group in LEGACY_ALIAS_GROUPS),
        "new SearchTimer GET must not silently promote pre-v1 browser aliases")
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

public_runtime = read("api/rest/src/PublicDiscoveryRoutes.cpp")
require(r'\"deprecatedAliases\":[]' in public_runtime, "public capabilities must keep deprecatedAliases empty")

client_api = read("web/frontend/api/client-api.js")
require(
    "function requestJsonWithFallback(path, fallbackPath, options)" not in client_api,
    "retired single-path fallback helper unexpectedly returned",
)
require(
    "function requestJsonWithFallbacks(paths, options)" not in client_api,
    "retired multi-path fallback helper unexpectedly returned",
)

timer_body = function_body(client_api, "fetchClientTimers", "fetchClientTimerConflicts")
require(
    "publicNativeTimerClient().getLiveNativeTimers(" in timer_body
    and "/api/vdr/timers/live" not in timer_body,
    "Timer browser read must keep the live route",
)
require(
    "requestJsonWithFallback" not in timer_body
    and "'/api/vdr/timers'" not in timer_body,
    "Timer browser read must not restore live/snapshot fallback probing",
)

read_alias_contracts = (
    ("fetchClientVdrOverview","fetchClientVdrStatus","/api/vdr/overview","/api/vdr"),
    ("fetchClientPersons","fetchClientRecordingPersons","/api/vdr/persons","/api/persons"),
    ("fetchClientRecordingPersons","fetchClientRecordingTrailer","/api/v1/search/recordings","/api/vdr/recordings/persons/search"),
)
for function_name, next_name, primary_route, alternate_route in read_alias_contracts:
    body = function_body(client_api, function_name, next_name)
    require("requestJson(" in body and primary_route in body, f"{function_name} primary read route drifted")
    require("requestJsonWithFallback" not in body, f"{function_name} must not restore read alias fallback probing")
    require(
        f"'{alternate_route}'" not in body,
        f"{function_name} must not probe the alternate read alias",
    )

for route in ("/api/vdr/searchtimers", "/api/searchtimers", "/api/vdr/timers/actions/"):
    require(route not in client_api, "retired browser transport returned: " + route)

refresh_body = function_body(client_api, "fetchClientSearchTimerPreviewCacheRefresh", "fetchClientSearchTimerCreateStaged")
require(
    "requestJson(" in refresh_body
    and "/api/v1/search-timers/preview-cache/refresh" in refresh_body
    and "queryMutationOptions(" in refresh_body
    and "requestJsonWithFallback" not in refresh_body,
    "Public-v1 preview refresh must remain one-shot and CSRF-bound",
)

# Public-v1 SearchTimer CREATE is an intentional newer contract. Legacy
# same-handler aliases remain only for the separate pre-v1 read/update/delete.
staged_create = function_body(client_api,
    "fetchClientSearchTimerCreateStaged", "fetchClientSearchTimerCreateAction")
require(
    "requestJson(" in staged_create
    and "'/api/v1/search-timers'" in staged_create
    and "queryMutationOptions(jsonPostOptions(" in staged_create
    and "'Idempotency-Key'" in staged_create,
    "new SearchTimer CREATE must preserve Public-v1 admission and idempotency",
)
create_action = function_body(client_api,
    "fetchClientSearchTimerCreateAction", "fetchClientSearchTimerOperation")
require(
    "return fetchClientSearchTimerCreateStaged(options)" in create_action
    and "requestJson(" not in create_action
    and "requestJsonWithFallback" not in create_action,
    "Browser CREATE must delegate to one Public-v1 route",
)

mutation_start = client_api.index("function mutatePublicSearchTimer(options)")
mutation_end = client_api.index("window.VdrSuiteClientApi", mutation_start)
mutation_body = client_api[mutation_start:mutation_end]
require(
    "publicSearchTimerPath(normalized, '/mutations')" in mutation_body
    and "queryMutationOptions(jsonPostOptions(" in mutation_body
    and "'Idempotency-Key'" in mutation_body
    and "'If-Match'" in mutation_body
    and "requestJsonWithFallback" not in mutation_body,
    "Public-v1 SearchTimer mutations must remain version-fenced and one-shot",
)

require("/api/vdr/searchtimers/live" not in client_api, "speculative SearchTimer live client probe returned")
require("/api/vdr/searchtimers/live" not in EXPECTED_ROUTE_LITERALS, "speculative SearchTimer live probe unexpectedly became a server route")

epg_cache = read("web/frontend/epg-cache.js")
start = epg_cache.find("function loadLiveNowNextEvents()")
end = epg_cache.find("function loadCachedNowNextEvents", start)
body = epg_cache[start:end]
require(
    "fetchJsonOrThrow('/api/epg/now-next?from=-1')" in body,
    "Home EPG canonical route drifted after the 69.F successor",
)
require(
    "/api/vdr/events" not in body,
    "retired Home EPG arbitrary-error fallback returned",
)
require(
    ".catch(() => ({ events: [] }))" in body,
    "Home EPG fail-soft behavior drifted after fallback retirement",
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
print(f"Routes: {len(EXPECTED_ROUTE_LITERALS)} total / {len(EXPECTED_PUBLIC_V1_ROUTE_LITERALS)} public-v1 / 118 retained unversioned.")
print("Legacy: 27 same-handler groups / 54 alias members / 64 standalone transition / 0 deprecated.")
print("69.E baseline: 15 wrapper call sites plus one manual EPG GET fallback, including four definite state-changing SearchTimer mutation fallbacks.")
print("Current downstream state: all browser route fallback probing is retired by dedicated 69.F successors; retained server routes remain classified pre-v1.")
