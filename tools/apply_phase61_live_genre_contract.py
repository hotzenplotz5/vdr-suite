#!/usr/bin/env python3
"""Apply the VDR-Live-aligned Phase 61 EPG Genre correction locally.

Mobile workflow:

    git pull --ff-only
    python3 tools/apply_phase61_live_genre_contract.py

The script validates every expected old block before writing any file. It never
commits, pushes, installs or restarts services.
"""

from __future__ import annotations

import subprocess
import sys
from pathlib import Path

EXPECTED_BRANCH = "feature/phase61-metadata-genre-browser"

Replacement = tuple[str, str]

REPLACEMENTS: dict[str, list[Replacement]] = {
    "core/metadata/src/GenreIndexRepositoryHelpers.inc": [
        (
            "'epg-browse-taxonomy-v1',",
            "'epg-browse-taxonomy-v2',",
        ),
        (
            """    bool scraperDocumentary = false;
    bool scraperSports = false;
    bool dvbMovie = false;
    bool dvbSeries = false;
    bool dvbDocumentary = false;
    bool dvbSports = false;
    bool dvbFilmGenre = false;
""",
            """    bool scraperDocumentary = false;
    bool scraperSports = false;
    bool dvbDocumentary = false;
    bool dvbSports = false;
""",
        ),
        (
            """        else if (providerId == "vdr-epg" && sourceKind == "dvb-content-descriptor")
        {
            dvbMovie = dvbMovie || genreId == "movie";
            dvbSeries = dvbSeries || genreId == "series";
            dvbDocumentary = dvbDocumentary || genreId == "documentary";
            dvbSports = dvbSports || genreId == "sports";
            dvbFilmGenre = dvbFilmGenre || filmGenreIds().count(genreId) > 0;
        }
""",
            """        else if (providerId == "vdr-epg" && sourceKind == "dvb-content-descriptor")
        {
            dvbDocumentary = dvbDocumentary || genreId == "documentary";
            dvbSports = dvbSports || genreId == "sports";
        }
""",
        ),
        (
            """        else if (scraperDocumentary)
        {
            contentClass = "documentary";
            confidence = 0.98;
        }
        else
        {
            contentClass = "movie";
            confidence = 0.99;
        }
""",
            """        else if (scraperDocumentary != scraperSports)
        {
            contentClass = scraperDocumentary ? "documentary" : "sports";
            confidence = 0.98;
        }
        else if (scraperDocumentary && scraperSports)
        {
            return true;
        }
        else
        {
            contentClass = "movie";
            confidence = 0.99;
        }
""",
        ),
        (
            """    else
    {
        if (dvbDocumentary && !dvbSeries && !dvbSports)
        {
            contentClass = "documentary";
        }
        else if (dvbSports && !dvbSeries && !dvbDocumentary)
        {
            contentClass = "sports";
        }
        else if (dvbSeries && !dvbDocumentary && !dvbSports)
        {
            contentClass = "series";
        }
        else if (dvbMovie &&
                 dvbFilmGenre &&
                 !dvbSeries &&
                 !dvbDocumentary &&
                 !dvbSports)
        {
            contentClass = "movie";
        }
        else
        {
            return true;
        }
        confidence = 0.65;
    }
""",
            """    else
    {
        if (dvbDocumentary != dvbSports)
        {
            contentClass = dvbDocumentary ? "documentary" : "sports";
        }
        else
        {
            return true;
        }
        confidence = 0.65;
    }
""",
        ),
    ],
    "core/metadata/src/GenreIndexRepositoryQueries.inc": [
        (
            """        "AND a.source_kind NOT IN("
        "'scraper-media-type','epg-browse-content-class') "
        "AND a.assignment_state IN('active','unknown') "
""",
            """        "AND a.provider_id='tvscraper' "
        "AND a.source_kind='scraper-metadata' "
        "AND a.assignment_state IN('active','unknown','conflict') "
""",
        ),
        (
            """          "AND g.source_kind NOT IN("
          "'scraper-media-type','epg-browse-content-class') "
          "AND g.assignment_state IN('active','unknown')) ";
""",
            """          "AND g.provider_id='tvscraper' "
          "AND g.source_kind='scraper-metadata' "
          "AND g.assignment_state IN('active','unknown','conflict')) ";
""",
        ),
    ],
    "core/metadata/src/GenreIndexRepositorySchema.inc": [
        (
            """        "CREATE INDEX IF NOT EXISTS idx_vdr_channel_cache_backend_number "
        "ON vdr_channel_cache(backend_id,channel_number,channel_id);"
    );
""",
            """        "CREATE INDEX IF NOT EXISTS idx_vdr_channel_cache_backend_number "
        "ON vdr_channel_cache(backend_id,channel_number,channel_id);"
        "DELETE FROM suite_metadata_genre_assignments "
        "WHERE source_kind='epg-browse-content-class' "
        "AND NOT EXISTS(SELECT 1 FROM suite_metadata_schema_versions WHERE version=6);"
        "INSERT OR IGNORE INTO suite_metadata_schema_versions(version,description) VALUES"
        "(6,'TVScraper-owned movie and series browse classification with DVB documentary and sports fallback');"
    );
""",
        ),
    ],
    "core/daemon/src/DaemonRuntimeEpgCache.cpp": [
        (
            "const int genreRefreshSeconds = 60;",
            "const int genreRefreshSeconds = 10;",
        ),
        (
            """                        fromTime + GenreWindowSeconds,
                        8);
""",
            """                        fromTime + GenreWindowSeconds,
                        64);
""",
        ),
        (
            """                fromTime + GenreWindowSeconds,
                32);
""",
            """                fromTime + GenreWindowSeconds,
                64);
""",
        ),
    ],
    "core/metadata/tests/test_genre_index_repository.cpp": [
        (
            "('a','C3','30','Lifestyle series','Episode','Text','1600','2600',1000,'Film/Unterhaltung'),",
            "('a','C3','30','Hartz Rot Gold','Episode','Text','1600','2600',1000,'Film/Drama'),",
        ),
        (
            """        assert(category(dvbBrowse, "movie").itemCount == 1);
        assert(category(dvbBrowse, "series").itemCount == 0);
""",
            """        assert(category(dvbBrowse, "movie").itemCount == 0);
        assert(category(dvbBrowse, "movie").children.empty());
        assert(category(dvbBrowse, "series").itemCount == 0);
""",
        ),
        (
            """        assert(dvbMovies.totalCount == 1);
        assert(dvbMovies.events.front().eventId == "20");
        assert(dvbMovies.events.front().title != "Lifestyle series");
""",
            """        assert(dvbMovies.totalCount == 0);
        assert(dvbMovies.events.empty());
""",
        ),
        (
            """        assert(series.totalCount == 1);
        assert(series.events.front().eventId == "30");
""",
            """        assert(series.totalCount == 1);
        assert(series.events.front().eventId == "30");
        assert(series.events.front().title == "Hartz Rot Gold");
""",
        ),
        (
            """        assert(isolated.totalCount == 1);
        assert(isolated.events.front().title == "Other");
""",
            """        assert(isolated.totalCount == 0);
        assert(isolated.events.empty());
""",
        ),
    ],
    "api/rest/tests/test_genre_browser_controller.cpp": [
        (
            """        "('default','C-1','101','Lifestyle Format','Episode','Description','" +
        std::to_string(now + 900) + "','" + std::to_string(now + 4500) +
        "',3600,'Film/Unterhaltung'),"
""",
            """        "('default','C-1','101','Hartz Rot Gold','Episode','Description','" +
        std::to_string(now + 900) + "','" + std::to_string(now + 4500) +
        "',3600,'Film/Drama'),"
""",
        ),
        (
            """    assert(contains(initialMovie, "\"eventId\":\"100\""));
    assert(contains(initialMovie, "\"channelName\":\"Das Erste HD\""));
    assert(contains(initialMovie, "\"available\":true"));
""",
            """    assert(!contains(initialMovie, "\"eventId\":\"100\""));
    assert(!contains(initialMovie, "Hartz Rot Gold"));
""",
        ),
        (
            """    assert(contains(initialMovies, "\"eventId\":\"100\""));
    assert(!contains(initialMovies, "\"eventId\":\"101\""));
""",
            """    assert(!contains(initialMovies, "\"eventId\":\"100\""));
    assert(!contains(initialMovies, "\"eventId\":\"101\""));
""",
        ),
        (
            "assert(contains(series, \"\\\"title\\\":\\\"Lifestyle Format\\\"\"));",
            "assert(contains(series, \"\\\"title\\\":\\\"Hartz Rot Gold\\\"\"));",
        ),
        (
            """    assert(contains(movies, "\"eventId\":\"100\""));
    assert(!contains(movies, "\"eventId\":\"101\""));
""",
            """    assert(contains(movies, "\"eventId\":\"100\""));
    assert(!contains(movies, "\"eventId\":\"101\""));
    assert(contains(movies, "\"channelName\":\"Das Erste HD\""));
    assert(contains(movies, "\"available\":true"));
""",
        ),
    ],
    "tools/check_genre_browser_architecture_contracts.py": [
        (
            """require(
    "suite_metadata_genre_assignments" in helpers
    and "suite_metadata_genre_assignments" in repository,
""",
            """require(
    "dvbMovie" not in helpers and "dvbSeries" not in helpers,
    "DVB descriptors must not classify events as movies or series",
)
require(
    "a.provider_id='tvscraper'" in repository
    and "a.source_kind='scraper-metadata'" in repository
    and "g.provider_id='tvscraper'" in repository
    and "g.source_kind='scraper-metadata'" in repository,
    "film subgenres must come exclusively from TVScraper genre evidence",
)
require(
    "epg-browse-taxonomy-v2" in helpers and "version=6" in schema,
    "corrected EPG browse taxonomy migration is missing",
)
require(
    "suite_metadata_genre_assignments" in helpers
    and "suite_metadata_genre_assignments" in repository,
""",
        ),
    ],
}


