#!/usr/bin/env python3

from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]

required_files = (
    ROOT / "Makefile",
    ROOT / "README.md",
    ROOT / "suitebridge.h",
    ROOT / "suitebridge.cpp",
    ROOT / "suitebridge_lifecycle.h",
    ROOT / "suitebridge_lifecycle.cpp",
    ROOT / "suitebridge_capabilities.h",
    ROOT / "suitebridge_capabilities.cpp",
    ROOT / "suitebridge_capability_discovery.h",
    ROOT / "suitebridge_capability_discovery.cpp",
    ROOT / "suitebridge_counter_continuity.h",
    ROOT / "suitebridge_counter_continuity.cpp",
    ROOT / "suitebridge_status_snapshot.h",
    ROOT / "suitebridge_status_snapshot.cpp",
    ROOT / "suitebridge_local_contract.h",
    ROOT / "suitebridge_local_contract.cpp",
    ROOT / "suitebridge_svdrp_contract.h",
    ROOT / "suitebridge_svdrp_contract.cpp",
    ROOT / "suitebridge_status_events.h",
    ROOT / "suitebridge_status_events.cpp",
    ROOT / "suitebridge_status_monitor.h",
    ROOT / "suitebridge_status_monitor.cpp",
    ROOT / "suitebridge_artwork_reference.h",
    ROOT / "suitebridge_artwork_reference.cpp",
    ROOT / "suitebridge_image_dimensions.h",
    ROOT / "suitebridge_image_dimensions.cpp",
    ROOT / "suitebridge_tvscraper_adapter.h",
    ROOT / "suitebridge_tvscraper_adapter.cpp",
    ROOT / "suitebridge_epg_artwork_contract.h",
    ROOT / "suitebridge_epg_artwork_contract.cpp",
    ROOT / "docs/SB-2-capabilities.md",
    ROOT / "docs/SB-3-status-events.md",
    ROOT / "docs/SB-4-status-snapshots.md",
    ROOT / "docs/SB-5-local-contract-payload.md",
    ROOT / "docs/SB-6-read-only-svdrp.md",
    ROOT / "docs/SB-7-lifecycle-callback-hardening.md",
    ROOT / "docs/SB-8-counter-continuity.md",
    ROOT / "docs/SB-9-capability-discovery.md",
    ROOT / "tests/check_capabilities_contract.py",
    ROOT / "tests/check_capability_discovery_contract.py",
    ROOT / "tests/check_counter_continuity_contract.py",
    ROOT / "tests/check_status_events_contract.py",
    ROOT / "tests/check_status_snapshot_contract.py",
    ROOT / "tests/check_local_contract_contract.py",
    ROOT / "tests/check_svdrp_contract.py",
    ROOT / "tests/test_suitebridge_lifecycle.cpp",
    ROOT / "tests/test_suitebridge_capabilities.cpp",
    ROOT / "tests/test_suitebridge_capability_discovery.cpp",
    ROOT / "tests/test_suitebridge_counter_continuity.cpp",
    ROOT / "tests/test_suitebridge_status_events.cpp",
    ROOT / "tests/test_suitebridge_status_snapshot.cpp",
    ROOT / "tests/test_suitebridge_local_contract.cpp",
    ROOT / "tests/test_suitebridge_svdrp_contract.cpp",
    ROOT / "tests/test_suitebridge_artwork_reference.cpp",
    ROOT / "tests/test_suitebridge_image_dimensions.cpp",
    ROOT / "tests/test_suitebridge_epg_artwork_contract.cpp",
)

errors = []

for path in required_files:
    if not path.is_file():
        errors.append(f"missing file: {path.relative_to(ROOT)}")

if errors:
    for error in errors:
        print(f"ERROR: {error}", file=sys.stderr)
    raise SystemExit(1)

makefile = (ROOT / "Makefile").read_text(encoding="utf-8")
source = (ROOT / "suitebridge.cpp").read_text(encoding="utf-8")

combined = "\n".join(
    path.read_text(encoding="utf-8")
    for path in required_files
    if path.suffix in {".h", ".cpp"}
)

