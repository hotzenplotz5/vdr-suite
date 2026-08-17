#!/usr/bin/env python3

from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
REQUIRED_FILES = (
    "Makefile",
    "README.md",
    "suitebridge.h",
    "suitebridge.cpp",
    "suitebridge_plugin_identity.h",
    "suitebridge_svdrp.cpp",
    "suitebridge_epg_command_handler.cpp",
    "suitebridge_epg_type_snapshot_command.cpp",
    "suitebridge_epg_type_snapshot_contract.cpp",
    "suitebridge_epg_type_snapshot_contract.h",
    "suitebridge_recording_metadata_command.cpp",
    "suitebridge_lifecycle.cpp",
    "suitebridge_capabilities.cpp",
    "suitebridge_capability_discovery.cpp",
    "suitebridge_status_monitor.cpp",
    "suitebridge_tvscraper_adapter.cpp",
    "suitebridge_tvscraper_type_adapter.cpp",
    "suitebridge_recording_identity.cpp",
    "suitebridge_recording_metadata_contract.cpp",
    "suitebridge_tvscraper_recording_adapter.cpp",
    "tests/check_recording_metadata_contract.py",
    "tests/test_suitebridge_epg_type_snapshot_contract.cpp",
)
errors = []
for name in REQUIRED_FILES:
    if not (ROOT / name).is_file():
        errors.append(f"missing file: {name}")

if not errors:
    makefile = (ROOT / "Makefile").read_text(encoding="utf-8")
    sources = "\n".join(
        (ROOT / name).read_text(encoding="utf-8")
        for name in REQUIRED_FILES
        if name.endswith((".h", ".cpp"))
    )

    make_fragments = (
        "PLUGIN = suitebridge",
        "suitebridge_svdrp.o",
        "suitebridge_epg_command_handler.o",
        "suitebridge_epg_type_snapshot_command.o",
        "suitebridge_epg_type_snapshot_contract.o",
        "suitebridge_tvscraper_type_adapter.o",
        "suitebridge_recording_metadata_command.o",
        "suitebridge_recording_identity.o",
        "suitebridge_recording_metadata_contract.o",
        "suitebridge_tvscraper_recording_adapter.o",
        "check-recording-metadata-contract:",
        "test-epg-type-snapshot-contract:",
        "test-recording-identity:",
        "test-recording-metadata-contract:",
        'test "$(VERSION)" = "0.13.3"',
    )
    for fragment in make_fragments:
        if fragment not in makefile:
            errors.append(f"missing Makefile contract: {fragment}")

    source_fragments = (
        'inline constexpr const char *Version = "0.13.3";',
        "bool cPluginSuiteBridge::Initialize(void)",
        "bool cPluginSuiteBridge::Start(void)",
        "void cPluginSuiteBridge::Stop(void)",
        "cPluginSuiteBridge::SVDRPHelpPages(void)",
        "cPluginSuiteBridge::SVDRPCommand(",
        "statusMonitor_.CaptureSnapshot()",
        "SuiteBridgeCapabilityDiscoveryReply capabilityReply(",
        '"ARTW <channel-id> <event-id>\\n"',
        '"META <channel-id> <event-id>\\n"',
        '"ETYPES <from-epoch> <until-epoch> <offset> <limit>\\n"',
        '"RMETA <recording-key>\\n"',
        "HandleTypeSnapshot(Command, Option)",
        "LOCK_CHANNELS_READ;",
        "LOCK_SCHEDULES_READ;",
        "adapter.ResolveMediaType(*event)",
        "cGetScraperVideo request(&event, nullptr)",
        "SuiteBridgeEpgMediaTypeIsResolved(metadata.mediaType)",
        "cGetScraperVideo request(nullptr, &recording)",
        "SuiteBridgeRecordingIdentity::KeyForNativeId(nativeId)",
        "VDRPLUGINCREATOR(cPluginSuiteBridge);",
    )
    for fragment in source_fragments:
        if fragment not in sources:
            errors.append(f"missing source contract: {fragment}")

    for forbidden in (
        "#include <thread>",
        "std::thread",
        "cThread",
        "socket(",
        "bind(",
        "listen(",
        "connect(",
        "fork(",
        "execv(",
    ):
        if forbidden in sources:
            errors.append(f"forbidden foundation function: {forbidden}")

if errors:
    for error in errors:
        print(f"ERROR: {error}", file=sys.stderr)
    raise SystemExit(1)

print("suitebridge foundation contract ok")
