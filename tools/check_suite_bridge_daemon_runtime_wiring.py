#!/usr/bin/env python3

from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
CONFIG_HEADER = ROOT / "core/daemon/include/RuntimeConfig.h"
CONFIG_SOURCE = ROOT / "core/daemon/src/RuntimeConfig.cpp"
TMDB_CONFIG_SOURCE = ROOT / "core/daemon/src/TmdbSeriesArtworkRuntimeConfig.cpp"
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
TMDB_MAKE = ROOT / "mk/tmdb-series-artwork.mk"
PACKAGED_DEFAULTS = ROOT / "packaging/systemd/vdr-suite-daemon.default"
ARCHITECTURE = ROOT / "docs/architecture/suite-bridge-embedded-agent-runtime.md"
MATERIALIZATION_ARCHITECTURE = (
    ROOT / "docs/architecture/epg-series-artwork-materialization.md"
)
TMDB_ARCHITECTURE = (
    ROOT / "docs/architecture/epg-series-artwork-tmdb-provider.md"
)


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
    tmdb_config_source = TMDB_CONFIG_SOURCE.read_text(encoding="utf-8")
    context = CONTEXT.read_text(encoding="utf-8")
    runtime = read_runtime_sources()
    runtime_tests = RUNTIME_TESTS.read_text(encoding="utf-8")
    tmdb_make = TMDB_MAKE.read_text(encoding="utf-8")
    packaged_defaults = PACKAGED_DEFAULTS.read_text(encoding="utf-8")

    require(ARCHITECTURE.exists(), "SB.10d architecture contract must exist")
    require(
        MATERIALIZATION_ARCHITECTURE.exists(),
        "series artwork materialization architecture contract must exist",
    )
    require(
        TMDB_ARCHITECTURE.exists(),
        "TMDB series artwork provider architecture contract must exist",
    )
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
        "VDR_SUITE_SERIES_ARTWORK_FALLBACK_SOURCE_ROOTS",
        "VDR_SUITE_SERIES_ARTWORK_FALLBACK_CACHE_ROOT",
        "VDR_SUITE_SERIES_ARTWORK_FALLBACK_MAX_BYTES",
        "VDR_SUITE_SERIES_ARTWORK_FALLBACK_MAX_DIMENSION",
    )
    for name in required_environment:
        require(name in config_source, f"missing runtime configuration: {name}")

    required_tmdb_environment = (
        "VDR_SUITE_SERIES_ARTWORK_FALLBACK_PROVIDER",
        "VDR_SUITE_TMDB_READ_ACCESS_TOKEN",
        "VDR_SUITE_TMDB_LANGUAGE",
        "VDR_SUITE_TMDB_INCLUDE_IMAGE_LANGUAGES",
        "VDR_SUITE_TMDB_CONNECT_TIMEOUT_MS",
        "VDR_SUITE_TMDB_TOTAL_TIMEOUT_MS",
        "VDR_SUITE_TMDB_MAX_RETRIES",
        "VDR_SUITE_TMDB_RETRY_BACKOFF_MS",
        "VDR_SUITE_TMDB_NEGATIVE_CACHE_TTL_SECONDS",
        "VDR_SUITE_TMDB_TRANSIENT_CACHE_TTL_SECONDS",
        "VDR_SUITE_TMDB_MAX_JSON_BYTES",
    )
    for name in required_tmdb_environment:
        require(name in tmdb_config_source, f"missing TMDB runtime configuration: {name}")

    require(
        "/var/cache/vdr-suite/epg-artwork" in config_source,
        "fallback cache root must remain below the managed EPG artwork cache",
    )
    require(
        "isManagedSeriesArtworkCacheRoot" in config_source,
        "fallback cache root must have an explicit managed-root policy",
    )

    require(
        "SuiteBridgeEmbeddedAgentRuntime> suiteBridgeAgentRuntime" in context,
        "backend context must own backend-scoped Suite Bridge health runtime",
    )
    require(
        "SuiteBridgeEpgMetadataResolver> epgScraperMetadataDelegate" in context,
        "backend context must own its typed EPG scraper metadata delegate",
    )
    require(
        "CurlExternalArtworkHttpTransport> epgExternalArtworkHttpTransport" in context,
        "backend context must own its bounded external HTTPS transport",
    )
    require(
        "EpgSeriesArtworkProviderCacheRepository> epgSeriesArtworkProviderCacheRepository" in context,
        "backend context must own its persistent provider cache",
    )
    require(
        "TmdbSeriesArtworkProvider> epgTmdbSeriesArtworkProvider" in context,
        "backend context must own the optional TMDB provider",
    )
    require(
        "SeriesArtworkFallbackResolver> epgSeriesArtworkFallbackResolver" in context,
        "backend context must own its series artwork fallback decorator",
    )
    require(
        "FilesystemSeriesArtworkFallbackMaterializer> epgSeriesArtworkFallbackMaterializer" in context,
        "backend context must own its filesystem fallback materializer",
    )
    require(
        "SeriesArtworkFallbackMaterializingResolver> epgSeriesArtworkFallbackMaterializingResolver" in context,
        "backend context must own its fail-closed materializing decorator",
    )
    require(
        "EpgSeriesArtworkFallbackRepository> epgSeriesArtworkFallbackRepository" in context,
        "backend context must own its separate fallback repository",
    )
    require(
        "PersistentSeriesArtworkFallbackResolver> epgPersistentSeriesArtworkFallbackResolver" in context,
        "backend context must own fallback retention",
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
        "TmdbSeriesArtworkRuntimeConfig::fromEnvironment" in runtime and
        "tmdbRuntimeConfig.usable()" in runtime,
        "TMDB provider must be guarded by explicit fail-closed runtime policy",
    )
    require(
        "std::make_unique<" in runtime and
        "EpgSeriesArtworkProviderCacheRepository>" in runtime and
        "epgSeriesArtworkProviderCacheRepository->ensureSchema()" in runtime,
        "TMDB provider must fail closed when its cache schema is unavailable",
    )
    require(
        "std::make_unique<CurlExternalArtworkHttpTransport>" in runtime,
        "DaemonRuntime must construct the bounded external HTTPS transport",
    )
    require(
        "std::make_unique<TmdbSeriesArtworkProvider>" in runtime and
        "fallbackProvider =" in runtime,
        "DaemonRuntime must inject the qualified TMDB provider boundary",
    )
    require(
        "std::make_unique<SeriesArtworkFallbackResolver>" in runtime and
        "fallbackProvider" in runtime,
        "fallback resolver must receive only the guarded provider pointer",
    )
    require(
        "std::make_unique<EpgSeriesArtworkFallbackRepository>" in runtime,
        "DaemonRuntime must construct a separate fallback repository",
    )
    require(
        "epgSeriesArtworkFallbackRepository->ensureSchema()" in runtime,
        "fallback materialization must fail closed when its schema is unavailable",
    )
    require(
        "FilesystemSeriesArtworkFallbackMaterializerConfig" in runtime and
        "std::make_unique<" in runtime and
        "FilesystemSeriesArtworkFallbackMaterializer>" in runtime,
        "DaemonRuntime must construct the guarded filesystem materializer",
    )
    require(
        "runtimeFallbackConfig.sourceRoots" in runtime and
        "runtimeFallbackConfig.cacheRoot" in runtime and
        "runtimeFallbackConfig.maximumSourceBytes" in runtime and
        "runtimeFallbackConfig.maximumDimension" in runtime,
        "materializer must receive all bounded runtime policy values",
    )
    require(
        "std::make_unique<SeriesArtworkFallbackMaterializingResolver>" in runtime,
        "DaemonRuntime must construct the fail-closed materializing decorator",
    )
    require(
        "std::make_unique<PersistentSeriesArtworkFallbackResolver>" in runtime,
        "DaemonRuntime must retain only materialized fallback artwork",
    )
    require(
        "config_.seriesArtworkFallback()" in runtime and
        "runtimeFallbackConfig.enabled" in runtime,
        "fallback runtime must use the explicit disabled-by-default switch",
    )
    require(
        "*context->epgSeriesArtworkFallbackMaterializingResolver" in runtime or
        "persistentDelegate" in runtime,
        "persistent fallback retention must wrap the materializing decorator",
    )
    require(
        "std::make_unique<PersistentEpgScraperMetadataResolver>" in runtime,
        "DaemonRuntime must wrap EPG scraper metadata with primary persistence",
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

    metadata_transport_index = runtime.index("context->suiteBridgeTransport =")
    metadata_delegate_index = runtime.index("context->epgScraperMetadataDelegate =")
    provider_cache_index = runtime.index(
        "context->epgSeriesArtworkProviderCacheRepository ="
    )
    external_transport_index = runtime.index(
        "context->epgExternalArtworkHttpTransport ="
    )
    tmdb_provider_index = runtime.index("context->epgTmdbSeriesArtworkProvider =")
    fallback_resolver_index = runtime.index(
        "context->epgSeriesArtworkFallbackResolver ="
    )
    fallback_repository_index = runtime.index(
        "context->epgSeriesArtworkFallbackRepository ="
    )
    fallback_materializer_index = runtime.index(
        "context->epgSeriesArtworkFallbackMaterializer ="
    )
    materializing_resolver_index = runtime.index(
        "context->epgSeriesArtworkFallbackMaterializingResolver ="
    )
    persistent_fallback_index = runtime.index(
        "context->epgPersistentSeriesArtworkFallbackResolver ="
    )
    metadata_resolver_index = runtime.index(
        "context->epgScraperMetadataResolver ="
    )
    require(
        metadata_transport_index < metadata_delegate_index <
        provider_cache_index < external_transport_index < tmdb_provider_index <
        fallback_resolver_index < fallback_repository_index <
        fallback_materializer_index < materializing_resolver_index <
        persistent_fallback_index < metadata_resolver_index,
        "runtime order must be Bridge metadata, provider cache, external "
        "transport, qualified provider, candidate fallback, materializer, "
        "fallback retention, then primary persistence",
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
        "pkg-config --libs libcurl" in tmdb_make and
        "CurlExternalArtworkHttpTransport.cpp" in tmdb_make,
        "TMDB runtime must link its explicit libcurl transport",
    )
    require(
        "epg-artwork/incoming" in tmdb_make and
        "epg-artwork/external" in tmdb_make,
        "install staging must create both artwork trust-boundary roots",
    )
    require(
        "VDR_SUITE_SERIES_ARTWORK_FALLBACK_ENABLED=false" in packaged_defaults and
        "VDR_SUITE_SERIES_ARTWORK_FALLBACK_PROVIDER=none" in packaged_defaults,
        "packaged TMDB runtime must remain offline by default",
    )
    require(
        "# VDR_SUITE_TMDB_READ_ACCESS_TOKEN=" in packaged_defaults and
        "\nVDR_SUITE_TMDB_READ_ACCESS_TOKEN=" not in packaged_defaults,
        "packaged defaults must not define or ship a TMDB token",
    )
    require(
        "RestfulApiVdrAdapter" in runtime,
        "SB.10d must preserve the existing RESTfulAPI domain adapter",
    )
    require(
        "ApiRouter(" not in config_source and "ApiRouter(" not in tmdb_config_source,
        "fallback configuration must not create a public route",
    )
    require(
        "readAccessToken" not in "\n".join(
            line for line in runtime.splitlines() if "std::cerr" in line
        ),
        "TMDB token must not be written to runtime error logs",
    )

    print("check_suite_bridge_daemon_runtime_wiring passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
