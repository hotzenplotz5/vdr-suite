#!/usr/bin/env python3
"""Guard the third bounded Phase 69.F same-handler read-alias cleanup."""

from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


def read(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")


def require(condition: bool, message: str) -> None:
    if not condition:
        raise SystemExit(message)


def function_body(source: str, function_name: str, next_name: str) -> str:
    start = source.find(f"function {function_name}(options)")
    require(start >= 0, f"missing client function: {function_name}")
    end = source.find(f"function {next_name}(options)", start)
    require(end > start, f"cannot bound client function: {function_name}")
    return source[start:end]


client_api = read("web/frontend/api/client-api.js")
require(
    client_api.count("return requestJsonWithFallback(") == 0,
    "all wrapper fallback calls must remain retired by 69.F successors",
)

timer_body = function_body(client_api, "fetchClientTimers", "fetchClientTimerConflicts")
require(
    "requestJson('/api/vdr/timers/live', options)" in timer_body,
    "Timer successor must preserve the live route",
)
require(
    "requestJsonWithFallback" not in timer_body
    and "'/api/vdr/timers'" not in timer_body,
    "Timer successor must keep snapshot fallback retired",
)

read_aliases = (
    ("fetchClientVdrOverview", "fetchClientVdrStatus", "/api/vdr/overview", "/api/vdr"),
    ("fetchClientPersons", "fetchClientRecordingPersons", "/api/vdr/persons", "/api/persons"),
    (
        "fetchClientRecordingPersons",
        "fetchClientRecordingTrailer",
        "/api/vdr/recordings/persons/search",
        "/api/recordings/persons/search",
    ),
)

for function_name, next_name, primary_route, alternate_route in read_aliases:
    body = function_body(client_api, function_name, next_name)
    require(
        "requestJson(" in body and primary_route in body,
        f"{function_name} primary route drifted",
    )
    require(
        "requestJsonWithFallback" not in body,
        f"{function_name} restored fallback probing",
    )
    require(
        f"'{alternate_route}'" not in body,
        f"{function_name} restored alternate alias probing",
    )

recording_persons_body = function_body(
    client_api,
    "fetchClientRecordingPersons",
    "fetchClientRecordingTrailer",
)
require(
    "backendQueryOptions(options)" in recording_persons_body,
    "RecordingPersons backend query normalization drifted",
)

router = read("api/rest/src/ApiRouter.cpp")
for first, second in (
    ("/api/vdr", "/api/vdr/overview"),
    ("/api/persons", "/api/vdr/persons"),
    ("/api/recordings/persons/search", "/api/vdr/recordings/persons/search"),
):
    pair = (
        f'path == "{first}" ||\n        path == "{second}"' in router
        or f'path == "{second}" ||\n        path == "{first}"' in router
    )
    require(pair, f"same-handler read alias proof drifted: {first} <-> {second}")

require(
    'if (path == "/api/vdr/timers/live")' in router
    and "return vdrController_.getLiveTimers();" in router,
    "Timer live route semantics drifted",
)
require(
    'if (path == "/api/vdr/timers")' in router
    and "return vdrController_.getTimers();" in router,
    "Timer snapshot route semantics drifted",
)

epg_cache = read("web/frontend/epg-cache.js")
start = epg_cache.find("function loadLiveNowNextEvents()")
end = epg_cache.find("function loadCachedNowNextEvents", start)
require(start >= 0 and end > start, "cannot bound manual EPG fallback")
epg_body = epg_cache[start:end]
require(
    "fetchJsonOrThrow('/api/epg/now-next?from=-1')" in epg_body,
    "Home EPG successor must preserve the canonical route",
)
require(
    "/api/vdr/events" not in epg_body,
    "dedicated Home EPG successor must retire alternate-source fallback",
)
require(
    ".catch(() => ({ events: [] }))" in epg_body,
    "Home EPG successor must preserve fail-soft empty-event behavior",
)

runtime_test = read("web/frontend/tests/test_phase69f_read_alias_fallback_removal.js")
for token in (
    "assert.strictEqual(emitted.length, 1)",
    "fetchClientVdrOverview",
    "fetchClientPersons",
    "fetchClientRecordingPersons",
    "backend_unavailable",
    "living-room",
):
    require(token in runtime_test, "focused read-alias regression drifted: " + token)

doc = read("docs/development/phase-69f-read-alias-fallback-removal.md")
for token in (
    "main=b7ec2a6cf44e5b6b3e4652370a73e27aa071e772",
    "PR #370",
    "same-handler",
    "exactly one wrapper fallback remains",
    "Home or",
    "LiveTV",
):
    require(token in doc, "read-alias slice documentation drifted: " + token)

public_runtime = read("api/rest/src/PublicApiRuntime.cpp")
require(
    r'\"deprecatedAliases\":[]' in public_runtime,
    "read-alias client cleanup must not invent server deprecation",
)

current = read("docs/CURRENT.md")
require(
    "[Phase 69.F Same-Handler Read Alias Fallback Removal](development/phase-69f-read-alias-fallback-removal.md)" in current,
    "CURRENT must link the third 69.F slice",
)

kickoff = read("docs/development/phase-69-public-api-kickoff.md")
require(
    "[Phase 69.F Same-Handler Read Alias Fallback Removal](phase-69f-read-alias-fallback-removal.md)" in kickoff,
    "Phase-69 kickoff must link the third 69.F slice",
)

phase_make = read("mk/phase69-public-api-tests.mk")
for token in (
    "test-phase69f-read-alias-fallback-removal:",
    "node web/frontend/tests/test_phase69f_read_alias_fallback_removal.js",
    "python3 tools/check_phase69f_read_alias_fallback_removal.py",
    "test-frontend-contracts: test-phase69f-read-alias-fallback-removal",
):
    require(token in phase_make, "read-alias Make wiring drifted: " + token)

maintenance = read("mk/maintenance-tests.mk")
require(
    "python3 tools/check_phase69f_read_alias_fallback_removal.py" in maintenance,
    "architecture group must run the read-alias guard",
)
require(
    "test-phase69f-read-alias-fallback-removal" in maintenance,
    "phase group must include the read-alias slice",
)

test_groups = read("mk/test-groups.mk")
require(
    "test-phase69f-read-alias-fallback-removal" in test_groups,
    "fast CI must include the read-alias slice",
)

print("Phase 69.F same-handler read-alias fallback-removal guard passed.")
print("Timer live/snapshot fallback is retired by its dedicated 69.F successor.")
print("Manual Home-sensitive EPG fallback is retired by its dedicated 69.F successor.")
