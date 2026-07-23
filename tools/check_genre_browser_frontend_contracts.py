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

for forbidden in ("fetch(", "tvscraper", "tmdb", "imdb", "restfulapi", "SuiteBridge"):
    require(forbidden not in genres, f"genres module violates provider/HTTP ownership: {forbidden}")

require("fetchClientGenres" in genres, "genres module must use the genre Client API")
require("fetchClientGenreRecordings" in genres, "recording genre query must use the Client API")
require("fetchClientGenreEpg" in genres, "EPG genre query must use the Client API")
require("/api/metadata/genres" not in genres, "route literals belong to the Client API")
require("/api/metadata/genres" in client, "genre Client API route is missing")
require("document." not in client, "genre Client API extension must remain DOM-free")
require(
    "Promise.all([client.fetchClientGenres" not in genres,
    "EPG genre overview must not wait for supplementary channel metadata",
)

overview_start = genres.find("function loadOverview()")
overview_end = genres.find("function requestItems(", overview_start)
require(overview_start >= 0 and overview_end > overview_start, "genre overview boundary is missing")
overview_body = genres[overview_start:overview_end]
require("client.fetchClientGenres(options)" in overview_body, "genre overview request is missing")
require("loadChannels();" not in overview_body, "EPG overview must not start channel loading before it renders")

selection_start = genres.find("function selectGenre(")
selection_end = genres.find("function loadMore(", selection_start)
require(selection_start >= 0 and selection_end > selection_start, "genre selection boundary is missing")
selection_body = genres[selection_start:selection_end]
require("requestItems(0)" in selection_body, "genre selection must request its result page")
require(
    "scheduleSupplementaryChannels(sequence)" in selection_body,
    "supplementary channel metadata must start only after EPG result rendering",
)
require(
    selection_body.find("requestItems(0)") < selection_body.find("scheduleSupplementaryChannels(sequence)"),
    "EPG result request must precede supplementary channel metadata",
)
require(
    "return new Promise(() => {});" in runtime_test
    and "EPG overview must not start supplementary channels before the genre response" in runtime_test
    and "EPG result request must complete before supplementary channels start" in runtime_test,
    "genre runtime test must cover serial HTTP request ordering with a pending channel request",
)

require("createRecordingCard" in recording_view, "Recordings 2 card owner is not exported")
require(
    "VdrSuiteRecordings2BrowserView" in genres
    and ".createRecordingCard(recording, openRecording)" in genres,
    "genres must reuse the Recordings 2 card owner",
)
require("openRecording" in recordings, "Recordings 2 external detail handoff is missing")
require("owner.openRecording" in genres, "genre recording clicks must use Recordings 2 detail ownership")
require("createEpgEventDetailCard" in epg_owner, "EPG owner bridge must use the existing detail renderer")
require("owner.open(event" in genres, "genre EPG clicks must use the EPG detail owner")

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
    "['overview','channels2','recordings2','genres','epg','channelsort','timers','searchtimers']" in remote,
    "remote runtime navigation order must include genres after recordings2",
)

print("genre browser frontend contracts ok")
