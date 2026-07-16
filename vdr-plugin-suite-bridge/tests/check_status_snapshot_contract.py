#!/usr/bin/env python3

from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]

required_files = (
    ROOT / "suitebridge_status_snapshot.h",
    ROOT / "suitebridge_status_snapshot.cpp",
    ROOT / "suitebridge_status_events.h",
    ROOT / "suitebridge_status_events.cpp",
    ROOT / "suitebridge_status_monitor.h",
    ROOT / "suitebridge_status_monitor.cpp",
    ROOT / "tests/test_suitebridge_status_snapshot.cpp",
)

errors = []

for path in required_files:
    if not path.is_file():
        errors.append(f"missing file: {path.relative_to(ROOT)}")

if errors:
    for error in errors:
        print(f"ERROR: {error}", file=sys.stderr)
    raise SystemExit(1)

combined = "\n".join(
    path.read_text(encoding="utf-8")
    for path in required_files[:-1]
)

required_content = (
    "class SuiteBridgeStatusSnapshot final",
    "SchemaVersion() noexcept",
    "return 1;",
    "operator=(const SuiteBridgeStatusSnapshot &) = delete;",
    "const bool monitorActive_;",
    "const unsigned long long channelSwitchCount_;",
    "const unsigned long long recordingCount_;",
    "const unsigned long long replayingCount_;",
    "const unsigned long long timerChangeCount_;",
    "CaptureSnapshot(bool monitorActive) const noexcept",
    "SuiteBridgeStatusMonitor::CaptureSnapshot() const noexcept",
    "status-snapshot schema=%u active=%s total=%llu",
)

for fragment in required_content:
    if fragment not in combined:
        errors.append(f"missing status snapshot contract: {fragment}")

forbidden_content = (
    "std::vector",
    "std::map",
    "std::unordered_map",
    "std::mutex",
    "new ",
    "delete ",
    "socket(",
    "connect(",
)

for fragment in forbidden_content:
    if fragment in combined:
        errors.append(f"forbidden status snapshot implementation: {fragment}")

if errors:
    for error in errors:
        print(f"ERROR: {error}", file=sys.stderr)
    raise SystemExit(1)

print("suitebridge status snapshot contract ok")
