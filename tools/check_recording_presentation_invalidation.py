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
index = (ROOT / "web/frontend/index.html").read_text()

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
if "new global.IntersectionObserver" in frontend or "rootMargin: '320px 0px'" in frontend:
    raise AssertionError("Home Recording rails must not be gated by viewport intersection")
discovery_script = '<script src="../frontend/home-recording-discovery.js"></script>'
hero_script = '<script src="../frontend/home-live-hero.js"></script>'
require(discovery_script, index,
        "Home must register Recording Discovery in the production shell")
require(hero_script, index,
        "Home must register the Live Hero in the production shell")
if index.index(discovery_script) > index.index(hero_script):
    raise AssertionError("Recording Home owners must register before the Live Hero")
require("scheduleSync(false, {", hero,
        "Live Hero Home resume must be deferred behind primary Home owners")
require("revalidatePrograms: true", hero,
        "deferred Live Hero resume must still revalidate Now/Next")
require("state.loadingPrograms && state.events.length > 0", hero,
        "retained Now/Next must not be rebuilt into a loading state")
require("Promise.allSettled(loads)", frontend,
        "Recording Discovery must launch rail work as independent promises")
require("loadNewly(client, backendId, generation, {", frontend,
        "Newly Recorded must be an independent refresh owner")
require("loadGenres(client, backendId, generation, {", frontend,
        "Genres must be an independent refresh owner")
require("loadSeries(client, backendId, generation, [{id: 'series'}]", frontend,
        "Series must start from its canonical membership endpoint independently")
require("loadFolders(client, backendId, generation, {", frontend,
        "Recording folders must be an independent refresh owner")
if "parallelHomeResume" in frontend or "includeSeries" in frontend:
    raise AssertionError("Recording Discovery must not gate Series behind Genres")
if "recordingRefreshBusy" in frontend:
    raise AssertionError("Recording feed refresh must not wait for the slowest older rail")

genre_start = frontend.index("function loadGenres(")
genre_end = frontend.index("function scheduleRandomFolderInline(", genre_start)
if "loadSeries(" in frontend[genre_start:genre_end]:
    raise AssertionError("Genres must not own or await Series refresh")

feed_start = frontend.index("function scheduleRecordingChangeRefresh()")
feed_end = frontend.index("function stopRecordingChanges()", feed_start)
feed_refresh = frontend[feed_start:feed_end]
require("retainVisible: true", feed_refresh,
        "recording feed refresh must preserve visible Home presentation")
if "coalesce: true" in feed_refresh:
    raise AssertionError("new recording generations must supersede slow older refreshes")
if "refreshRecordingPresentationDependents" in frontend:
    raise AssertionError("Recording Discovery must not directly refresh foreign Home owners")

print("recording presentation invalidation architecture ok")