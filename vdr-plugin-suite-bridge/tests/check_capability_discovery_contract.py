#!/usr/bin/env python3

from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]

required_files = (
    ROOT / "suitebridge_capabilities.h",
    ROOT / "suitebridge_capabilities.cpp",
    ROOT / "suitebridge_capability_discovery.h",
    ROOT / "suitebridge_capability_discovery.cpp",
    ROOT / "suitebridge_status_snapshot.h",
    ROOT / "suitebridge_local_contract.h",
    ROOT / "suitebridge_plugin_identity.h",
    ROOT / "suitebridge_svdrp.cpp",
    ROOT / "tests/test_suitebridge_capability_discovery.cpp",
    ROOT / "docs/SB-9-capability-discovery.md",
)

capability_implementation_files = (
    ROOT / "suitebridge_capabilities.h",
    ROOT / "suitebridge_capabilities.cpp",
    ROOT / "suitebridge_capability_discovery.h",
    ROOT / "suitebridge_capability_discovery.cpp",
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

capability_implementation = "\n".join(
    path.read_text(encoding="utf-8")
    for path in capability_implementation_files
)

plugin_source = (ROOT / "suitebridge_svdrp.cpp").read_text(encoding="utf-8")
discovery_source = (
    ROOT / "suitebridge_capability_discovery.cpp"
).read_text(encoding="utf-8")

required_content = (
    "class SuiteBridgeCapabilityDiscoveryPayload final",
    "SchemaVersion() noexcept",
    "return 1;",
    "BufferSize = 768",
    "std::array<char, BufferSize> data_;",
    "SuiteBridgeCapabilities::All()",
    "SuiteBridgeCapabilities::StateName(capability.state)",
    "SuiteBridgeCapabilities::SchemaVersion()",
    "SuiteBridgeStatusSnapshot::SchemaVersion()",
    "SuiteBridgeLocalContractPayload::SchemaVersion()",
    '\\"discovery_schema\\"',
    '\\"plugin_name\\"',
    '\\"plugin_version\\"',
    '\\"capability_schema\\"',
    '\\"snapshot_schema\\"',
    '\\"local_contract_schema\\"',
    '\\"capabilities\\"',
    "class SuiteBridgeCapabilityDiscoveryReply final",
    'return "CAPS";',
    "return 900;",
    "return 501;",
    "return 504;",
    "return 451;",
    "strcasecmp(command, CommandName())",
    "ParseRequestedSchema(option)",
    'inline constexpr const char *Name = "suitebridge";',
    'inline constexpr const char *Version = "0.13.3";',
    "SuiteBridgeCapabilityDiscoveryReply capabilityReply(",
    "svdrp command=CAPS result=served reply=%d bytes=%zu schema=%u",
    "svdrp command=CAPS result=rejected reply=%d",
)

for fragment in required_content:
    if fragment not in combined:
        errors.append(f"missing capability discovery contract: {fragment}")

for capability_id, state in (
    ("lifecycle", "available"),
    ("status-events", "available"),
    ("snapshots", "available"),
    ("local-contract", "available"),
    ("recording-metadata", "available"),
    ("epg-type-snapshot", "available"),
    ("mutations", "disabled"),
):
    source_fragment = (
        f'{{"{capability_id}", '
        f'SuiteBridgeCapabilityState::'
        f'{"Available" if state == "available" else "Disabled"}'
        "}"
    )

    if source_fragment not in combined:
        errors.append(
            f"missing discovery capability source: {capability_id}={state}"
        )

discovery_dispatch = plugin_source.find(
    "SuiteBridgeCapabilityDiscoveryReply capabilityReply("
)
discovery_handled = plugin_source.find("if (capabilityReply.Handled())")
snapshot_reply = plugin_source.find(
    "const SuiteBridgeSvdrpReply snapshotReply("
)
snapshot_capture = plugin_source.find(
    "statusMonitor_.CaptureSnapshot()",
    max(snapshot_reply, 0),
)

if (
    discovery_dispatch < 0
    or discovery_handled < 0
    or snapshot_reply < 0
    or snapshot_capture < 0
    or not (
        discovery_dispatch
        < discovery_handled
        < snapshot_reply
        <= snapshot_capture
    )
):
    errors.append(
        "CAPS must be dispatched and returned before SNAP captures a snapshot"
    )

if "statusMonitor_" in discovery_source or "CaptureSnapshot" in discovery_source:
    errors.append(
        "capability discovery implementation must not access the status monitor"
    )

forbidden_content = (
    "std::string",
    "std::vector",
    "std::map",
    "std::unordered_map",
    "std::mutex",
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
    if fragment in capability_implementation:
        errors.append(
            f"forbidden capability discovery implementation: {fragment}"
        )

if errors:
    for error in errors:
        print(f"ERROR: {error}", file=sys.stderr)
    raise SystemExit(1)

print("suitebridge capability discovery contract ok")
