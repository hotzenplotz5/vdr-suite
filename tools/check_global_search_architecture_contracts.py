#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def read(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")


def require(condition: bool, message: str) -> None:
    if not condition:
        raise SystemExit(message)


router = read("api/rest/include/ApiRouter.h")
runtime = read("api/rest/src/GlobalSearchApiRuntime.cpp")
controller = read("api/rest/src/GlobalSearchController.cpp")
repository = read("core/vdr/src/GlobalSearchRepository.cpp")
service = read("core/vdr/src/GlobalSearchService.cpp")
epg_repository = read("core/vdr/src/EpgArtworkRepository.cpp")
epg_resolver = read("core/vdr/src/PersistentEpgScraperMetadataResolver.cpp")
client_api = read("web/frontend/api/client-api.js")
frontend = read("web/frontend/modules/global-search.js")
index = read("web/frontend/index.html")
remote = read("web/frontend/modules/remote.js")
backend_context = read("core/daemon/src/DaemonRuntimeBackendContext.cpp")
shutdown = read("core/daemon/src/DaemonRuntime.cpp")
install = read("mk/install.mk")
http_assets = read("core/http/src/TestHttpServerPaths.inc")
database_header = read("core/sqlite/include/Database.h")
database_implementation = read("core/sqlite/src/Database.cpp")

live = router.find("LiveRemoteApiRuntime::instance().tryHandleGet")
search = router.find("GlobalSearchApiRuntime::instance().tryHandleGet")
genre = router.find("GenreBrowserApiRuntime::instance().tryHandleGet")
legacy = router.find("return handleGet(requestTarget)")
require(min(live, search, genre, legacy) >= 0, "client GET runtime chain is incomplete")
require(live < search < genre < legacy, "global search must preserve remote precedence and precede legacy routing")

require('path != "/api/search"' in runtime and '"/api/vdr/search"' in runtime, "global search routes are missing")
require("PRAGMA query_only=ON" in runtime, "global search must use a query-only SQLite read connection")
require("database.filename()" in runtime, "global search does not open the existing database through the SQLite abstraction")
require("sqlite3_db_filename" not in runtime and "sqlite3.h" not in runtime, "global search runtime bypasses the SQLite abstraction")
require("std::string filename() const" in database_header, "Database filename abstraction is missing")
require("sqlite3_db_filename" in database_implementation, "Database filename abstraction is not implemented")
require("writerRepository->ensureSchema()" in runtime, "search index bootstrap is missing")
require("GlobalSearchService" in service and "repository_.search" in service, "dedicated search service is missing")
require("backendRegistryService_.getBackend" in controller, "backend registry validation is missing")
require("minimumQueryLength" in controller and '"too-short"' in controller, "short-query validation is missing")
require("MaximumLimit" in controller and "DefaultEpgFutureSeconds" in controller, "bounded limit or EPG window is missing")

for required in (
    "vdr_recording_cache",
    "vdr_recording_native_person",
    "epg_events",
    "epg_scraper_metadata_people",
    "ORDER BY CASE",
    "LIMIT :limit OFFSET :offset",
):
    require(required in repository, f"search repository contract is missing: {required}")
require("CREATE INDEX IF NOT EXISTS idx_epg_scraper_metadata_people_name" in repository, "EPG person index is missing")
require("candidate_hits" in repository and "COUNT(*) OVER()" in repository, "EPG search must use bounded set-based candidates")
require("epgTitlePredicate" not in repository and "epgPersonNameExpression" not in repository, "per-event correlated EPG search path returned")
require("json_each" in repository, "existing persisted EPG person backfill is missing")
require("BEGIN" not in repository[repository.find("GlobalSearchResult GlobalSearchRepository::search"):], "normal search path starts a write transaction")

require("replaceMetadataPeople" in epg_repository and "replaceMetadataPeople" in epg_resolver, "ongoing EPG person persistence is not wired")
require("GlobalSearchApiRuntime::instance().configure" in backend_context, "daemon search runtime configuration is missing")
require("GlobalSearchApiRuntime::instance().reset()" in shutdown, "daemon search runtime reset is missing")

require("fetchClientGlobalSearch" in client_api and "requestJson('/api/search'" in client_api, "client API search method is missing")
require("fetchClientGlobalSearch" in frontend, "frontend does not use VdrSuiteClientApi")
require("fetch(" not in frontend, "frontend search contains a direct fetch")
for forbidden in (
    "TVScraper",
    "TMDB",
    "IMDb",
    "RESTfulAPI",
    "SVDRP",
    "SuiteBridge",
    "VdrSuiteRecordingBrowser",
):
    require(forbidden not in frontend, f"frontend search leaks or depends on {forbidden}")
require("VdrSuiteRecordings2.openRecording" not in frontend, "recording detail call must remain owner-resolved")
require("VdrSuiteRecordings2" in frontend and "openRecording" in frontend, "Recordings 2 detail owner is missing")
require("VdrSuiteEpgDetailOwner" in frontend and "owner.open(event" in frontend, "EPG detail owner is missing")
require("createRecordingCard" in frontend, "Recordings 2 card owner is not reused")
require("createRequestCoordinator" in frontend and "AbortController" in frontend, "stale-response protection is missing")
require("REQUEST_TIMEOUT_MS" in frontend and "dauerte zu lange" in frontend, "visible search timeout is missing")
require("DEBOUNCE_MS" in frontend and "scheduleSearch" in frontend, "live-search debounce is missing")
for state in ("empty", "too-short", "loading", "error", "Keine Treffer"):
    require(state in frontend, f"visible frontend state is missing: {state}")
require("state.scrollTop" in frontend and "state.query" in frontend, "search return state is not retained")
require("global-search-scroll" in frontend and "overflow-y:auto" in frontend, "mobile search scrolling is missing")

remote_placeholder = index.find('<article class="brand-feature">', index.find('data-brand-module="epg"'))
search_card = index.find('data-brand-module="search"', remote_placeholder)
settings_card = index.find('data-brand-module="settings"', search_card)
require(remote_placeholder >= 0 and remote_placeholder < search_card < settings_card, "search launcher is not directly after the remote placeholder")
require("features[3]" not in remote, "unexpected remote implementation shape")
require("if(f[3])" in remote and "VDR - Fernbedienung" in remote, "existing remote launcher ownership changed")
require('/frontend/modules/global-search.js' in index, "global search frontend module is not loaded")
require('{"/frontend/modules/global-search.js", "modules/global-search.js"' in http_assets, "HTTP server does not publish the global search module")
require("web/frontend/modules/global-search.js $(DESTDIR)$(DATADIR)/web/frontend/modules/global-search.js" in install, "install-runtime does not publish global-search.js")

print("global search architecture contracts passed")
