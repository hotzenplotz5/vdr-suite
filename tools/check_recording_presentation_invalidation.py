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
remote = (ROOT / "web/frontend/modules/remote.js").read_text()

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
require("'vdr-suite:home-resume'", remote,
        "canonical Home navigation must publish Home resume lifecycle")
require("refreshRecordingPresentationDependents", frontend,
        "recordings invalidation must refresh retained Recording Home projections")

print("recording presentation invalidation architecture ok")
