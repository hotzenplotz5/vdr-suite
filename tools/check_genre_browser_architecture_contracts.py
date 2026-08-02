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
refresh_gate = read("core/daemon/include/DaemonCacheRefreshExecutionGate.h")
backend_context = read("core/daemon/src/DaemonRuntimeBackendContext.cpp")
shutdown = read("core/daemon/src/DaemonRuntime.cpp")
repository = read("core/metadata/src/GenreIndexRepositoryQueries.inc")
helpers = read("core/metadata/src/GenreIndexRepositoryHelpers.inc")
live_parity = read("core/metadata/src/GenreIndexRepositoryLiveParity.inc")
storage = read("core/metadata/src/GenreIndexRepositoryStorage.inc")
schema = read("core/metadata/src/GenreIndexRepositorySchema.inc")
synchronization = read(
    "core/metadata/src/GenreIndexRepositorySynchronization.inc"
)
repository_cpp = read("core/metadata/src/GenreIndexRepository.cpp")
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
require(
    get_start >= 0 and get_end > get_start,
    "genre GET handler boundary is missing",
)
get_body = runtime[get_start:get_end]
for forbidden in ("resolve(", "refreshEpgIndex(", "refreshRecordingIndex("):
    require(
        forbidden not in get_body,
        f"genre GET handler performs synchronous enrichment: {forbidden}",
    )

require("resolver->resolve" in runtime, "bounded asynchronous EPG enrichment is missing")
require("epgRefreshCandidates" in runtime, "EPG enrichment must use SQL-bounded candidates")
require("ResolverFreshnessSeconds" in runtime, "EPG enrichment freshness throttling is missing")
require("enrichmentLimit" in runtime, "EPG enrichment hard limit is missing")
require(
    "resolution.metadata.mediaType" in runtime
    and '"scraper-media-type"' in runtime,
    "TVScraper media type must be persisted as classification evidence",
)
require(
    "replaceEpgEvidenceAndReconcile" in runtime
    and "replaceEpgEvidenceAndReconcile" in storage
    and "acquireTransactionLease" in storage,
    "TVScraper evidence must be stored and reconciled atomically",
)
require(
    "contentClassFrom" in runtime,
    "EPG result routing must carry the explicit content class",
)

continue_start = runtime.find("bool GenreBrowserApiRuntime::continueEpgEnrichment")
continue_end = runtime.find("bool GenreBrowserApiRuntime::tryHandleGet", continue_start)
require(
    continue_start >= 0 and continue_end > continue_start,
    "continuation enrichment boundary is missing",
)
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
require(
    "sqlite3_db_filename" in runtime,
    "Genre runtime must open a dedicated SQLite read connection",
)
require(
    "PRAGMA journal_mode=WAL" in runtime,
    "Genre runtime must enable WAL read/write isolation",
)
require(
    "PRAGMA query_only=ON" in runtime,
    "dedicated Genre read connection must remain query-only",
)

for forbidden in ("IEpgScraperMetadataResolver", "SuiteBridge", "TMDB", "IMDb"):
    require(
        forbidden not in controller,
        f"public controller depends on a provider/runtime: {forbidden}",
    )

require("refreshEpgIndex" in epg_worker, "EPG worker does not materialize the genre index")
require("result.stored" in epg_worker, "EPG genre materialization must follow a stored cache refresh")
require("continueEpgEnrichment" in epg_worker, "periodic EPG enrichment continuation is missing")
periodic_start = epg_worker.find(
    "if (secondsSinceEpgContinuation >= PeriodicEpgContinuationSeconds)"
)
periodic_end = epg_worker.find("if (!epgCacheDirtyHint_.load())", periodic_start)
require(
    periodic_start >= 0 and periodic_end > periodic_start,
    "periodic EPG enrichment block is missing",
)
periodic_body = epg_worker[periodic_start:periodic_end]
require(
    "continueEpgEnrichment" in periodic_body,
    "periodic EPG work must use continuation enrichment",
)
require(
    "refreshEpgIndex" not in periodic_body,
    "periodic EPG work must not rematerialize the complete index",
)
require("refreshRecordingIndex" in recording_worker, "recording worker does not materialize the genre index")
require("replaceRecordingsForBackend" in recording_worker, "recording genre materialization must follow cache persistence")
require(
    "readRecordingEvidenceSignatures" in synchronization
    and "changedRecordingKeys.empty() && !needsRetirement"
    in synchronization,
    "unchanged recording Genre synchronization must avoid write transactions",
)
require("registerEpgScraperMetadataResolver" in backend_context, "backend-scoped EPG resolver registration is missing")
require("GenreBrowserApiRuntime::instance().reset()" in shutdown, "genre runtime reset is missing")
require(
    shutdown.find("GenreBrowserApiRuntime::instance().reset()")
    < shutdown.find("backendRuntimeContexts_.clear()"),
    "genre runtime must reset before backend resolver ownership is destroyed",
)

require(
    "class DaemonCacheRefreshExecutionGate" in refresh_gate
    and "std::unique_lock<std::mutex>" in refresh_gate,
    "daemon cache refresh execution gate is missing",
)
require(
    epg_worker.count("DaemonCacheRefreshExecutionGate::acquire()") >= 2,
    "EPG cache and periodic Genre writes must use the shared execution gate",
)
require(
    recording_worker.count("DaemonCacheRefreshExecutionGate::acquire()") >= 2,
    "recording cache and periodic metadata writes must use the shared execution gate",
)

