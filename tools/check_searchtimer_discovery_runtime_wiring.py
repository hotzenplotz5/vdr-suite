#!/usr/bin/env python3

from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
RUNTIME_SOURCE_ROOT = ROOT / "core" / "daemon" / "src"
RUNTIME_SOURCES = (
    "DaemonRuntimeBackendContext.cpp",
    "DaemonRuntimeInitialization.cpp",
    "DaemonRuntimePolling.cpp",
    "DaemonRuntimeEpgCache.cpp",
    "DaemonRuntimeRecordingCache.cpp",
    "DaemonRuntime.cpp",
)
RUNTIME_HEADER = ROOT / "core" / "daemon" / "include" / "DaemonRuntime.h"
DAEMON_SOURCES = ROOT / "mk" / "daemon-sources.mk"
VDR_SOURCES = ROOT / "mk" / "vdr-sources.mk"
DISCOVERY_PROVIDER_SOURCE = "core/vdr/src/RestfulApiSearchTimerDiscoveryProvider.cpp"


def require(condition: bool, message: str) -> None:
    if not condition:
        print(f"ERROR: {message}", file=sys.stderr)
        sys.exit(1)


def read_runtime_sources() -> str:
    return "\n".join(
        (RUNTIME_SOURCE_ROOT / filename).read_text(encoding="utf-8")
        for filename in RUNTIME_SOURCES
    )


def main() -> int:
    runtime_cpp = read_runtime_sources()
    runtime_header = RUNTIME_HEADER.read_text(encoding="utf-8")
    daemon_sources = DAEMON_SOURCES.read_text(encoding="utf-8")
    vdr_sources = VDR_SOURCES.read_text(encoding="utf-8")

    require(
        "#include \"RestfulApiSearchTimerDiscoveryProvider.h\"" in runtime_cpp,
        "daemon runtime sources must include RestfulApiSearchTimerDiscoveryProvider.h",
    )
    require(
        "std::unique_ptr<ISearchTimerDiscoveryProvider> searchTimerDiscoveryProvider_;" in runtime_header,
        "DaemonRuntime must own discovery through ISearchTimerDiscoveryProvider",
    )
    require(
        "std::make_unique<RestfulApiSearchTimerDiscoveryProvider>" in runtime_cpp,
        "DaemonRuntime must construct the HTTP-backed RESTfulAPI discovery provider",
    )
    require(
        "*backendRuntimeContexts_.front()->httpClient" in runtime_cpp,
        "RESTfulAPI discovery provider must use the backend HTTP client",
    )
    require(
        "std::make_unique<SearchTimerDiscoveryStaticProvider>" in runtime_cpp,
        "DaemonRuntime should retain static discovery fallback for no-backend cases",
    )
    require(
        DISCOVERY_PROVIDER_SOURCE in vdr_sources,
        "VDR source list must compile RestfulApiSearchTimerDiscoveryProvider.cpp",
    )
    require(
        DISCOVERY_PROVIDER_SOURCE not in daemon_sources,
        "daemon source list must not duplicate RestfulApiSearchTimerDiscoveryProvider.cpp",
    )

    print("check_searchtimer_discovery_runtime_wiring passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
