#!/usr/bin/env python3

from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]

required_files = (
    ROOT / "suitebridge.h",
    ROOT / "suitebridge.cpp",
    ROOT / "suitebridge_svdrp_contract.h",
    ROOT / "suitebridge_svdrp_contract.cpp",
    ROOT / "tests/test_suitebridge_svdrp_contract.cpp",
    ROOT / "docs/SB-6-read-only-svdrp.md",
)

errors = []

for path in required_files:
    if not path.is_file():
        errors.append(f"missing file: {path.relative_to(ROOT)}")

if errors:
    for error in errors:
        print(f"ERROR: {error}", file=sys.stderr)
    raise SystemExit(1)

header = (ROOT / "suitebridge.h").read_text(encoding="utf-8")
source = (ROOT / "suitebridge.cpp").read_text(encoding="utf-8")
contract_header = (ROOT / "suitebridge_svdrp_contract.h").read_text(
    encoding="utf-8"
)
contract_source = (ROOT / "suitebridge_svdrp_contract.cpp").read_text(
    encoding="utf-8"
)
test = (ROOT / "tests/test_suitebridge_svdrp_contract.cpp").read_text(
    encoding="utf-8"
)

combined = "\n".join(
    (
        header,
        source,
        contract_header,
        contract_source,
        test,
    )
)

required_content = (
    "const char **SVDRPHelpPages(void) override;",
    "cString SVDRPCommand(",
    "cPluginSuiteBridge::SVDRPHelpPages(void)",
    "cPluginSuiteBridge::SVDRPCommand(",
    '"SNAP\\n"',
    "Return the current read-only VDR-Suite status payload.",
    "class SuiteBridgeSvdrpReply final",
    'return "SNAP";',
    "return 900;",
    "return 504;",
    "return 451;",
    "strcasecmp(command, CommandName())",
    "statusMonitor_.CaptureSnapshot()",
    "SuiteBridgeCapabilities::SchemaVersion()",
    "svdrp command=SNAP result=served reply=%d bytes=%zu",
    "svdrp command=SNAP result=rejected reply=%d",
    "return nullptr;",
)

for fragment in required_content:
    if fragment not in combined:
        errors.append(f"missing SVDRP contract: {fragment}")

forbidden_content = (
    "#include <thread>",
    "std::thread",
    "cThread",
    "std::string",
    "std::vector",
    "std::queue",
    "std::map",
    "std::unordered_map",
    "socket(",
    "bind(",
    "listen(",
    "connect(",
    "system(",
    "fork(",
    "execv(",
)

for fragment in forbidden_content:
    if fragment in combined:
        errors.append(f"forbidden SVDRP implementation: {fragment}")

if errors:
    for error in errors:
        print(f"ERROR: {error}", file=sys.stderr)
    raise SystemExit(1)

print("suitebridge read-only SVDRP contract ok")