require("CREATE TABLE IF NOT EXISTS vdr_channel_cache" in schema, "persistent channel cache schema is missing")
require("BEGIN IMMEDIATE TRANSACTION" in channel_repository, "channel snapshots must replace atomically")
require("DELETE FROM vdr_channel_cache WHERE backend_id=?" in channel_repository, "channel snapshot replacement must be backend scoped")
require("VdrChannelCacheRepository channelCache(database_)" in epg_worker, "EPG worker must persist the existing channel snapshot")
require("replaceChannelsForBackend" in epg_worker, "EPG worker does not persist channel snapshots")
require("LEFT JOIN vdr_channel_cache" in repository, "EPG Genre query must join persisted channel metadata")
require("event.channelName" in controller, "EPG Genre API must serialize the persisted channel name")

require(
    "epg-browse-content-class" in helpers
    and "reconcileEpgBrowseClassificationLockedV2" in repository_cpp
    and "GenreIndexRepositoryLiveParity.inc" in repository_cpp,
    "Live-parity derived EPG browse classification wiring is missing",
)
require(
    "epg-browse-taxonomy-v7" in live_parity
    and "version=10" in live_parity
    and "version=11" in live_parity
    and "version=12" in live_parity,
    "fiction-confirmed EPG browse taxonomy v7 migrations are missing",
)
require(
    "liveParityStrongNewsTitle" in live_parity
    and '"tagesschau"' in live_parity
    and '"tagesschau24"' in live_parity
    and '"tagesthemen"' in live_parity,
    "ARD news title guards must veto series classification without DVB labels",
)
require(
    "liveParityStrongSportsTitle" in live_parity
    and '"sportschau"' in live_parity,
    "sports title guards must outrank scraper series labels",
)
require(
    "lastKnownMediaSeries" in live_parity
    and "lastKnownMediaMovie" in live_parity
    and "lastKnownScraperFiction" in live_parity
    and "lastKnownScraperNonFiction" in live_parity,
    "last-known TVScraper types must remain bounded by fiction evidence",
)
require(
    "if (dvbSports || strongSportsTitle)" in live_parity
    and "else if (dvbDocumentary)" in live_parity,
    "DVB sport/documentary and strong sports titles must outrank scraper labels",
)
require(
    "liveParitySeriesFictionGenre" in live_parity
    and "liveParitySeriesNonFictionGenre" in live_parity
    and "activeScraperFiction" in live_parity
    and "activeScraperNonFiction" in live_parity
    and "if (!(activeScraperFiction || dvbFiction)) return true;" in live_parity
    and "if (!scraperFiction || scraperNonFiction) return true;" in live_parity,
    "TVScraper series candidates must require positive fictional evidence",
)
require(
    "dvbSpecificFilmGenre" in live_parity
    and "if (!dvbMovie || !dvbSpecificFilmGenre)" in live_parity
    and "FeatureFilmMinimumSeconds" not in live_parity,
    "strict specific-genre DVB movie fallback is missing",
)
require(
    "recordingFolderGenreCandidate" in synchronization
    and "recording-folder-genre" in synchronization
    and 'genre.id == "movie"' in synchronization,
    "known recording folder Genre fallback is missing",
)
require(
    "for (const std::string& key : changedRecordingKeys)"
    in synchronization
    and "DELETE FROM suite_metadata_genre_assignments"
    in synchronization
    and "'recording-metadata'," in synchronization
    and "'recording-folder-genre');" in synchronization,
    "changed recording snapshots must remove stale folder and metadata evidence",
)
require(
    "suite_metadata_genre_assignments" in helpers
    and "suite_metadata_genre_assignments" in repository,
    "EPG browse taxonomy must reuse the persistent metadata genre index",
)
require(
    "source_kind NOT IN('scraper-media-type','epg-browse-content-class')" in helpers,
    "content-class evidence must not participate in flat genre conflicts",
)
require(
    "epgBrowseOverview" in repository
    and "epgByBrowse" in repository,
    "hierarchical EPG browse queries are missing",
)
require(
    "AND c.genre_id=?" in repository
    and "AND g.genre_id=?" in repository,
    "EPG film genre reads must require content class and genre server-side",
)
require(
    '"movie",\n        "series",\n        "documentary",\n        "sports"' in repository,
    "EPG overview must expose the four ordered browse classes",
)
for hidden in ("news", "talk-show", "reality", "unclassified"):
    require(
        f'"{hidden}"' not in repository[
            repository.find("const std::vector<std::string> categoryIds"):
            repository.find("CanonicalGenreRegistry registry;")
        ],
        f"{hidden} must not be an EPG browse main category",
    )
require(
    "filmGenreIds()" in storage,
    "film subgenre validation must use the bounded canonical whitelist",
)

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
    "idx_suite_metadata_genre_assignments_source" in schema,
    "EPG browse source/classification index is missing",
)
require(
    "EpgSynchronizationBatchSize" in synchronization
    and "begin += EpgSynchronizationBatchSize" in synchronization,
    "EPG genre synchronization must commit in bounded batches",
)
require(
    "reconcileEpgBrowseClassificationLocked" in synchronization,
    "DVB fallback must reconcile the EPG browse class during cache sync",
)
require(
    "std::this_thread::sleep_for(std::chrono::milliseconds(1))"
    in synchronization,
    "EPG synchronization must yield between write batches for waiting reads",
)
require(
    synchronization.count("BEGIN IMMEDIATE TRANSACTION;") >= 3,
    "EPG batching and retirement must use explicit transaction boundaries",
)

print("genre browser architecture contracts ok")
