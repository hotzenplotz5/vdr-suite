#!/usr/bin/env python3
"""Keep failed inventory reads outstanding until a successful cache commit."""
from pathlib import Path
root = Path(__file__).resolve().parents[1]
worker = (root / "core/daemon/src/DaemonRuntimeRecordingCache.cpp").read_text()
polling = (root / "core/daemon/src/DaemonRuntimePolling.cpp").read_text()
runtime = (root / "core/daemon/src/DaemonRuntime.cpp").read_text()
refresh = worker[worker.index("void DaemonRuntime::refreshRecordingCacheForAllBackends("):
                 worker.index("void DaemonRuntime::publishCompletedRecordingRefreshes()")]
assert refresh.count("recordingCacheRefreshQueue_.failed(backendRuntimeContext->backendId)") == 3, \
    "repository rejection and both exception paths must retain dirty work"
for marker in ("else {", "catch (const std::exception& error)", "catch (...)"):
    section = refresh[refresh.index(marker):]
    assert section.index("recordingCacheRefreshQueue_.failed(") < section.index("markRefreshFailed(")
assert refresh.index("if (stored)") < refresh.index("recordingCacheRefreshQueue_.completed(")
assert "recordingCacheRefreshQueue_.request(backendRuntimeContext->backendId)" in polling
assert "publishCompletedRecordingRefreshes();" in runtime
publication = worker[worker.index("void DaemonRuntime::publishCompletedRecordingRefreshes()"):]
assert "takeCompleted()" in publication and "publishChangeFeedEntry(" in publication
print("Recording cache failure recovery and commit-before-feed wiring: PASS")
