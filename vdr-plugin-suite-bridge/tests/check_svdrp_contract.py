#!/usr/bin/env python3

from pathlib import Path
import sys


ROOT = Path(__file__).resolve().parents[1]

required_files = (
    ROOT / "suitebridge.h",
    ROOT / "suitebridge.cpp",
    ROOT / "suitebridge_svdrp.cpp",
    ROOT / "suitebridge_epg_command_handler.h",
    ROOT / "suitebridge_epg_command_handler.cpp",
    ROOT / "suitebridge_epg_type_snapshot_command.cpp",
    ROOT / "suitebridge_epg_type_snapshot_contract.h",
    ROOT / "suitebridge_epg_type_snapshot_contract.cpp",
    ROOT / "suitebridge_recording_metadata_command.h",
    ROOT / "suitebridge_recording_metadata_command.cpp",
    ROOT / "suitebridge_capability_discovery.h",
    ROOT / "suitebridge_capability_discovery.cpp",
    ROOT / "suitebridge_status_snapshot.h",
    ROOT / "suitebridge_status_snapshot.cpp",
    ROOT / "suitebridge_local_contract.h",
    ROOT / "suitebridge_local_contract.cpp",
    ROOT / "suitebridge_svdrp_contract.h",
    ROOT / "suitebridge_svdrp_contract.cpp",
    ROOT / "tests" / "test_suitebridge_capability_discovery.cpp",
    ROOT / "tests" / "test_suitebridge_epg_type_snapshot_contract.cpp",
    ROOT / "tests" / "test_suitebridge_svdrp_contract.cpp",
    ROOT / "docs" / "SB-6-read-only-svdrp.md",
    ROOT / "docs" / "SB-8-counter-continuity.md",
    ROOT / "docs" / "SB-9-capability-discovery.md",
)

implementation_files = (
    ROOT / "suitebridge.h",
    ROOT / "suitebridge.cpp",
    ROOT / "suitebridge_svdrp.cpp",
    ROOT / "suitebridge_epg_command_handler.cpp",
    ROOT / "suitebridge_epg_type_snapshot_command.cpp",
    ROOT / "suitebridge_epg_type_snapshot_contract.cpp",
    ROOT / "suitebridge_recording_metadata_command.cpp",
    ROOT / "suitebridge_capability_discovery.cpp",
    ROOT / "suitebridge_status_snapshot.cpp",
    ROOT / "suitebridge_local_contract.cpp",
    ROOT / "suitebridge_svdrp_contract.cpp",
)

boundary_files = (
    ROOT / "suitebridge.h",
    ROOT / "suitebridge.cpp",
    ROOT / "suitebridge_svdrp.cpp",
)

errors = []

for path in required_files:
    if not path.is_file():
        errors.append(
            f"missing file: {path.relative_to(ROOT)}"
        )

if errors:
    for error in errors:
        print(f"ERROR: {error}", file=sys.stderr)
    raise SystemExit(1)

combined = "\n".join(
    path.read_text(encoding="utf-8")
    for path in implementation_files
)

boundary_combined = "\n".join(
    path.read_text(encoding="utf-8")
    for path in boundary_files
)

required_content = (
    "const char **SVDRPHelpPages(void) override;",
    "cString SVDRPCommand(",
    "cPluginSuiteBridge::SVDRPHelpPages(void)",
    "cPluginSuiteBridge::SVDRPCommand(",

    '"CAPS [discovery-schema]\\n"',
    "Return the read-only VDR-Suite capability discovery payload.",

    '"SNAP\\n"',
    "Return the current read-only VDR-Suite status payload.",

    '"ARTW <channel-id> <event-id>\\n"',
    "Resolve preferred TVScraper artwork for one EPG event.",

    '"META <channel-id> <event-id>\\n"',
    "Resolve bounded TVScraper metadata for one EPG event.",

    '"ETYPES <from-epoch> <until-epoch> <offset> <limit>\\n"',
    "Return a bounded page of TVScraper movie/series types for real VDR EPG events.",

    '"RMETA <recording-key>\\n"',
    "Resolve bounded TVScraper metadata for one current VDR recording.",

    "SuiteBridgeEpgCommandHandler::HandleArtwork(",
    "SuiteBridgeEpgCommandHandler::HandleMetadata(",
    "SuiteBridgeEpgCommandHandler::HandleTypeSnapshot(",
    "SuiteBridgeRecordingMetadataCommand::Handle(",

    "SuiteBridgeEpgArtworkRequest",
    "SuiteBridgeEpgMetadataRequest",
    "SuiteBridgeEpgTypeSnapshotRequest",
    "SuiteBridgeRecordingMetadataRequest",

    "SuiteBridgeCapabilityDiscoveryReply",
    "statusMonitor_.CaptureSnapshot()",

    "svdrp command=CAPS result=served",
    "svdrp command=CAPS result=rejected",
    "svdrp command=SNAP result=served",
    "svdrp command=SNAP result=rejected",
    "svdrp command=ARTW result=served",
    "svdrp command=META result=served",
    "svdrp command=ETYPES result=served",
    "svdrp command=RMETA result=served",
)

for fragment in required_content:
    if fragment not in combined:
        errors.append(
            f"missing SVDRP contract: {fragment}"
        )

forbidden_boundary_content = (
    "#include <thread>",
    "std::thread",
    "cThread",
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

for fragment in forbidden_boundary_content:
    if fragment in boundary_combined:
        errors.append(
            f"forbidden SVDRP top-level implementation: {fragment}"
        )

if errors:
    for error in errors:
        print(f"ERROR: {error}", file=sys.stderr)
    raise SystemExit(1)

print("suitebridge read-only SVDRP contract ok")
