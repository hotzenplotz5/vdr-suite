#!/usr/bin/env python3

from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]

required_files = (
    ROOT / "suitebridge_capabilities.h",
    ROOT / "suitebridge_capabilities.cpp",
    ROOT / "tests/test_suitebridge_capabilities.cpp",
)

errors = []

for path in required_files:
    if not path.is_file():
        errors.append(f"missing file: {path.relative_to(ROOT)}")

if errors:
    for error in errors:
        print(f"ERROR: {error}", file=sys.stderr)
    raise SystemExit(1)

header = (ROOT / "suitebridge_capabilities.h").read_text(encoding="utf-8")
source = (ROOT / "suitebridge_capabilities.cpp").read_text(encoding="utf-8")
combined = header + "\n" + source

required_content = (
    "enum class SuiteBridgeCapabilityState",
    "SuiteBridgeCapabilityState::Available",
    "SuiteBridgeCapabilityState::Planned",
    "SuiteBridgeCapabilityState::Disabled",
    "SchemaVersion() noexcept",
    "return 1;",
    '{"lifecycle", SuiteBridgeCapabilityState::Available}',
    '{"status-events", SuiteBridgeCapabilityState::Available}',
    '{"snapshots", SuiteBridgeCapabilityState::Available}',
    '{"local-contract", SuiteBridgeCapabilityState::Available}',
    '{"recording-metadata", SuiteBridgeCapabilityState::Available}',
    '{"recording-marks", SuiteBridgeCapabilityState::Available}',
    '{"epg-type-snapshot", SuiteBridgeCapabilityState::Available}',
    '{"mutations", SuiteBridgeCapabilityState::Disabled}',
    "SuiteBridgeCapabilities::Find(",
    "SuiteBridgeCapabilities::IsAvailable(",
)

for fragment in required_content:
    if fragment not in combined:
        errors.append(f"missing capability contract: {fragment}")

forbidden_content = (
    "std::vector",
    "std::map",
    "std::unordered_map",
    "new ",
    "delete ",
    "socket(",
    "connect(",
)

for fragment in forbidden_content:
    if fragment in combined:
        errors.append(f"forbidden capability implementation: {fragment}")

if errors:
    for error in errors:
        print(f"ERROR: {error}", file=sys.stderr)
    raise SystemExit(1)

print("suitebridge capability contract ok")
