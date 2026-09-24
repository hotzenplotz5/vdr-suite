#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

runtime_h = (ROOT / "api/rest/include/ManualRecordingMetadataApiRuntime.h").read_text()
runtime_cpp = (ROOT / "api/rest/src/ManualRecordingMetadataApiRuntime.cpp").read_text()
daemon_h = (ROOT / "core/daemon/include/DaemonRuntime.h").read_text()
daemon_init = (ROOT / "core/daemon/src/DaemonRuntimeInitialization.cpp").read_text()
daemon_run = (ROOT / "core/daemon/src/DaemonRuntime.cpp").read_text()
daemon_publish = (ROOT / "core/daemon/src/DaemonRuntimeRecordingCache.cpp").read_text()
daemon_shutdown = (ROOT / "core/daemon/src/DaemonRuntimeShutdown.cpp").read_text()
frontend = (ROOT / "web/frontend/home-recording-discovery.js").read_text()
app = (ROOT / "web/frontend/app.js").read_text()
remote = (ROOT / "web/frontend/modules/remote.js").read_text()
continue_watching = (ROOT / "web/frontend/home-continue-watching.js").read_text()
history = (ROOT / "web/frontend/home-recently-watched.js").read_text()
hero = (ROOT / "web/frontend/home-live-hero.js").read_text()

def require(fragment, text, message):
    if fragment not in text:
        raise AssertionError(message)

require("registerRecordingPresentationChangedCallback", runtime_h,
        "manual metadata runtime callback contract missing")
require("notifyRecordingPresentationChanged(route.backendId);", runtime_cpp,
        "successful manual metadata mutation must publish presentation invalidation")
require("recordingPresentationChangeQueue_.request(backendId);", daemon_init,
        "metadata mutation must hand off to daemon-owned invalidation queue")
require("RecordingPresentationChangeQueue recordingPresentationChangeQueue_;", daemon_h,
        "daemon presentation invalidation queue missing")
require("publishRecordingPresentationChanges();", daemon_run,
        "daemon loop must drain presentation invalidations")

publication = daemon_publish[
    daemon_publish.index("void DaemonRuntime::publishRecordingPresentationChanges()"):
]
require("recordingPresentationChangeQueue_.takePending()", publication,
        "presentation invalidations must be coalesced and drained")
require("VdrChangeType::RecordingsChanged", publication,
        "presentation mutation must reuse canonical recordings change domain")
require("publishChangeFeedEntry(", publication,
        "presentation mutation must use existing live transport")
require("ManualRecordingMetadataApiRuntime::instance().reset();", daemon_shutdown,
        "shutdown must detach metadata invalidation callback")
require("'vdr-suite:home-resume'", app,
        "canonical app shell must publish Home resume lifecycle")
if "'vdr-suite:home-resume'" in remote:
    raise AssertionError("remote addon must not own or duplicate Home resume lifecycle")
for name, source in (
    ("Recording Discovery", frontend),
    ("Continue Watching", continue_watching),
    ("Home history", history),
    ("Live Hero", hero),
):
    require("'vdr-suite:home-resume'", source,
            name + " must subscribe to canonical Home resume lifecycle")
require("retainVisible: true", frontend,
        "Recording Discovery Home revalidation must retain visible UI")
require("parallelHomeResume", frontend,
        "Home revalidation must explicitly enable parallel Series refresh")
require("includeSeries: !parallelSeries", frontend,
        "Genres must retain cold-load Series ownership and skip it only for parallel Home return")
require("loadSeries(client, backendId, generation, [{id: 'series'}]", frontend,
        "known canonical Series must start in parallel on Home return")
if "refreshRecordingPresentationDependents" in frontend:
    raise AssertionError("Recording Discovery must not directly refresh foreign Home owners")

print("recording presentation invalidation architecture ok")