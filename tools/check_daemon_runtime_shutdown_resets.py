#!/usr/bin/env python3
from pathlib import Path
import re
import sys

SOURCE = Path("core/daemon/src/DaemonRuntime.cpp")
BACKEND_CONTEXT_SOURCE = Path("core/daemon/src/DaemonRuntimeBackendContext.cpp")
HTTP_CLIENT_HEADER = Path("core/http/include/BasicHttpClient.h")
HTTP_CLIENT_SOURCE = Path("core/http/src/BasicHttpClient.cpp")
DAEMON_SQLITE_SHUTDOWN_CANCELLATION_HEADER = Path(
    "core/daemon/include/DaemonSqliteShutdownCancellation.h")
SQLITE_SHUTDOWN_CANCELLATION_HEADER = Path(
    "core/sqlite/include/SqliteShutdownCancellation.h")

REQUIRED_RESETS = [
    "epgController_",
    "epgSearchResultJsonSerializer_",
    "epgSearchService_",
    "epgQueryService_",
    "epgSearchNativeFuzzyStartupRestoreService_",
    "epgSearchNativeFuzzyCapabilityFreshnessPolicy_",
    "epgSearchNativeFuzzyCapabilityDetector_",
    "epgSearchNativeFuzzyCapabilityRepository_",
    "searchTimerAutomationPreviewController_",
    "searchTimerAutomationDryRunResultJsonSerializer_",
    "searchTimerAutomationReadOnlyService_",
    "runtimeDiagnosticsJsonSerializer_",
]


def extract_shutdown_body(source: str) -> str:
    match = re.search(r"void\s+DaemonRuntime::shutdown\s*\(\)\s*\{", source)
    if not match:
        raise RuntimeError("DaemonRuntime::shutdown() not found")

    body_start = match.end()
    depth = 1
    index = body_start

    while index < len(source):
        character = source[index]
        if character == "{":
            depth += 1
        elif character == "}":
            depth -= 1
            if depth == 0:
                return source[body_start:index]
        index += 1

    raise RuntimeError("DaemonRuntime::shutdown() body not closed")


def main() -> int:
    source = SOURCE.read_text(encoding="utf-8")
    backend_context = BACKEND_CONTEXT_SOURCE.read_text(encoding="utf-8")
    http_client_header = HTTP_CLIENT_HEADER.read_text(encoding="utf-8")
    http_client_source = HTTP_CLIENT_SOURCE.read_text(encoding="utf-8")
    daemon_sqlite_cancellation_header = (
        DAEMON_SQLITE_SHUTDOWN_CANCELLATION_HEADER.read_text(encoding="utf-8"))
    sqlite_cancellation_header = (
        SQLITE_SHUTDOWN_CANCELLATION_HEADER.read_text(encoding="utf-8"))
    shutdown_body = extract_shutdown_body(source)

    missing = []
    for member in REQUIRED_RESETS:
        expected = f"{member}.reset();"
        if expected not in shutdown_body:
            missing.append(expected)

    recording_stop_request = shutdown_body.find(
        "recordingCacheWarmupStopRequested_.store(true);")
    epg_stop_request = shutdown_body.find(
        "epgCacheWarmupStopRequested_.store(true);")
    sqlite_cancellation = shutdown_body.find(
        "DaemonSqliteShutdownCancellation sqliteCancellation(")
    recording_join = shutdown_body.find("stopRecordingCacheWarmupWorker();")
    epg_join = shutdown_body.find("stopEpgCacheWarmupWorker();")

    if min(
        recording_stop_request,
        epg_stop_request,
        sqlite_cancellation,
        recording_join,
        epg_join,
    ) < 0:
        missing.append(
            "both cache stop requests and SQLite cancellation before joins")
    elif not (
        recording_stop_request < sqlite_cancellation < recording_join
        and epg_stop_request < sqlite_cancellation < recording_join
        and recording_stop_request < sqlite_cancellation < epg_join
        and epg_stop_request < sqlite_cancellation < epg_join
    ):
        missing.append(
            "cache stop flags and SQLite cancellation must precede both joins")

    if '#include "DaemonSqliteShutdownCancellation.h"' not in source:
        missing.append("DaemonRuntime SQLite shutdown cancellation include")
    if '#include "SqliteShutdownCancellation.h"' not in (
            daemon_sqlite_cancellation_header):
        missing.append("daemon compatibility alias to SQLite cancellation owner")
    if "using DaemonSqliteShutdownCancellation = " not in (
            daemon_sqlite_cancellation_header):
        missing.append("daemon SQLite cancellation compatibility alias")
    if "sqlite3_progress_handler(" not in sqlite_cancellation_header:
        missing.append("SQLite native progress cancellation")
    if "compare_exchange_strong(" not in sqlite_cancellation_header:
        missing.append("one-shot SQLite cancellation delivery")
    if "sqlite3_progress_handler(database_, 0, nullptr, nullptr);" not in (
            sqlite_cancellation_header):
        missing.append("SQLite progress handler cleanup after worker joins")

    if "using CancellationCheck = std::function<bool()>;" not in http_client_header:
        missing.append("BasicHttpClient cancellation contract")
    if "configureSocketPollingTimeouts" not in http_client_source:
        missing.append("BasicHttpClient cancellable socket polling")
    if "HTTP request cancelled" not in http_client_source:
        missing.append("BasicHttpClient cancellation result")
    if "return shutdownRequested_.load();" not in backend_context:
        missing.append("daemon shutdown cancellation callback wiring")

    if missing:
        print("DaemonRuntime shutdown reset check failed:")
        for item in missing:
            print(f"- missing {item}")
        return 1

    print("DaemonRuntime shutdown reset check passed.")
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except Exception as error:
        print(f"DaemonRuntime shutdown reset check failed: {error}")
        sys.exit(1)