required_makefile_content = (
    "PLUGIN = suitebridge",
    "SOFILE = libvdr-$(PLUGIN).so",
    "APIVERSION = $(call PKGCFG,apiversion)",
    "suitebridge_capability_discovery.o",
    "suitebridge_counter_continuity.o",
    "suitebridge_status_snapshot.o",
    "suitebridge_local_contract.o",
    "suitebridge_svdrp_contract.o",
    "suitebridge_status_events.o",
    "suitebridge_status_monitor.o",
    "suitebridge_artwork_reference.o",
    "suitebridge_image_dimensions.o",
    "suitebridge_tvscraper_adapter.o",
    "suitebridge_epg_artwork_contract.o",
    "check-capabilities-contract:",
    "check-capability-discovery-contract:",
    "check-counter-continuity-contract:",
    "check-status-events-contract:",
    "check-status-snapshot-contract:",
    "check-local-contract-contract:",
    "check-svdrp-contract:",
    "test-lifecycle:",
    "test-capabilities:",
    "test-capability-discovery:",
    "test-counter-continuity:",
    "test-status-events:",
    "test-status-snapshot:",
    "test-local-contract:",
    "test-svdrp-contract:",
    "test-artwork-reference:",
    "test-image-dimensions:",
    "test-epg-artwork-contract:",
    'test "$(VERSION)" = "0.10.0"',
)

for fragment in required_makefile_content:
    if fragment not in makefile:
        errors.append(f"missing Makefile contract: {fragment}")

required_source_content = (
    'static const char *PLUGIN_NAME = "suitebridge";',
    'static const char *VERSION = "0.10.0";',
    "bool cPluginSuiteBridge::Initialize(void)",
    "bool cPluginSuiteBridge::Start(void)",
    "void cPluginSuiteBridge::Stop(void)",
    "cPluginSuiteBridge::SVDRPHelpPages(void)",
    "cPluginSuiteBridge::SVDRPCommand(",
    "lifecycle_.Initialize()",
    "lifecycle_.Start()",
    "lifecycle_.BeginStop()",
    "lifecycle_.CompleteStop()",
    "statusMonitor_.Activate()",
    "statusMonitor_.Deactivate()",
    "statusMonitor_.CaptureSnapshot()",
    "SuiteBridgeCapabilities::All()",
    "SuiteBridgeCapabilities::SchemaVersion()",
    "SuiteBridgeCapabilities::StateName(capability.state)",
    "SuiteBridgeCapabilityDiscoveryReply capabilityReply(",
    '"ARTW <channel-id> <event-id>\\n"',
    "SuiteBridgeEpgArtworkRequest artworkRequest(",
    "ResolvePreferredArtwork(event)",
    "return nullptr;",
    "VDRPLUGINCREATOR(cPluginSuiteBridge);",
)

for fragment in required_source_content:
    if fragment not in source:
        errors.append(f"missing source contract: {fragment}")

required_foundation_content = (
    "enum class SuiteBridgeLifecycleState",
    "SuiteBridgeLifecycleState::Stopping",
    "bool SuiteBridgeLifecycle::Initialize() noexcept",
    "bool SuiteBridgeLifecycle::Start() noexcept",
    "bool SuiteBridgeLifecycle::BeginStop() noexcept",
    "bool SuiteBridgeLifecycle::CompleteStop() noexcept",
    "class SuiteBridgeCapabilityDiscoveryPayload final",
    "class SuiteBridgeCapabilityDiscoveryReply final",
    "class SuiteBridgeCounterEpoch final",
    "class SuiteBridgeSaturatingCounter final",
    "class SuiteBridgeStatusSnapshot final",
    "CounterEpoch() const noexcept",
    "CounterOverflow() const noexcept",
    "CaptureSnapshot(bool monitorActive) const noexcept",
    "SuiteBridgeStatusMonitor::CaptureSnapshot() const noexcept",
    "status-snapshot schema=%u active=%s total=%llu",
    "class SuiteBridgeLocalContractPayload final",
    "local-contract-payload schema=%u result=prepared",
    "class SuiteBridgeSvdrpReply final",
    "SuiteBridgeSvdrpReply::Handled() const noexcept",
    "SuiteBridgeSvdrpReply::HasPayload() const noexcept",
    "class SuiteBridgeEpgArtworkRequest final",
    "class SuiteBridgeEpgArtworkPayload final",
    "SuiteBridgeReadImageDimensions(",
    "cGetScraperVideo request(&event, nullptr)",
    "request.call()",
)

for fragment in required_foundation_content:
    if fragment not in combined:
        errors.append(f"missing Suite bridge foundation: {fragment}")

forbidden_content = (
    "#include <thread>",
    "std::thread",
    "cThread",
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
        errors.append(f"forbidden foundation functionality: {fragment}")

if errors:
    for error in errors:
        print(f"ERROR: {error}", file=sys.stderr)
    raise SystemExit(1)

print("suitebridge foundation contract ok")