def command(root: Path, *args: str) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        args,
        cwd=root,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        check=False,
    )


def fail(message: str) -> int:
    print(f"ERROR: {message}")
    return 1


def main() -> int:
    root_result = subprocess.run(
        ["git", "rev-parse", "--show-toplevel"],
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        check=False,
    )
    if root_result.returncode != 0:
        print(root_result.stdout, end="")
        return root_result.returncode

    root = Path(root_result.stdout.strip())
    branch = command(root, "git", "branch", "--show-current").stdout.strip()
    if branch != EXPECTED_BRANCH:
        return fail(f"expected branch {EXPECTED_BRANCH}, found {branch or '<detached>'}")

    status = command(root, "git", "status", "--porcelain")
    if status.stdout:
        print("ERROR: working tree is not clean")
        print(status.stdout, end="")
        return 1

    originals: dict[Path, str] = {}
    updated: dict[Path, str] = {}

    for relative, replacements in REPLACEMENTS.items():
        path = root / relative
        if not path.is_file():
            return fail(f"missing file: {relative}")

        text = path.read_text(encoding="utf-8")
        originals[path] = text

        for number, (old, new) in enumerate(replacements, start=1):
            count = text.count(old)
            if count != 1:
                return fail(
                    f"{relative}: replacement {number} expected exactly once, found {count}"
                )
            text = text.replace(old, new, 1)

        updated[path] = text

    try:
        for path, text in updated.items():
            path.write_text(text, encoding="utf-8")
    except Exception as error:
        for path, text in originals.items():
            path.write_text(text, encoding="utf-8")
        return fail(f"write failed; original files restored: {error}")

    diff_check = command(root, "git", "diff", "--check")
    if diff_check.returncode != 0:
        for path, text in originals.items():
            path.write_text(text, encoding="utf-8")
        print("ERROR: git diff --check failed; original files restored")
        print(diff_check.stdout, end="")
        return diff_check.returncode

    print("PASS: VDR-Live Genre correction applied locally.")
    print(command(root, "git", "status", "--short").stdout, end="")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
