#!/usr/bin/env python3

from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
CONFIG_HEADER = ROOT / "core/daemon/include/RuntimeConfig.h"
CONFIG_SOURCE = ROOT / "core/daemon/src/RuntimeConfig.cpp"
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
RUNTIME_TESTS = ROOT / "mk/runtime-api-tests.mk"
ARCHITECTURE = ROOT / "docs/architecture/suite-bridge-embedded-agent-runtime.md"


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
    config_header = CONFIG_HEADER.read_text(encoding="utf-8")
    config_source = CONFIG_SOURCE.read_text(encoding="utf-8")
    context = CONTEXT.read_text(encoding="utf-8")
    runtime = read_runtime_sources()
    runtime_tests = RUNTIME_TESTS.read_text(encoding="utf-8")

    require(ARCHITECTURE.exists(), "SB.10d architecture contract must exist")
    require(
        "RuntimeSuiteBridgeConfig" in config_header,
        "RuntimeConfig must expose Suite Bridge configuration",
    )
    require(
        "RuntimeSeriesArtworkFallbackConfig" in config_header,
        "RuntimeConfig must expose series artwork fallback configuration",
    )

    required_environment = (
        "VDR_SUITE_SUITE_BRIDGE_ENABLED",
        "VDR_SUITE_SUITE_BRIDGE_BACKEND_ID",
        "VDR_SUITE_SUITE_BRIDGE_HOST",
        "VDR_SUITE_SUITE_BRIDGE_PORT",
        "VDR_SUITE_SUITE_BRIDGE_CONNECT_TIMEOUT_MS",
        "VDR_SUITE_SUITE_BRIDGE_IO_TIMEOUT_MS",
        "VDR_SUITE_SUITE_BRIDGE_OPERATION_TIMEOUT_MS",
        "VDR_SUITE_SUITE_BRIDGE_POLL_INTERVAL_MS",
        "VDR_SUITE_SUITE_BRIDGE_STALE_AFTER_MS",
        "VDR_SUITE_SUITE_BRIDGE_OFFLINE_AFTER_MS",
        "VDR_SUITE_SUITE_BRIDGE_RECONNECT_INITIAL_MS",
        "VDR_SUITE_SUITE_BRIDGE_RECONNECT_MAXIMUM_MS",
        "VDR_SUITE_SERIES_ARTWORK_FALLBACK_ENABLED",
    )
    for name in required_environment:
        require(name in config_source, f"missing runtime configuration: {name}")

    require(
        "SuiteBridgeEmbeddedAgentRuntime> suiteBridgeAgentRuntime" in context,
        "backend context must own backend-scoped Suite Bridge health runtime",
    )
    require(
        "SuiteBridgeEpgMetadataResolver> epgScraperMetadataDelegate" in context,
        "backend context must own its typed EPG scraper metadata delegate",
    )
    require(
        "SeriesArtworkFallbackResolver> epgSeriesArtworkFallbackResolver" in context,
        "backend context must own its series artwork fallback decorator",
    )
    require(
        "PersistentEpgScraperMetadataResolver> epgScraperMetadataResolver" in context,
        "backend context must own its persistent EPG scraper metadata resolver",
    )
    require(
        "context->backendId == suiteBridgeConfig.backendId" in runtime,
        "Suite Bridge runtime must attach only to the configured backend",
    )
    require(
        "std::make_unique<vdrsuite::agent::SuiteBridgeEmbeddedAgentRuntime>" in runtime,
        "DaemonRuntime must construct the embedded Agent runtime",
    )
    require(
        "std::make_unique<SuiteBridgeEpgMetadataResolver>" in runtime,
        "DaemonRuntime must construct the backend EPG scraper metadata delegate",
    )
    require(
        "std::make_unique<SeriesArtworkFallbackResolver>" in runtime,
        "DaemonRuntime must construct the series artwork fallback decorator",
    )
    require(
        "config_.seriesArtworkFallback().enabled" in runtime,
        "fallback decorator must use the explicit disabled-by-default runtime switch",
    )
    require(
        "*context->epgSeriesArtworkFallbackResolver" in runtime,
        "persistent metadata resolver must wrap the fallback decorator",
    )
    require(
        "std::make_unique<PersistentEpgScraperMetadataResolver>" in runtime,
        "DaemonRuntime must wrap EPG scraper metadata with persistent artwork retention",
    )
    require(
        "registerScraperMetadataResolver(" in runtime,
        "DaemonRuntime must register EPG scraper metadata by backend",
    )
    require(
        "suiteBridgeAgentRuntime->start()" in runtime,
        "DaemonRuntime must start the embedded Agent runtime",
    )
    require(
        "suiteBridgeAgentRuntime->stop()" in runtime,
        "DaemonRuntime must stop the embedded Agent runtime",
    )

    metadata_transport_index = runtime.index(
        "context->suiteBridgeTransport ="
    )
    metadata_delegate_index = runtime.index(
        "context->epgScraperMetadataDelegate ="
    )
    fallback_resolver_index = runtime.index(
        "context->epgSeriesArtworkFallbackResolver ="
    )
    metadata_resolver_index = runtime.index(
        "context->epgScraperMetadataResolver ="
    )
    require(
        metadata_transport_index < metadata_delegate_index <
        fallback_resolver_index < metadata_resolver_index,
        "runtime order must be transport, Bridge metadata, fallback decorator, persistence",
    )

    start_index = runtime.index("suiteBridgeAgentRuntime->start()")
    event_start_index = runtime.index("eventStreamClient->start()", start_index)
    require(
        start_index < event_start_index,
        "Suite Bridge worker must start before the RESTfulAPI event-stream client",
    )

    event_stop_index = runtime.index("eventStreamClient->stop()")
    suite_stop_index = runtime.index("suiteBridgeAgentRuntime->stop()", event_stop_index)
    require(
        event_stop_index < suite_stop_index,
        "shutdown must reverse active local reader start order",
    )

    require(
        "$(AGENT_SRC)" in runtime_tests,
        "daemon and backend-context targets must link the embedded Agent sources",
    )
    require(
        "RestfulApiVdrAdapter" in runtime,
        "SB.10d must preserve the existing RESTfulAPI domain adapter",
    )
    require(
        "ApiRouter(" not in config_source,
        "Suite Bridge configuration must not create a public route",
    )

    print("check_suite_bridge_daemon_runtime_wiring passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
