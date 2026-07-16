#!/usr/bin/env python3

from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]

required_files = (
    ROOT / "suitebridge_counter_continuity.h",
    ROOT / "suitebridge_counter_continuity.cpp",
    ROOT / "suitebridge_status_events.h",
    ROOT / "suitebridge_status_events.cpp",
    ROOT / "suitebridge_status_snapshot.h",
    ROOT / "suitebridge_status_snapshot.cpp",
    ROOT / "suitebridge_local_contract.h",
    ROOT / "suitebridge_local_contract.cpp",
    ROOT / "tests/test_suitebridge_counter_continuity.cpp",
    ROOT / "docs/SB-8-counter-continuity.md",
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
    for path in required_files
    if path.suffix in {".h", ".cpp"}
)

required_content = (
    "class SuiteBridgeCounterEpoch final",
    "HexLength() noexcept",
    "return 32;",
    "std::array<char, Capacity()> data_;",
    "std::atomic<std::uint64_t> instanceSequence",
    "std::chrono::steady_clock::now()",
    "std::chrono::system_clock::now()",
    "getpid()",
    "WriteHex64(",
    "class SuiteBridgeSaturatingCounter final",
    "compare_exchange_weak(",
    "std::numeric_limits<unsigned long long>::max()",
    "overflowed_.store(true",
    "SuiteBridgeCounterEpoch epoch_;",
    "CounterOverflowed() const noexcept",
    "CounterEpoch() const noexcept",
    "CounterEpochLength() noexcept",
    "return 2;",
    "CounterOverflow() const noexcept",
    '\\"counter_epoch\\"',
    '\\"counter_overflow\\"',
)

for fragment in required_content:
    if fragment not in combined:
        errors.append(f"missing counter continuity contract: {fragment}")

forbidden_content = (
    "std::string",
    "std::vector",
    "std::map",
    "std::unordered_map",
    "std::mutex",
    "std::random_device",
    "#include <random>",
    "#include <fstream>",
    "std::ifstream",
    "std::ofstream",
    "fopen(",
    "socket(",
    "bind(",
    "listen(",
    "connect(",
    "new ",
    "delete ",
)

for fragment in forbidden_content:
    if fragment in combined:
        errors.append(f"forbidden counter continuity implementation: {fragment}")

if errors:
    for error in errors:
        print(f"ERROR: {error}", file=sys.stderr)
    raise SystemExit(1)

print("suitebridge counter continuity contract ok")
