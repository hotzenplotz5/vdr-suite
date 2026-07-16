#!/usr/bin/env python3

from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]

required_files = (
    ROOT / "suitebridge_counter_continuity.h",
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
    "return 2;",
    "CounterEpochLength() noexcept",
    "return 32;",
    "operator=(const SuiteBridgeStatusSnapshot &) = delete;",
    "unsigned long long totalCount_;",
    "std::array<char, CounterEpochLength() + 1> counterEpoch_;",
    "bool counterOverflow_;",
    "AddSaturating(",
    "std::numeric_limits<unsigned long long>::max()",
    "CounterEpoch() const noexcept",
    "CounterOverflow() const noexcept",
    "CaptureSnapshot(bool monitorActive) const noexcept",
    "SuiteBridgeStatusMonitor::CaptureSnapshot() const noexcept",
    "status-snapshot schema=%u active=%s total=%llu",
    "counter-epoch=%s counter-overflow=%s",
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
