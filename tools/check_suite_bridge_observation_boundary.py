#!/usr/bin/env python3

from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[1]

REQUIRED = [
    ROOT / "core/agent/include/SuiteBridgeObservation.h",
    ROOT / "core/agent/include/SuiteBridgeObservationService.h",
    ROOT / "core/agent/include/SuiteBridgeObservationWorker.h",
    ROOT / "core/agent/src/SuiteBridgeObservation.cpp",
    ROOT / "core/agent/src/SuiteBridgeObservationService.cpp",
    ROOT / "core/agent/src/SuiteBridgeObservationWorker.cpp",
    ROOT / "core/agent/tests/test_suite_bridge_observation_service.cpp",
    ROOT / "core/agent/tests/test_suite_bridge_observation_worker.cpp",
]

SERVICE_FILES = [
    ROOT / "core/agent/include/SuiteBridgeObservation.h",
    ROOT / "core/agent/include/SuiteBridgeObservationService.h",
    ROOT / "core/agent/src/SuiteBridgeObservation.cpp",
    ROOT / "core/agent/src/SuiteBridgeObservationService.cpp",
]

ALL_SOURCE_FORBIDDEN = [
    "popen(",
    "system(",
    "fork(",
    "execv",
    "sqlite3",
    "Database.h",
    "DaemonRuntime.h",
    "BackendRuntimeContext.h",
    "RestfulApi",
    "SvdrpChannelMoveExecutor",
    "vdr-plugin-suite-bridge/",
    "std::filesystem",
    "std::fstream",
    "svdrpsend",
]

SERVICE_FORBIDDEN_LITERALS = [
    "std::thread",
    "std::mutex",
    "std::condition_variable",
    "sleep_for",
    "sleep_until",
    "usleep(",
]

SERVICE_FORBIDDEN_CALLS = [
    "socket",
    "connect",
    "poll",
]

REQUIRED_STATES = [
    "NotConfigured",
    "Connecting",
    "PluginMissing",
    "LegacyOrUnknown",
    "Incompatible",
    "Compatible",
    "SnapshotCurrent",
    "SnapshotStale",
    "TransportDegraded",
    "Overflowed",
    "Offline",
]

REQUIRED_SERVICE_FRAGMENTS = [
    "pollInterval{5000}",
    "staleAfter{15000}",
    "offlineAfter{60000}",
    "reconnectInitial{1000}",
    "reconnectMaximum{30000}",
    "handshakeService_.discover()",
    "handshakeService_.readSnapshot(snapshot_.discovery)",
    "RejectedCounterRegression",
    "counter epoch changed; baseline replaced",
    "counter continuity overflowed",
    "snapshot_.mutationsEnabled = false",
]

REQUIRED_WORKER_FRAGMENTS = [
    "std::thread",
    "std::condition_variable",
    "wait_until(",
    "notify_all()",
    "thread_.join()",
    "~SuiteBridgeObservationWorker()",
]

REQUIRED_TEST_FRAGMENTS = [
    "test_suite_bridge_observation_service passed",
    "test_suite_bridge_observation_worker passed",
    "SuiteBridgeLocalCommand::DiscoverSchema1",
    "SuiteBridgeLocalCommand::Snapshot",
    "SnapshotStale",
    "TransportDegraded",
    "PluginMissing",
    "LegacyOrUnknown",
    "Overflowed",
]


def contains_standalone_call(text: str, name: str) -> bool:
    pattern = re.compile(
        rf"(?<![A-Za-z0-9_]){re.escape(name)}\s*\("
    )
    return pattern.search(text) is not None


errors: list[str] = []

if not contains_standalone_call("connect(fd, address, length);", "connect"):
    errors.append("standalone-call matcher does not detect connect()")

if not contains_standalone_call("::connect(fd, address, length);", "connect"):
    errors.append("standalone-call matcher does not detect ::connect()")

if contains_standalone_call("scheduleReconnect(now);", "connect"):
    errors.append("standalone-call matcher rejects scheduleReconnect()")

if contains_standalone_call("reconnectDelay();", "connect"):
    errors.append("standalone-call matcher rejects reconnectDelay()")

for path in REQUIRED:
    if not path.is_file():
        errors.append(
            f"missing required SB.10c file: {path.relative_to(ROOT)}"
        )

source_files = [
    path
    for path in REQUIRED
    if path.is_file() and "/tests/" not in path.as_posix()
]

for path in source_files:
    text = path.read_text(encoding="utf-8")

    for token in ALL_SOURCE_FORBIDDEN:
        if token in text:
            errors.append(
                f"forbidden observation dependency {token!r} in "
                f"{path.relative_to(ROOT)}"
            )

for path in SERVICE_FILES:
    if not path.is_file():
        continue

    text = path.read_text(encoding="utf-8")

    for token in SERVICE_FORBIDDEN_LITERALS:
        if token in text:
            errors.append(
                f"deterministic observation service contains {token!r} in "
                f"{path.relative_to(ROOT)}"
            )

    for call_name in SERVICE_FORBIDDEN_CALLS:
        if contains_standalone_call(text, call_name):
            errors.append(
                "deterministic observation service contains standalone "
                f"{call_name}() call in {path.relative_to(ROOT)}"
            )

observation_header = ROOT / "core/agent/include/SuiteBridgeObservation.h"
if observation_header.is_file():
    text = observation_header.read_text(encoding="utf-8")

    for state in REQUIRED_STATES:
        if state not in text:
            errors.append(f"missing observation state: {state}")

    for fragment in REQUIRED_SERVICE_FRAGMENTS[:5]:
        if fragment not in text:
            errors.append(f"missing observation timing contract: {fragment}")

service_source = ROOT / "core/agent/src/SuiteBridgeObservationService.cpp"
if service_source.is_file():
    text = service_source.read_text(encoding="utf-8")

    for fragment in REQUIRED_SERVICE_FRAGMENTS[5:]:
        if fragment not in text:
            errors.append(f"missing observation service contract: {fragment}")

worker_header = ROOT / "core/agent/include/SuiteBridgeObservationWorker.h"
worker_source = ROOT / "core/agent/src/SuiteBridgeObservationWorker.cpp"
worker_text = ""

if worker_header.is_file():
    worker_text += worker_header.read_text(encoding="utf-8")

if worker_source.is_file():
    worker_text += worker_source.read_text(encoding="utf-8")

for fragment in REQUIRED_WORKER_FRAGMENTS:
    if fragment not in worker_text:
        errors.append(f"missing interruptible worker contract: {fragment}")

if "detach(" in worker_text:
    errors.append("observation worker must never detach its thread")

if "sleep_for" in worker_text or "usleep(" in worker_text:
    errors.append("observation worker must use interruptible condition waits")

test_text = ""
for path in REQUIRED:
    if path.is_file() and "/tests/" in path.as_posix():
        test_text += path.read_text(encoding="utf-8")

for fragment in REQUIRED_TEST_FRAGMENTS:
    if fragment not in test_text:
        errors.append(f"missing SB.10c regression evidence: {fragment}")

plugin_directory = ROOT / "vdr-plugin-suite-bridge"
for forbidden_name in (
    "suitebridge_observation.cpp",
    "suitebridge_worker.cpp",
    "suitebridge_polling.cpp",
):
    if (plugin_directory / forbidden_name).exists():
        errors.append(
            "Agent observation implementation must not be placed in plugin: "
            + forbidden_name
        )

if errors:
    for error in errors:
        print(f"ERROR: {error}")
    sys.exit(1)

print("suite bridge observation boundary contract ok")
