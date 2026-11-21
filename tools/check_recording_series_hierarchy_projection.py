#!/usr/bin/env python3

from pathlib import Path

header = Path(
    "core/metadata/include/GenreIndexRepository.h"
).read_text(encoding="utf-8")

queries = Path(
    "core/metadata/src/GenreIndexRepositoryQueries.inc"
).read_text(encoding="utf-8")

controller = Path(
    "api/rest/src/GenreBrowserController.cpp"
).read_text(encoding="utf-8")


def require(text, needle, owner):
    if needle not in text:
        raise SystemExit(
            f"{owner}: missing required contract: {needle}"
        )


genre_item_start = header.find(
    "struct GenreRecordingItem"
)

genre_item_end = header.find(
    "\n};",
    genre_item_start
)

if genre_item_start < 0 or genre_item_end < 0:
    raise SystemExit(
        "GenreRecordingItem: struct boundary missing"
    )

genre_item = header[
    genre_item_start:
    genre_item_end + 3
]

require(
    genre_item,
    "std::string resourceKey;",
    "GenreRecordingItem"
)

require(
    genre_item,
    "bool seriesHierarchyOverrideAvailable = false;",
    "GenreRecordingItem"
)

require(
    queries,
    "b.resource_key,c.title,c.path",
    "recordingsByGenre canonical resource key"
)

require(
    queries,
    "FROM recording_series_hierarchy_overrides ",
    "recordingsByGenre hierarchy batch"
)

require(
    queries,
    "WHERE backend_id=? AND recording_key IN (",
    "recordingsByGenre hierarchy batch"
)

require(
    queries,
    "item.seriesHierarchyOverrideAvailable =",
    "recordingsByGenre hierarchy projection"
)

require(
    controller,
    'appendJsonString(json, recording.resourceKey);',
    "Genre Browser recording response"
)

require(
    controller,
    "seriesHierarchyOverride",
    "Genre Browser recording response"
)

require(
    controller,
    "recording.seriesHierarchyGroupType",
    "Genre Browser hierarchy response"
)

require(
    controller,
    "recording.seriesHierarchyEpisodeStart",
    "Genre Browser hierarchy response"
)

require(
    controller,
    "recording.seriesHierarchyEpisodeEnd",
    "Genre Browser hierarchy response"
)

# The read model must batch from SQLite. It must never call the hierarchy
# HTTP runtime for each Recording.
if "RecordingSeriesHierarchyApiRuntime" in queries:
    raise SystemExit(
        "recordingsByGenre must not call hierarchy API runtime"
    )

# Exactly one hierarchy table read belongs in recordingsByGenre.
start = queries.find(
    "GenreRecordingPage "
    "GenreIndexRepository::recordingsByGenre("
)

end = queries.find(
    "\nGenreEpgPage ",
    start
)

if start < 0 or end < 0:
    raise SystemExit(
        "recordingsByGenre function boundary missing"
    )

function = queries[start:end]

count = function.count(
    "FROM recording_series_hierarchy_overrides "
)

if count != 1:
    raise SystemExit(
        "recordingsByGenre must contain exactly one "
        f"hierarchy override batch query, found {count}"
    )

# Native hierarchy must still be populated independently.
require(
    function,
    "item.seasonNumber =",
    "native Series hierarchy"
)

require(
    function,
    "item.episodeNumber =",
    "native Series hierarchy"
)

print(
    "recording Series hierarchy projection ownership, "
    "batching and native fallback contract ok"
)
