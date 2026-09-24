#!/usr/bin/env python3
"""Guard the bounded SuiteBridge recording-list dirty-hint path."""

from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]

PLUGIN_HEADER = ROOT / "vdr-plugin-suite-bridge/suitebridge.h"
PLUGIN_SOURCE = ROOT / "vdr-plugin-suite-bridge/suitebridge.cpp"
MONITOR_HEADER = ROOT / "vdr-plugin-suite-bridge/suitebridge_status_monitor.h"
MONITOR_SOURCE = ROOT / "vdr-plugin-suite-bridge/suitebridge_status_monitor.cpp"
EVENTS_HEADER = ROOT / "vdr-plugin-suite-bridge/suitebridge_status_events.h"
SNAPSHOT_HEADER = ROOT / "vdr-plugin-suite-bridge/suitebridge_status_snapshot.h"
LOCAL_CONTRACT = ROOT / "vdr-plugin-suite-bridge/suitebridge_local_contract.cpp"
HANDSHAKE = ROOT / "core/agent/include/SuiteBridgeHandshake.h"
PARSER = ROOT / "core/agent/src/SuiteBridgeLocalContractParser.cpp"
CONTEXT = ROOT / "core/daemon/include/BackendRuntimeContext.h"
POLLING = ROOT / "core/daemon/src/DaemonRuntimePolling.cpp"
CACHE = ROOT / "core/daemon/src/DaemonRuntimeRecordingCache.cpp"

paths = [
    PLUGIN_HEADER, PLUGIN_SOURCE, MONITOR_HEADER, MONITOR_SOURCE,
    EVENTS_HEADER, SNAPSHOT_HEADER, LOCAL_CONTRACT, HANDSHAKE,
    PARSER, CONTEXT, POLLING, CACHE,
]
errors = []
for path in paths:
    if not path.is_file():
        errors.append(f"missing required file: {path.relative_to(ROOT)}")

def require(path: Path, fragments: tuple[str, ...]) -> None:
    if not path.is_file():
        return
    text = path.read_text(encoding="utf-8")
    for fragment in fragments:
        if fragment not in text:
            errors.append(
                f"{path.relative_to(ROOT)} misses required fragment: {fragment}"
            )

require(PLUGIN_HEADER, ("void MainThreadHook(void) override;",))
require(
    PLUGIN_SOURCE,
    (
        "void cPluginSuiteBridge::MainThreadHook(void)",
        "statusMonitor_.ObserveRecordingListState();",
    ),
)
require(
    MONITOR_HEADER,
    (
        "void ObserveRecordingListState() noexcept;",
        "cStateKey recordingsStateKey_;",
    ),
)
require(
    MONITOR_SOURCE,
    (
        "cRecordings::GetRecordingsRead(recordingsStateKey_, 1)",
        "recordingsStateKey_.Remove();",
        "RecordEvent(SuiteBridgeStatusEventKind::RecordingList);",
    ),
)
require(EVENTS_HEADER, ("SuiteBridgeStatusEventKind", "RecordingList",))
require(SNAPSHOT_HEADER, ("return 4;", "RecordingListCount() const noexcept;",))
require(LOCAL_CONTRACT, ("recording_list", "snapshot.RecordingListCount()"))
require(HANDSHAKE, ("std::uint64_t recordingList = 0;",))
require(PARSER, ('key == "recording_list"', "value.recordingList"))
require(
    CONTEXT,
    (
        "SuiteBridgeCounterChangeTracker recordingListChangeTracker;",
        "SuiteBridgeCounterChangeTracker recordingMarksChangeTracker;",
    ),
)
require(
    POLLING,
    (
        "recordingListChangeTracker.observe(",
        "observation.baseline.recordingList",
        "recordingCacheRefreshQueue_.request(",
    ),
)
require(
    CACHE,
    (
        "recordingCacheRefreshQueue_.completed(",
        "publishCompletedRecordingRefreshes()",
        "publishChangeFeedEntry(",
    ),
)

if MONITOR_SOURCE.is_file():
    monitor = MONITOR_SOURCE.read_text(encoding="utf-8")
    start = monitor.find("void SuiteBridgeStatusMonitor::ObserveRecordingListState()")
    end = monitor.find(
        "unsigned long long SuiteBridgeStatusMonitor::EventCount(", start
    )
    body = monitor[start:end] if start >= 0 and end > start else ""
    for forbidden in (
        "cRecordings::Update(",
        "TouchUpdate(",
        "std::thread",
        "cThread",
        "sleep(",
        "usleep(",
        "std::filesystem",
        "opendir(",
        "readdir(",
        "system(",
        "popen(",
        "socket(",
    ):
        if forbidden in body:
            errors.append(f"recording-list observer contains forbidden work: {forbidden}")

if POLLING.is_file():
    polling = POLLING.read_text(encoding="utf-8")
    marker = "recordingListChangeTracker.observe("
    next_marker = "recordingMarksChangeTracker.observe("
    start = polling.find(marker)
    end = polling.find(next_marker, start) if start >= 0 else -1
    block = polling[start:end] if start >= 0 and end > start else ""
    if not block:
        errors.append("unable to isolate SuiteBridge recording-list hint block")
    elif "publishChangeFeedEntry(" in block or "replaceRecordingsForBackend(" in block:
        errors.append("SuiteBridge list hint must not publish or mutate cache directly")

if errors:
    for error in errors:
        print(f"ERROR: {error}", file=sys.stderr)
    raise SystemExit(1)

print("SuiteBridge recording-list dirty-hint boundary: PASS")
