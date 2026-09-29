#!/usr/bin/env python3
"""Guard the seventh bounded Phase 69.F Timer live single-route slice."""
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

def read(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")

def require(condition: bool, message: str) -> None:
    if not condition:
        raise SystemExit(message)

def function_body(source: str, function_name: str, next_name: str) -> str:
    start = source.find(f"function {function_name}(options)")
    end = source.find(f"function {next_name}(options)", start)
    require(start >= 0 and end > start, f"cannot bound client function: {function_name}")
    return source[start:end]

client_api = read("web/frontend/api/client-api.js")
timer_body = function_body(client_api, "fetchClientTimers", "fetchClientTimerConflicts")
require("function requestJsonWithFallback(path, fallbackPath, options)" not in client_api, "retired single-path catch-all fallback helper returned")
require("function requestJsonWithFallbacks(paths, options)" not in client_api, "retired multi-path catch-all fallback helper returned")
require("requestJson('/api/vdr/timers/live', options)" in timer_body, "fetchClientTimers must use the live Timer route")
require("requestJsonWithFallback" not in timer_body and "'/api/vdr/timers'" not in timer_body, "fetchClientTimers must not retry the snapshot Timer route")

router = read("api/rest/src/ApiRouter.cpp")
require('if (path == "/api/vdr/timers/live")' in router and "return vdrController_.getLiveTimers();" in router, "live Timer route owner drifted")
require('if (path == "/api/vdr/timers")' in router and "return vdrController_.getTimers();" in router, "snapshot Timer route owner drifted")

controller = read("api/rest/src/VdrController.cpp")
live_start = controller.find("ApiResponse VdrController::getLiveTimers()")
live_end = controller.find("ApiResponse VdrController::getLiveTimerConflicts()", live_start)
require(live_start >= 0 and live_end > live_start, "cannot bound getLiveTimers")
live_body = controller[live_start:live_end]
require("if (liveService_ == nullptr)" in live_body and "return getTimers();" in live_body and "liveService_->getTimers()" in live_body, "server-owned live-service-unavailable Timer fallback drifted")

app = read("web/frontend/app.js")
load_start = app.find("function loadTimers()")
load_end = app.find("function configureSearchTimerBrowserContextBoundary()", load_start)
require(load_start >= 0 and load_end > load_start, "cannot bound loadTimers")
load_body = app[load_start:load_end]
require("Lade aktuelle Timerliste direkt vom VDR" in load_body and "fetchClientTimers()" in load_body, "Timer module must remain a current/live VDR read consumer")

sync_start = app.find("function syncEpgTimerDetailStates()")
sync_end = app.find("function startEpgTimerDetailSync()", sync_start)
require(sync_start >= 0 and sync_end > sync_start, "cannot bound EPG Timer live sync")
sync_body = app[sync_start:sync_end]
require("fetchClientTimers({ cache: 'no-store' })" in sync_body and "epgLiveTimerCache" in sync_body, "EPG Timer detail synchronization must keep live/no-store Timer semantics")

matrix = json.loads(read("docs/development/phase-69f-client-contract-matrix.json"))
require(matrix.get("explicitDeferredFallbacks") == [], "no deferred browser route fallback may remain after the Timer successor")
candidate = matrix.get("derivedNextRuntimeCandidate", {})
require(
    candidate.get("domain") in {"phase69f-closeout-audit", "phase69-complete"}
    and candidate.get("proposedTemplate") is None,
    "Timer hardening must allow closeout/final state without preselecting a public resource",
)
require(len(matrix.get("publicV1Resources", [])) >= 8, "Timer client hardening Phase-69 public-v1 baseline disappeared")

runtime_test = read("web/frontend/tests/test_phase69f_timer_live_fallback_removal.js")
for token in ("fetchClientTimers", "/api/vdr/timers/live", "backend_unavailable", "assert.strictEqual(requests.length, 1)", "requestJsonWithFallback"):
    require(token in runtime_test, "focused Timer live regression drifted: " + token)

doc = read("docs/development/phase-69f-timer-live-fallback-removal.md")
for token in ("main=fbaab982964c838e589dfce8adb62e14013e0e07", "PR #374", "/api/vdr/timers/live", "/api/vdr/timers", "getLiveTimers()", "server-owned", "epgLiveTimerCache", "No public-v1"):
    require(token in doc, "Timer live slice documentation drifted: " + token)

link = "[Phase 69.F Timer Live Single-Route Hardening]"
require(link + "(development/phase-69f-timer-live-fallback-removal.md)" in read("docs/CURRENT.md"), "CURRENT must link the Timer live slice")
for path in ("docs/development/phase-69-public-api-kickoff.md", "docs/development/index.md", "docs/development/web-client-api-contract-snapshot.md"):
    require(link + "(phase-69f-timer-live-fallback-removal.md)" in read(path), path + " must link the Timer live slice")

phase_make = read("mk/phase69-public-api-tests.mk")
for token in ("test-phase69f-timer-live-fallback-removal:", "test_phase69f_timer_live_fallback_removal.js", "check_phase69f_timer_live_fallback_removal.py"):
    require(token in phase_make, "Timer live Make wiring drifted: " + token)

maintenance = read("mk/maintenance-tests.mk")
require("python3 tools/check_phase69f_timer_live_fallback_removal.py" in maintenance, "architecture group must run Timer live guard")
require("test-phase69f-timer-live-fallback-removal" in maintenance, "phase group must include Timer live slice")
require("test-phase69f-timer-live-fallback-removal" in read("mk/test-groups.mk"), "fast CI must include Timer live slice")

print("Phase 69.F Timer live single-route guard passed.")
print("Browser Timer reads issue one live request and preserve structured failure evidence.")
print("Snapshot fallback ownership remains server-side and all client route fallback helpers are retired.")
