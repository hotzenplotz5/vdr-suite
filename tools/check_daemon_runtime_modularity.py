#!/usr/bin/env python3

from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
SOURCE_ROOT = ROOT / "core/daemon/src"
DAEMON_SOURCES = ROOT / "mk/daemon-sources.mk"

SOURCE_OWNERS = {
    "DaemonRuntime.cpp": (
        "DaemonRuntime::DaemonRuntime()",
        "DaemonRuntime::run()",
        "DaemonRuntime::shutdown()",
    ),
    "DaemonRuntimeSignal.cpp": (
        "DaemonRuntime::handleSignal(",
    ),
    "DaemonRuntimeInitialization.cpp": (
        "DaemonRuntime::initialize()",
    ),
    "DaemonRuntimeBackendContext.cpp": (
        "DaemonRuntime::createBackendRuntimeContext(",
    ),
    "DaemonRuntimePolling.cpp": (
        "DaemonRuntime::pollVdrAndUpdateChangeFeed()",
    ),
    "DaemonRuntimeEpgCache.cpp": (
        "DaemonRuntime::startEpgCacheWarmupWorker()",
        "DaemonRuntime::stopEpgCacheWarmupWorker()",
        "DaemonRuntime::runEpgCacheWarmupWorker()",
        "DaemonRuntime::refreshEpgCacheForAllBackends(",
    ),
    "DaemonRuntimeRecordingCache.cpp": (
        "DaemonRuntime::startRecordingCacheWarmupWorker()",
        "DaemonRuntime::stopRecordingCacheWarmupWorker()",
        "DaemonRuntime::runRecordingCacheWarmupWorker()",
        "DaemonRuntime::refreshRecordingCacheForAllBackends(",
    ),
}


def require(condition: bool, message: str) -> None:
    if not condition:
        print(f"ERROR: {message}", file=sys.stderr)
        raise SystemExit(1)


def main() -> int:
    sources = {}

    for filename in SOURCE_OWNERS:
        path = SOURCE_ROOT / filename
        require(path.exists(), f"missing daemon runtime source: {filename}")
        sources[filename] = path.read_text(encoding="utf-8")

    combined = "\n".join(sources.values())

    for owner, tokens in SOURCE_OWNERS.items():
        for token in tokens:
            require(
                combined.count(token) == 1,
                f"daemon runtime method must have exactly one definition: {token}",
            )
            require(
                token in sources[owner],
                f"daemon runtime method has the wrong source owner: {token}",
            )

    lifecycle = sources["DaemonRuntime.cpp"]
    require(
        len(lifecycle.splitlines()) <= 240,
        "DaemonRuntime.cpp must remain a focused lifecycle source",
    )
    require(
        "DaemonRuntime::initialize()" not in lifecycle,
        "DaemonRuntime.cpp must not absorb initialization again",
    )
    require(
        "runRecordingMetadataEnrichment(" not in lifecycle,
        "DaemonRuntime.cpp must not absorb recording metadata worker logic again",
    )

    daemon_sources = DAEMON_SOURCES.read_text(encoding="utf-8")
    for filename in SOURCE_OWNERS:
        source_path = f"core/daemon/src/{filename}"
        require(
            daemon_sources.count(source_path) == 1,
            f"daemon source inventory must contain exactly one {source_path}",
        )

    print("check_daemon_runtime_modularity passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
