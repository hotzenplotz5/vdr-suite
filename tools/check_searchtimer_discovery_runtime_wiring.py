#!/usr/bin/env python3

from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
RUNTIME_CPP = ROOT / "core" / "daemon" / "src" / "DaemonRuntime.cpp"
RUNTIME_HEADER = ROOT / "core" / "daemon" / "include" / "DaemonRuntime.h"
DAEMON_SOURCES = ROOT / "mk" / "daemon-sources.mk"


def require(condition: bool, message: str) -> None:
    if not condition:
        print(f"ERROR: {message}", file=sys.stderr)
        sys.exit(1)


def main() -> int:
    runtime_cpp = RUNTIME_CPP.read_text(encoding="utf-8")
    runtime_header = RUNTIME_HEADER.read_text(encoding="utf-8")
    daemon_sources = DAEMON_SOURCES.read_text(encoding="utf-8")

    require(
        "#include \"RestfulApiSearchTimerDiscoveryProvider.h\"" in runtime_cpp,
        "DaemonRuntime.cpp must include RestfulApiSearchTimerDiscoveryProvider.h",
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
        "core/vdr/src/RestfulApiSearchTimerDiscoveryProvider.cpp" in daemon_sources,
        "daemon source list must compile RestfulApiSearchTimerDiscoveryProvider.cpp",
    )

    print("check_searchtimer_discovery_runtime_wiring passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
