#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def read(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")


def require(condition: bool, message: str) -> None:
    if not condition:
        raise SystemExit(message)


router = read("api/rest/include/ApiRouter.h")
runtime = read("api/rest/src/GenreBrowserApiRuntime.cpp")
controller = read("api/rest/src/GenreBrowserController.cpp")
epg_worker = read("core/daemon/src/DaemonRuntimeEpgCache.cpp")
recording_worker = read("core/daemon/src/DaemonRuntimeRecordingCache.cpp")
backend_context = read("core/daemon/src/DaemonRuntimeBackendContext.cpp")
shutdown = read("core/daemon/src/DaemonRuntime.cpp")
repository = read("core/metadata/src/GenreIndexRepositoryQueries.inc")

live_position = router.find("LiveRemoteApiRuntime::instance().tryHandleGet")
genre_position = router.find("GenreBrowserApiRuntime::instance().tryHandleGet")
legacy_position = router.find("return handleGet(requestTarget)")
require(live_position >= 0, "live remote GET runtime routing is missing")
require(genre_position >= 0, "genre GET runtime routing is missing")
require(legacy_position >= 0, "legacy GET routing is missing")
require(
    live_position < genre_position < legacy_position,
    "genre routing must remain after PR #99 live remote and before legacy routing",
)

get_start = runtime.find("bool GenreBrowserApiRuntime::tryHandleGet")
get_end = runtime.find("bool GenreBrowserApiRuntime::configured", get_start)
require(get_start >= 0 and get_end > get_start, "genre GET handler boundary is missing")
get_body = runtime[get_start:get_end]
for forbidden in ("resolve(", "refreshEpgIndex(", "refreshRecordingIndex("):
    require(forbidden not in get_body, f"genre GET handler performs synchronous enrichment: {forbidden}")

require("resolver->resolve" in runtime, "bounded asynchronous EPG enrichment is missing")
require("epgRefreshCandidates" in runtime, "EPG enrichment must use SQL-bounded candidates")
require("ResolverFreshnessSeconds" in runtime, "EPG enrichment freshness throttling is missing")
require("enrichmentLimit" in runtime, "EPG enrichment hard limit is missing")

for forbidden in ("IEpgScraperMetadataResolver", "SuiteBridge", "TMDB", "IMDb"):
    require(forbidden not in controller, f"public controller depends on a provider/runtime: {forbidden}")

require("refreshEpgIndex" in epg_worker, "EPG worker does not materialize the genre index")
require("result.stored" in epg_worker, "EPG genre materialization must follow a stored cache refresh")
require("refreshRecordingIndex" in recording_worker, "recording worker does not materialize the genre index")
require("replaceRecordingsForBackend" in recording_worker, "recording genre materialization must follow cache persistence")
require("registerEpgScraperMetadataResolver" in backend_context, "backend-scoped EPG resolver registration is missing")
require("GenreBrowserApiRuntime::instance().reset()" in shutdown, "genre runtime reset is missing")
require(
    shutdown.find("GenreBrowserApiRuntime::instance().reset()") < shutdown.find("backendRuntimeContexts_.clear()"),
    "genre runtime must reset before backend resolver ownership is destroyed",
)

require("COUNT(DISTINCT b.metadata_target_id)" in repository, "genre counts must be SQL distinct counts")
require("LIMIT ? OFFSET ?" in repository, "genre result pages must be SQL paginated")
require("boundedCandidateLimit" in repository, "EPG resolver candidate queries must have a hard bound")
require("b.backend_id=?" in repository, "genre queries must remain backend scoped")

print("genre browser architecture contracts ok")
