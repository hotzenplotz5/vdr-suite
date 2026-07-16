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
    "void RecordEvent(SuiteBridgeStatusEventKind kind) noexcept",
    "void ChannelSwitch(",
    "void Recording(",
    "void Replaying(",
    "void TimerChange(",
    "events_.Record(kind)",
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


def function_body(source: str, signature: str) -> str:
    start = source.find(signature)
    if start < 0:
        errors.append(f"missing callback implementation: {signature}")
        return ""

    opening = source.find("{", start)
    if opening < 0:
        errors.append(f"missing callback body: {signature}")
        return ""

    depth = 0
    for index in range(opening, len(source)):
        character = source[index]
        if character == "{":
            depth += 1
        elif character == "}":
            depth -= 1
            if depth == 0:
                return source[opening + 1:index]

    errors.append(f"unterminated callback body: {signature}")
    return ""


callback_contracts = (
    (
        "void SuiteBridgeStatusMonitor::ChannelSwitch(",
        "RecordEvent(SuiteBridgeStatusEventKind::ChannelSwitch);",
    ),
    (
        "void SuiteBridgeStatusMonitor::Recording(",
        "RecordEvent(SuiteBridgeStatusEventKind::Recording);",
    ),
    (
        "void SuiteBridgeStatusMonitor::Replaying(",
        "RecordEvent(SuiteBridgeStatusEventKind::Replaying);",
    ),
    (
        "void SuiteBridgeStatusMonitor::TimerChange(",
        "RecordEvent(SuiteBridgeStatusEventKind::TimerChange);",
    ),
)

callback_forbidden = (
    "isyslog(",
    "esyslog(",
    "LogSnapshot(",
    "CaptureSnapshot(",
    "SuiteBridgeLocalContractPayload",
    "socket(",
    "connect(",
    "system(",
    "fork(",
    "sleep(",
    "usleep(",
    "new ",
    "delete ",
)

for signature, required_call in callback_contracts:
    body = function_body(monitor_source, signature)

    if required_call not in body:
        errors.append(
            f"callback does not perform bounded atomic recording: {signature}"
        )

    for fragment in callback_forbidden:
        if fragment in body:
            errors.append(
                f"forbidden callback-side effect in {signature}: {fragment}"
            )

record_body = function_body(
    monitor_source,
    "void SuiteBridgeStatusMonitor::RecordEvent(",
)

for required_fragment in ("IsActive()", "events_.Record(kind)"):
    if required_fragment not in record_body:
        errors.append(
            f"missing bounded RecordEvent behavior: {required_fragment}"
        )

for fragment in callback_forbidden:
    if fragment in record_body:
        errors.append(f"forbidden RecordEvent side effect: {fragment}")

if errors:
    for error in errors:
        print(f"ERROR: {error}", file=sys.stderr)
    raise SystemExit(1)

print("suitebridge status-event contract ok")
