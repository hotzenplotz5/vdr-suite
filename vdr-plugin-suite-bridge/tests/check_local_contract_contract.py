#!/usr/bin/env python3

from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]

required_files = (
    ROOT / "suitebridge_local_contract.h",
    ROOT / "suitebridge_local_contract.cpp",
    ROOT / "suitebridge_status_snapshot.h",
    ROOT / "tests/test_suitebridge_local_contract.cpp",
)

errors = []

for path in required_files:
    if not path.is_file():
        errors.append(f"missing file: {path.relative_to(ROOT)}")

if errors:
    for error in errors:
        print(f"ERROR: {error}", file=sys.stderr)
    raise SystemExit(1)

header = (ROOT / "suitebridge_local_contract.h").read_text(encoding="utf-8")
source = (ROOT / "suitebridge_local_contract.cpp").read_text(encoding="utf-8")
test = (ROOT / "tests/test_suitebridge_local_contract.cpp").read_text(
    encoding="utf-8"
)
combined = "\n".join((header, source, test))

required_content = (
    "class SuiteBridgeLocalContractPayload final",
    "SchemaVersion() noexcept",
    "return 1;",
    "Capacity() noexcept",
    "static constexpr std::size_t kCapacity = 320;",
    "operator=(\n      const SuiteBridgeLocalContractPayload &) = delete;",
    "std::array<char, kCapacity> data_;",
    "std::snprintf(",
    '\\"contract_schema\\"',
    '\\"capability_schema\\"',
    '\\"snapshot_schema\\"',
    '\\"channel_switch\\"',
    '\\"timer_change\\"',
    "bool SuiteBridgeLocalContractPayload::Complete() const noexcept",
)

for fragment in required_content:
    if fragment not in combined:
        errors.append(f"missing local contract payload: {fragment}")

forbidden_content = (
    "std::string",
    "std::vector",
    "std::map",
    "std::unordered_map",
    "std::mutex",
    "new ",
    "delete ",
    "socket(",
    "bind(",
    "listen(",
    "connect(",
)

for fragment in forbidden_content:
    if fragment in combined:
        errors.append(f"forbidden local contract implementation: {fragment}")

if errors:
    for error in errors:
        print(f"ERROR: {error}", file=sys.stderr)
    raise SystemExit(1)

print("suitebridge local contract payload ok")
