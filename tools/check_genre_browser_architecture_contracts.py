#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def read(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")


def require(condition: bool, message: str) -> None:
    if not condition:
        raise SystemExit(message)


router = read("api/rest/include/ApiRouter.h")
runtime_header = read("api/rest/include/GenreBrowserApiRuntime.h")
runtime = read("api/rest/src/GenreBrowserApiRuntime.cpp")
controller = read("api/rest/src/GenreBrowserController.cpp")
epg_worker = read("core/daemon/src/DaemonRuntimeEpgCache.cpp")
recording_worker = read("core/daemon/src/DaemonRuntimeRecordingCache.cpp")
backend_context = read("core/daemon/src/DaemonRuntimeBackendContext.cpp")
shutdown = read("core/daemon/src/DaemonRuntime.cpp")
repository = read("core/metadata/src/GenreIndexRepositoryQueries.inc")
schema = read("core/metadata/src/GenreIndexRepositorySchema.inc")
synchronization = read("core/metadata/src/GenreIndexRepositorySynchronization.inc")
channel_repository = read("core/vdr/src/VdrChannelCacheRepository.cpp")

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

continue_start = runtime.find("bool GenreBrowserApiRuntime::continueEpgEnrichment")
continue_end = runtime.find("bool GenreBrowserApiRuntime::tryHandleGet", continue_start)
require(continue_start >= 0 and continue_end > continue_start, "continuation enrichment boundary is missing")
continue_body = runtime[continue_start:continue_end]
require(
    "synchronizeEpgCache" not in continue_body,
    "periodic enrichment must never rematerialize the complete EPG Genre index",
)
require(
    "readDatabase_" in runtime_header
    and "readRepository_" in runtime_header
    and "writerRepository_" in runtime_header,
    "Genre runtime must own separate read and write repositories",
)
require("sqlite3_db_filename" in runtime, "Genre runtime must open a dedicated SQLite read connection")
require("PRAGMA journal_mode=WAL" in runtime, "Genre runtime must enable WAL read/write isolation")
require("PRAGMA query_only=ON" in runtime, "dedicated Genre read connection must remain query-only")

for forbidden in ("IEpgScraperMetadataResolver", "SuiteBridge", "TMDB", "IMDb"):
    require(forbidden not in controller, f"public controller depends on a provider/runtime: {forbidden}")

require("refreshEpgIndex" in epg_worker, "EPG worker does not materialize the genre index")
require("result.stored" in epg_worker, "EPG genre materialization must follow a stored cache refresh")
require("continueEpgEnrichment" in epg_worker, "periodic EPG enrichment continuation is missing")
periodic_start = epg_worker.find("if (secondsSinceGenreRefresh >= genreRefreshSeconds)")
periodic_end = epg_worker.find("if (!epgCacheDirtyHint_.load())", periodic_start)
require(periodic_start >= 0 and periodic_end > periodic_start, "periodic EPG enrichment block is missing")
periodic_body = epg_worker[periodic_start:periodic_end]
require("continueEpgEnrichment" in periodic_body, "periodic EPG work must use continuation enrichment")
require("refreshEpgIndex" not in periodic_body, "periodic EPG work must not rematerialize the complete index")
require("refreshRecordingIndex" in recording_worker, "recording worker does not materialize the genre index")
require("replaceRecordingsForBackend" in recording_worker, "recording genre materialization must follow cache persistence")
require("registerEpgScraperMetadataResolver" in backend_context, "backend-scoped EPG resolver registration is missing")
require("GenreBrowserApiRuntime::instance().reset()" in shutdown, "genre runtime reset is missing")
require(
    shutdown.find("GenreBrowserApiRuntime::instance().reset()") < shutdown.find("backendRuntimeContexts_.clear()"),
    "genre runtime must reset before backend resolver ownership is destroyed",
)

require("CREATE TABLE IF NOT EXISTS vdr_channel_cache" in schema, "persistent channel cache schema is missing")
require("BEGIN IMMEDIATE TRANSACTION" in channel_repository, "channel snapshots must replace atomically")
require("DELETE FROM vdr_channel_cache WHERE backend_id=?" in channel_repository, "channel snapshot replacement must be backend scoped")
require("VdrChannelCacheRepository channelCache(database_)" in epg_worker, "EPG worker must persist the existing channel snapshot")
require("replaceChannelsForBackend" in epg_worker, "EPG worker does not persist channel snapshots")
require("LEFT JOIN vdr_channel_cache" in repository, "EPG Genre query must join persisted channel metadata")
require("event.channelName" in controller, "EPG Genre API must serialize the persisted channel name")

require("COUNT(DISTINCT b.metadata_target_id)" in repository, "genre counts must be SQL distinct counts")
require("LIMIT ? OFFSET ?" in repository, "genre result pages must be SQL paginated")
require("boundedCandidateLimit" in repository, "EPG resolver candidate queries must have a hard bound")
require("b.backend_id=?" in repository, "genre queries must remain backend scoped")
require(
    "e.channel_id=b.channel_id AND e.event_id=b.native_id" in repository,
    "EPG Genre reads must use indexed native channel/event joins",
)
require(
    "idx_suite_metadata_target_bindings_epg_native" in schema,
    "EPG native binding lookup index is missing",
)
require(
    "EpgSynchronizationBatchSize" in synchronization
    and "begin += EpgSynchronizationBatchSize" in synchronization,
    "EPG genre synchronization must commit in bounded batches",
)
require(
    "std::this_thread::sleep_for(std::chrono::milliseconds(1))" in synchronization,
    "EPG synchronization must yield between write batches for waiting reads",
)
require(
    synchronization.count('BEGIN IMMEDIATE TRANSACTION;') >= 3,
    "EPG batching and retirement must use explicit transaction boundaries",
)

print("genre browser architecture contracts ok")
