#!/usr/bin/env python3
"""Guard the sixth bounded Phase 69.F Home EPG single-route slice."""

import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def read(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")


def require(condition: bool, message: str) -> None:
    if not condition:
        raise SystemExit(message)


epg_cache = read("web/frontend/epg-cache.js")
start = epg_cache.find("function loadLiveNowNextEvents()")
end = epg_cache.find("function loadCachedNowNextEvents", start)
require(start >= 0 and end > start, "cannot bound Home EPG owner")
body = epg_cache[start:end]

require(
    "fetchJsonOrThrow('/api/epg/now-next?from=-1')" in body,
    "Home EPG must use the canonical now-next route",
)
require(
    body.count("fetchJsonOrThrow(") == 1,
    "Home EPG must issue exactly one semantic data request",
)
require(
    "/api/vdr/events" not in body,
    "Home EPG alternate-source fallback must stay retired",
)
require(
    ".catch(() => ({ events: [] }))" in body,
    "Home EPG must preserve fail-soft empty-event behavior",
)

router = read("api/rest/src/ApiRouter.cpp")
require(
    'if (path == "/api/epg/now-next")' in router
    and "return epgController_->getNowNext(" in router,
    "canonical EPG now-next server owner drifted",
)
require(
    'if (path == "/api/vdr/events")' in router
    and "return vdrController_.getEvents();" in router,
    "separate VDR events server owner disappeared",
)

matrix = json.loads(read("docs/development/phase-69f-client-contract-matrix.json"))
deferred = {item.get("id") for item in matrix.get("explicitDeferredFallbacks", [])}
require(
    deferred == set(),
    "dedicated Timer successor must retire the final deferred browser fallback",
)
candidate = matrix.get("derivedNextRuntimeCandidate", {})
require(
    candidate.get("domain") in {"phase69f-closeout-audit", "phase69-complete"}
    and candidate.get("proposedTemplate") is None,
    "Home EPG hardening must allow closeout/final state without preselecting a public resource",
)
resources = {
    (item.get("method"), item.get("template"))
    for item in matrix.get("publicV1Resources", [])
}
require(len(resources) >= 8, "Phase-69 stable public-v1 baseline disappeared")
for absent in ("/api/v1/recordings", "/api/v1/program-events"):
    require(
        not any(absent in (template or "") for _, template in resources),
        "identity audit does not authorize public resource: " + absent,
    )

public_runtime = read("api/rest/src/PublicApiRuntime.cpp")
require(
    "/api/v1/recordings" not in public_runtime,
    "Recording public route was invented without durable identity",
)
require(
    "/api/v1/program-events" not in public_runtime,
    "ProgramEvent public route was invented without productive canonical identity",
)

runtime_test = read("web/frontend/tests/test_phase69f_home_epg_fallback_removal.js")
for token in (
    "loadLiveNowNextEvents",
    "/api/epg/now-next?from=-1",
    "/api/vdr/events",
    "events: []",
    "fetchJsonOrThrow",
):
    require(token in runtime_test, "focused Home EPG test drifted: " + token)

doc = read("docs/development/phase-69f-home-epg-fallback-removal.md")
for token in (
    "main=64798ee1bd51694d70a01432d6260b4f84dfa26b",
    "PR #373",
    "/api/epg/now-next?from=-1",
    "/api/vdr/events",
    "Recording",
    "programEventId",
    "Home/LiveTV",
    "Timer live/snapshot",
    "No public-v1",
):
    require(token in doc, "Home EPG slice documentation drifted: " + token)

link = "[Phase 69.F Home EPG Single-Route Hardening]"
require(
    link + "(development/phase-69f-home-epg-fallback-removal.md)" in read("docs/CURRENT.md"),
    "CURRENT must link the Home EPG slice",
)
require(
    link + "(phase-69f-home-epg-fallback-removal.md)" in read("docs/development/phase-69-public-api-kickoff.md"),
    "Phase-69 kickoff must link the Home EPG slice",
)
require(
    link + "(phase-69f-home-epg-fallback-removal.md)" in read("docs/development/index.md"),
    "development index must link the Home EPG slice",
)
require(
    link + "(phase-69f-home-epg-fallback-removal.md)" in read("docs/development/web-client-api-contract-snapshot.md"),
    "Web Client API snapshot must link the Home EPG slice",
)

phase_make = read("mk/phase69-public-api-tests.mk")
for token in (
    "test-phase69f-home-epg-fallback-removal:",
    "test_phase69f_home_epg_fallback_removal.js",
    "test_home_now_next_artwork_hero.js",
    "test_phase66_live_tv_hero.js",
    "test_post_phase66_home_performance.js",
    "check_phase69f_home_epg_fallback_removal.py",
):
    require(token in phase_make, "Home EPG Make wiring drifted: " + token)

maintenance = read("mk/maintenance-tests.mk")
require(
    "python3 tools/check_phase69f_home_epg_fallback_removal.py" in maintenance,
    "architecture group must run Home EPG guard",
)
require(
    "test-phase69f-home-epg-fallback-removal" in maintenance,
    "phase group must include Home EPG slice",
)
require(
    "test-phase69f-home-epg-fallback-removal" in read("mk/test-groups.mk"),
    "fast CI must include Home EPG slice",
)

print("Phase 69.F Home EPG single-route guard passed.")
print("Home EPG uses one canonical now-next request and preserves fail-soft behavior.")
print("The dedicated Timer successor retires the final explicitly deferred browser fallback.")
