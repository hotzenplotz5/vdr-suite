#!/usr/bin/env python3
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
CONTEXT = ROOT / "core/daemon/include/BackendRuntimeContext.h"
RUNTIME_SOURCE_ROOT = ROOT / "core/daemon/src"
RUNTIME_SOURCES = (
    "DaemonRuntimeBackendContext.cpp",
    "DaemonRuntimeInitialization.cpp",
    "DaemonRuntimePolling.cpp",
    "DaemonRuntimeEpgCache.cpp",
    "DaemonRuntimeRecordingCache.cpp",
    "DaemonRuntime.cpp",
)
BACKEND_RUNTIME = RUNTIME_SOURCE_ROOT / "DaemonRuntimeBackendContext.cpp"
INITIALIZATION_RUNTIME = RUNTIME_SOURCE_ROOT / "DaemonRuntimeInitialization.cpp"
RECORDING_RUNTIME = RUNTIME_SOURCE_ROOT / "DaemonRuntimeRecordingCache.cpp"
TEST_MAKE = ROOT / "mk/recording-native-metadata-tests.mk"
AGENT_SOURCES = ROOT / "mk/agent-sources.mk"
AGENT_TESTS = ROOT / "mk/agent-tests.mk"
RUNTIME_TESTS = ROOT / "mk/runtime-api-tests.mk"
DAEMON_HEADER = ROOT / "core/daemon/include/DaemonRuntime.h"
RECORDING_PERSON_CONTROLLER = (
    ROOT / "api/rest/src/RecordingPersonSearchController.cpp"
)
API_ROUTER = ROOT / "api/rest/src/ApiRouter.cpp"


def require(condition: bool, message: str) -> None:
    if not condition:
        print(f"ERROR: {message}", file=sys.stderr)
        raise SystemExit(1)


def read_runtime_sources() -> str:
    return "\n".join(
        (RUNTIME_SOURCE_ROOT / filename).read_text(encoding="utf-8")
        for filename in RUNTIME_SOURCES
    )


def main() -> int:
    context = CONTEXT.read_text(encoding="utf-8")
    runtime = read_runtime_sources()
    backend_runtime = BACKEND_RUNTIME.read_text(encoding="utf-8")
    initialization_runtime = INITIALIZATION_RUNTIME.read_text(encoding="utf-8")
    recording_runtime = RECORDING_RUNTIME.read_text(encoding="utf-8")
    test_make = TEST_MAKE.read_text(encoding="utf-8")
    agent_sources = AGENT_SOURCES.read_text(encoding="utf-8")
    agent_tests = AGENT_TESTS.read_text(encoding="utf-8")
    runtime_tests = RUNTIME_TESTS.read_text(encoding="utf-8")
    daemon_header = DAEMON_HEADER.read_text(encoding="utf-8")
    recording_person_controller = (
        RECORDING_PERSON_CONTROLLER.read_text(encoding="utf-8")
    )
    api_router = API_ROUTER.read_text(encoding="utf-8")

    require(
        "VDR_RECORDING_NATIVE_METADATA_SRC :=" in test_make,
        "native recording metadata module source group must be explicit",
    )
    require(
        "DAEMON_SRC += $(VDR_RECORDING_NATIVE_METADATA_SRC)" in test_make,
        "daemon must link the complete native recording metadata module",
    )
    require(
        "core/vdr/src/VdrRecordingNativePersonSearchService.cpp"
        in test_make,
        "native recording person search service must be linked",
    )
    require(
        "test-vdr-recording-native-person-search-service"
        in test_make,
        "native recording person search must have a focused test target",
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

    transport_index = backend_runtime.index("context->suiteBridgeTransport =")
    epg_resolver_index = backend_runtime.index("context->epgArtworkResolver =")
    recording_resolver_index = backend_runtime.index("context->recordingMetadataResolver =")
    require(
        transport_index < epg_resolver_index and transport_index < recording_resolver_index,
        "EPG and recording metadata must share the already bounded SuiteBridge transport",
    )

    require(
        "capabilityAvailable(" in recording_runtime
        and "recording-metadata" in recording_runtime,
        "RMETA processing must be gated by SuiteBridge discovery capability",
    )
    require(
        "reconcileInventory(" in recording_runtime
        and "recordings," in recording_runtime,
        "recording refresh must reconcile the persistent native metadata inventory",
    )
    require(
        "processBatch(now)" in recording_runtime,
        "recording refresh must process only the bounded enrichment batch",
    )
    require(
        "findAllForBackend(" in recording_runtime
        and "backendRuntimeContext->backendId" in recording_runtime,
        "periodic metadata retry must read the persistent recording cache",
    )
    require(
        "metadataRefreshSeconds = 60" in recording_runtime,
        "recording metadata retry cadence must remain explicit and bounded",
    )
    require(
        "VdrRecordingNativePersonSearchService" in initialization_runtime,
        "daemon initialization must wire the persistent recording person search service",
    )
    require(
        "native-persistent-index" in initialization_runtime,
        "daemon initialization must report the persistent recording person search source",
    )
    require(
        "persistentSearch_" in recording_person_controller,
        "recording person controller must prefer persistent search when wired",
    )
    require(
        "usesPersistentSearch()" in api_router
        and "fallbackRecordings" in api_router,
        "router must avoid snapshot loading when persistent search is wired",
    )

    refresh_finished_index = recording_runtime.index(
        "vdrRecordingCacheRepository_->markRefreshFinished("
    )
    enrichment_index = recording_runtime.index(
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
    require(
        "runRecordingMetadataEnrichment(" in runtime,
        "modular daemon source aggregation must retain recording enrichment wiring",
    )

    print("check_recording_native_metadata_runtime_wiring passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
