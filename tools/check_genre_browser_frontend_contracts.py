#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def read(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")


def require(condition: bool, message: str) -> None:
    if not condition:
        raise SystemExit(message)


genres = read("web/frontend/modules/genres.js")
client = read("web/frontend/api/genre-client-api.js")
recordings = read("web/frontend/recordings2.js")
recording_view = read("web/frontend/recordings2-browser-view.js")
epg_owner = read("web/frontend/epg-detail-owner.js")
loader = read("web/frontend/platform/deferred-runtime-loader.js")
index = read("web/frontend/index.html")
remote = read("web/frontend/modules/remote.js")
runtime_test = read("web/frontend/tests/test_genres_runtime.js")

for forbidden in (
    "fetch(",
    "tvscraper",
    "tmdb",
    "imdb",
    "restfulapi",
    "SuiteBridge",
):
    require(
        forbidden not in genres,
        f"genres module violates provider/HTTP ownership: {forbidden}",
    )

require("fetchClientGenres" in genres, "genres module must use the genre Client API")
require("fetchClientGenreRecordings" in genres, "recording genre query must use the Client API")
require("fetchClientGenreEpg" in genres, "EPG genre query must use the Client API")
require("/api/metadata/genres" not in genres, "route literals belong to the Client API")
require("/api/metadata/genres" in client, "genre Client API route is missing")
require("document." not in client, "genre Client API extension must remain DOM-free")
require(
    "contentClass: normalized.contentClass" in client,
    "genre Client API must carry the EPG content class",
)
require(
    "contentClass:text(" in genres,
    "EPG result requests must use the selected main class",
)
require(
    "state.view = 'movie-genres'" in genres
    and "Filmgenres" in genres
    and "Alle Filme" in genres,
    "two-level Film navigation is missing",
)
require(
    "← EPG-Hauptkategorien" in genres
    and "← Filmgenres" in genres,
    "hierarchical EPG back navigation is incomplete",
)
for hidden in ("Nachrichten", "Talkshow", "Reality"):
    require(
        hidden not in genres,
        f"frontend must not implement a title/category special case for {hidden}",
    )
require(
    "fetchClientChannels" not in genres
    and "loadChannels" not in genres
    and "scheduleSupplementaryChannels" not in genres,
    "Genre browser must never issue supplementary VDR channel requests",
)
require(
    "event.channelName" in genres,
    "EPG Genre cards must consume channel names from the persisted read response",
)
require(
    "channelRequests,\n    0" in runtime_test
    and "database-only EPG navigation" in runtime_test
    and "for (let index = 0; index < 12; index += 1)" in runtime_test,
    "genre runtime test must cover repeated database-only EPG hierarchy navigation",
)
require(
    "contentClass" in runtime_test
    and "genreId" in runtime_test
    and "Nachrichten" in runtime_test,
    "genre runtime test must verify server-provided hierarchy and filtering",
)

require("createRecordingCard" in recording_view, "Recordings 2 card owner is not exported")
require(
    "VdrSuiteRecordings2BrowserView" in genres
    and "cardOwner.createRecordingCard(projected, function ()" in genres
    and "openRecording(recording);" in genres,
    "genres must reuse the Recordings 2 card owner while preserving original recording identity",
)
require(
    "VdrSuiteRecordings2MetadataDetail" in genres
    and "owner.fetchMetadata(recording, requestedBackend)" in genres,
    "recording genre cards must reuse the existing native metadata detail owner",
)
require(
    "VdrSuiteFrontendHelpers" in genres
    and "recordingMetadataPosterUrl" in genres,
    "recording genre cards must reuse the shared canonical poster helper",
)
require(
    "RECORDING_METADATA_CONCURRENCY = 4" in genres
    and "Math.min(RECORDING_METADATA_CONCURRENCY, queue.length)" in genres,
    "recording genre metadata enrichment must remain bounded",
)
require(
    "maximumActiveMetadataCalls <= 4" in runtime_test
    and "kind=gallery&index=1" in runtime_test
    and "kind=preferred" in runtime_test
    and "genre card click must hand the original recording" in runtime_test,
    "genre runtime test must prove bounded native poster enrichment and identity-preserving handoff",
)
require("openRecording" in recordings, "Recordings 2 external detail handoff is missing")
require("owner.openRecording" in genres, "genre recording clicks must use Recordings 2 detail ownership")
require("createEpgEventDetailCard" in epg_owner, "EPG owner bridge must use the existing detail renderer")
require("owner.open(event" in genres, "genre EPG clicks must use the EPG detail owner")
require(
    "attachPersistentArtwork" in epg_owner
    and "'/api/epg/cache/artwork?'" in epg_owner
    and "persistentArtworkUrl" in epg_owner,
    "genre EPG details must prefer the Suite-owned persistent artwork route",
)
require(
    "/api/epg/cache/metadata/image" not in epg_owner,
    "genre EPG detail ownership must not depend on transient provider image routes",
)

require("/frontend/modules/genres.js" in loader, "deferred genre runtime is missing")
require("/frontend/epg-detail-owner.js" in loader, "EPG detail owner runtime is missing")
require("loadVdrSuiteRecordings2Runtime()" in loader, "genre loader must depend on Recordings 2")
require("loadVdrSuiteEpgDetailRuntime()" in loader, "genre loader must depend on the EPG detail runtime")

order = [
    'data-module="overview"',
    'data-module="channels2"',
    'data-module="recordings2"',
    'data-module="genres"',
    'data-module="epg"',
    'data-module="channelsort"',
    'data-module="timers"',
    'data-module="searchtimers"',
]
positions = [index.find(token) for token in order]
require(all(position >= 0 for position in positions), "genre navigation entry is incomplete")
require(positions == sorted(positions), "static navigation order is incorrect")
require(
    "['overview','channels2','recordings2','genres','epg','channelsort','timers','searchtimers']"
    in remote,
    "remote runtime navigation order must include genres after recordings2",
)

print("genre browser frontend contracts ok")
