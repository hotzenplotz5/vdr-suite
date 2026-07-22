#!/usr/bin/env python3
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
CONTEXT = ROOT / "core/daemon/include/BackendRuntimeContext.h"
RUNTIME = ROOT / "core/daemon/src/DaemonRuntime.cpp"
TEST_MAKE = ROOT / "mk/recording-native-metadata-tests.mk"
AGENT_SOURCES = ROOT / "mk/agent-sources.mk"
AGENT_TESTS = ROOT / "mk/agent-tests.mk"
RUNTIME_TESTS = ROOT / "mk/runtime-api-tests.mk"
DAEMON_HEADER = ROOT / "core/daemon/include/DaemonRuntime.h"


def require(condition: bool, message: str) -> None:
    if not condition:
        print(f"ERROR: {message}", file=sys.stderr)
        raise SystemExit(1)


def main() -> int:
    context = CONTEXT.read_text(encoding="utf-8")
    runtime = RUNTIME.read_text(encoding="utf-8")
    test_make = TEST_MAKE.read_text(encoding="utf-8")
    agent_sources = AGENT_SOURCES.read_text(encoding="utf-8")
    agent_tests = AGENT_TESTS.read_text(encoding="utf-8")
    runtime_tests = RUNTIME_TESTS.read_text(encoding="utf-8")
    daemon_header = DAEMON_HEADER.read_text(encoding="utf-8")

    require(
        "VDR_RECORDING_NATIVE_METADATA_SRC :=" in test_make,
        "native recording metadata module source group must be explicit",
    )
    require(
        "DAEMON_SRC += $(VDR_RECORDING_NATIVE_METADATA_SRC)" in test_make,
        "daemon must link the complete native recording metadata module",
    )
    require(
        "AGENT_SVDRP_TRANSPORT_STANDALONE_SRC =" in agent_sources,
        "standalone SuiteBridge transport linkage must own its identity dependency",
    )
    require(
        "AGENT_STANDALONE_SRC =" in agent_sources,
        "standalone embedded agent linkage must own its identity dependency",
    )
    require(
        "$(AGENT_SVDRP_TRANSPORT_STANDALONE_SRC)" in agent_tests,
        "standalone SuiteBridge transport tests must use the complete link group",
    )
    require(
        "$(AGENT_STANDALONE_SRC)" in agent_tests,
        "standalone embedded agent tests must use the complete link group",
    )
    require(
        "$(VDR_RECORDING_NATIVE_METADATA_SRC)" in runtime_tests,
        "BackendRuntimeContext test must link the module owned by the context",
    )

    require(
        "SuiteBridgeSvdrpTransport> suiteBridgeTransport" in context,
        "backend context must own the shared SuiteBridge transport",
    )
    require(
        "VdrRecordingNativeMetadataRepository> recordingMetadataRepository" in context,
        "backend context must own the native recording metadata repository",
    )
    require(
        "SuiteBridgeRecordingMetadataResolver> recordingMetadataResolver" in context,
        "backend context must own the typed recording metadata resolver",
    )
    require(
        "VdrRecordingNativeMetadataEnrichmentService> recordingMetadataEnrichmentService" in context,
        "backend context must own the bounded recording metadata enrichment service",
    )

    transport_index = runtime.index("context->suiteBridgeTransport =")
    epg_resolver_index = runtime.index("context->epgArtworkResolver =")
    recording_resolver_index = runtime.index("context->recordingMetadataResolver =")
    require(
        transport_index < epg_resolver_index and transport_index < recording_resolver_index,
        "EPG and recording metadata must share the already bounded SuiteBridge transport",
    )

    require(
        "capabilityAvailable(" in runtime and "recording-metadata" in runtime,
        "RMETA processing must be gated by SuiteBridge discovery capability",
    )
    require(
        "reconcileInventory(" in runtime and "recordings," in runtime,
        "recording refresh must reconcile the persistent native metadata inventory",
    )
    require(
        "processBatch(now)" in runtime,
        "recording refresh must process only the bounded enrichment batch",
    )
    require(
        "findAllForBackend(" in runtime and "backendRuntimeContext->backendId" in runtime,
        "periodic metadata retry must read the persistent recording cache",
    )
    require(
        "metadataRefreshSeconds = 60" in runtime,
        "recording metadata retry cadence must remain explicit and bounded",
    )

    refresh_finished_index = runtime.index(
        "vdrRecordingCacheRepository_->markRefreshFinished("
    )
    enrichment_index = runtime.index(
        "runRecordingMetadataEnrichment(", refresh_finished_index
    )
    require(
        refresh_finished_index < enrichment_index,
        "native metadata enrichment must follow a successful recording cache replace",
    )

    require(
        "recordingMetadataThread_" not in daemon_header,
        "recording metadata must not introduce a second daemon worker thread",
    )
    require(
        daemon_header.count("std::thread recordingCacheWarmupThread_;") == 1,
        "the existing recording-cache worker must remain the single owner",
    )
    require(
        "check-vdr-recording-native-metadata-runtime-wiring" in test_make,
        "recording metadata contract target must include the runtime wiring check",
    )

    print("check_recording_native_metadata_runtime_wiring passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
