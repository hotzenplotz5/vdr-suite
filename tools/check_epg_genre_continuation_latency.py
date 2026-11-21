#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
WORKER = (ROOT / "core/daemon/src/DaemonRuntimeEpgCache.cpp").read_text(
    encoding="utf-8"
)


def require(condition: bool, message: str) -> None:
    if not condition:
        raise SystemExit(message)


require(
    "constexpr int PeriodicEpgContinuationSeconds = 1;" in WORKER,
    "EPG genre continuation must run at one-second scheduling granularity",
)
require(
    "constexpr int PeriodicEpgTypeSnapshotPages = 1;" in WORKER,
    "ETYPES continuation must use one page per one-second iteration",
)
require(
    "constexpr int PeriodicGenreEnrichmentBatchSize = 8;" in WORKER,
    "TVScraper genre continuation must use a bounded eight-item batch",
)
require(
    "secondsSinceEpgContinuation >= PeriodicEpgContinuationSeconds" in WORKER,
    "EPG continuation scheduling must use the bounded one-second constant",
)
require(
    "PeriodicGenreEnrichmentBatchSize);" in WORKER,
    "periodic genre enrichment must use the latency-safe bounded batch",
)
require(
    "const int genreRefreshSeconds = 10;" not in WORKER,
    "legacy ten-second EPG genre scheduling must not return",
)

print("epg genre continuation latency contract ok")
