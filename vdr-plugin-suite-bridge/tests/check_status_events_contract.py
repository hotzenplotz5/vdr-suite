#!/usr/bin/env python3

from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]

required_files = (
    ROOT / "suitebridge_status_events.h",
    ROOT / "suitebridge_status_events.cpp",
    ROOT / "suitebridge_status_monitor.h",
    ROOT / "suitebridge_status_monitor.cpp",
    ROOT / "tests/test_suitebridge_status_events.cpp",
)

errors = []

for path in required_files:
    if not path.is_file():
        errors.append(f"missing file: {path.relative_to(ROOT)}")

if errors:
    for error in errors:
        print(f"ERROR: {error}", file=sys.stderr)
    raise SystemExit(1)

events_header = (ROOT / "suitebridge_status_events.h").read_text(
    encoding="utf-8"
)
events_source = (ROOT / "suitebridge_status_events.cpp").read_text(
    encoding="utf-8"
)
monitor_header = (ROOT / "suitebridge_status_monitor.h").read_text(
    encoding="utf-8"
)
monitor_source = (ROOT / "suitebridge_status_monitor.cpp").read_text(
    encoding="utf-8"
)
combined = "\n".join(
    (events_header, events_source, monitor_header, monitor_source)
)

required_content = (
    "enum class SuiteBridgeStatusEventKind",
    "SuiteBridgeStatusEventKind::ChannelSwitch",
    "SuiteBridgeStatusEventKind::Recording",
    "SuiteBridgeStatusEventKind::Replaying",
    "SuiteBridgeStatusEventKind::TimerChange",
    "std::atomic<unsigned long long>",
    "class SuiteBridgeStatusMonitor final : public cStatus",
    "void Activate() noexcept",
    "void Deactivate() noexcept",
    "void ChannelSwitch(",
    "void Recording(",
    "void Replaying(",
    "void TimerChange(",
    "status-event type=channel-switch",
    "status-event type=recording",
    "status-event type=replaying",
    "status-event type=timer-change",
)

for fragment in required_content:
    if fragment not in combined:
        errors.append(f"missing status-event contract: {fragment}")

forbidden_content = (
    "#include <thread>",
    "std::thread",
    "cThread",
    "std::vector",
    "std::queue",
    "new ",
    "delete ",
    "socket(",
    "bind(",
    "listen(",
    "connect(",
)

for fragment in forbidden_content:
    if fragment in combined:
        errors.append(f"forbidden status-event implementation: {fragment}")

if errors:
    for error in errors:
        print(f"ERROR: {error}", file=sys.stderr)
    raise SystemExit(1)

print("suitebridge status-event contract ok")
