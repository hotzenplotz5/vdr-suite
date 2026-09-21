#!/usr/bin/env python3

import argparse
import datetime as dt
import json
import os
import socket
import sqlite3
import sys
import tempfile
import time
import urllib.parse
from pathlib import Path


DEFAULT_SUITE_DATABASE = "/var/lib/vdr-suite/vdr-suite.db"
DEFAULT_BACKEND = "default"
DEFAULT_SVDRP_HOST = "127.0.0.1"
DEFAULT_SVDRP_PORT = 6419
DEFAULT_WINDOW_HOURS = 48
DEFAULT_LIMIT = 32
FILM_GENRES = frozenset(
    {
        "action",
        "adventure",
        "animation",
        "drama",
        "family",
        "fantasy",
        "history",
        "horror",
        "disaster",
        "comedy",
        "war",
        "crime",
        "musical",
        "mystery",
        "romance",
        "science-fiction",
        "thriller",
        "western",
    }
)
TVSCRAPER_DATABASE_CANDIDATES = (
    "/dev/shm/tvscraper2.db",
    "/var/cache/vdr/tvscraper/tvscraper2.db",
    "/var/cache/vdr/plugins/tvscraper/tvscraper2.db",
    "/var/lib/vdr/plugins/tvscraper/tvscraper2.db",
    "/var/lib/vdr/tvscraper/tvscraper2.db",
)


class ComparisonFailure(RuntimeError):
    pass


def require(condition, message):
    if not condition:
        raise ComparisonFailure(message)


def utc_iso(epoch=None):
    if epoch is None:
        epoch = time.time()
    return dt.datetime.fromtimestamp(epoch, dt.timezone.utc).isoformat(
        timespec="seconds"
    )


def atomic_write_json(path, value):
    output = Path(path)
    output.parent.mkdir(parents=True, exist_ok=True)
    temporary = output.with_name(output.name + ".new")
    temporary.write_text(
        json.dumps(value, ensure_ascii=False, indent=2, sort_keys=True) + "\n",
        encoding="utf-8",
    )
    os.replace(temporary, output)


def open_read_only_database(path):
    resolved = Path(path).resolve()
    require(resolved.is_file(), f"Datenbank fehlt: {resolved}")
    uri = "file:" + urllib.parse.quote(str(resolved), safe="/") + "?mode=ro"
    connection = sqlite3.connect(uri, uri=True, timeout=10)
    connection.row_factory = sqlite3.Row
    connection.execute("PRAGMA query_only=ON")
    connection.execute("PRAGMA busy_timeout=10000")
    return connection


def detect_tvscraper_database(explicit_path):
    if explicit_path:
        require(
            Path(explicit_path).is_file(),
            f"TVScraper-Datenbank fehlt: {explicit_path}",
        )
        return str(Path(explicit_path).resolve())

    for candidate in TVSCRAPER_DATABASE_CANDIDATES:
        if Path(candidate).is_file():
            return candidate

    checked = ", ".join(TVSCRAPER_DATABASE_CANDIDATES)
    raise ComparisonFailure(
        "TVScraper-Datenbank nicht gefunden; mit "
        f"--tvscraper-database angeben. Geprüft: {checked}"
    )


def split_tvscraper_list(value):
    result = []
    seen = set()
    for item in str(value or "").split("|"):
        if not item or item in seen:
            continue
        seen.add(item)
        result.append(item)
    return result


class SvdrpClient:
    def __init__(self, host, port, timeout_seconds):
        self.host = host
        self.port = port
        self.timeout_seconds = timeout_seconds

    @staticmethod
    def _read_reply(stream, maximum_lines=4096, maximum_bytes=1024 * 1024):
        code = None
        payload = []
        received = 0

        for _ in range(maximum_lines):
            raw = stream.readline()
            if not raw:
                raise ComparisonFailure(
                    "SVDRP-Verbindung endete vor vollständiger Antwort"
                )
            received += len(raw)
            if received > maximum_bytes:
                raise ComparisonFailure("SVDRP-Antwort überschreitet Größenlimit")
            line = raw.decode("utf-8", errors="replace").rstrip("\r\n")
            if (
                len(line) < 4
                or not line[:3].isdigit()
                or line[3] not in (" ", "-")
            ):
                raise ComparisonFailure(
                    f"Ungültige SVDRP-Antwortzeile: {line[:200]}"
                )

            line_code = int(line[:3])
            if code is None:
                code = line_code
            elif line_code != code:
                raise ComparisonFailure(
                    "SVDRP-Mehrzeilenantwort enthält verschiedene Antwortcodes"
                )

            payload.append(line[4:])
            if line[3] == " ":
                return code, "\n".join(payload)

        raise ComparisonFailure("SVDRP-Antwort überschreitet Zeilenlimit")

    def execute(self, command):
        try:
            with socket.create_connection(
                (self.host, self.port), timeout=self.timeout_seconds
            ) as connection:
                connection.settimeout(self.timeout_seconds)
                with connection.makefile("rwb", buffering=0) as stream:
                    greeting_code, greeting = self._read_reply(stream)
                    require(
                        greeting_code == 220,
                        "Unerwartete SVDRP-Begrüßung "
                        f"{greeting_code}: {greeting[:300]}",
                    )
                    stream.write((command + "\r\n").encode("utf-8"))
                    return self._read_reply(stream)
        except (OSError, TimeoutError) as error:
            raise ComparisonFailure(f"SVDRP-Verbindung fehlgeschlagen: {error}") from error


def parse_etypes_payload(payload):
    fields = payload.split("|", 4)
    require(len(fields) == 5, "ETYPES-Payload hat nicht fünf Kopffelder")
    schema, next_offset, scanned, done, items_text = fields
    require(schema == "1", f"Unbekanntes ETYPES-Schema: {schema}")
    require(next_offset.isdigit(), "ETYPES nextOffset ist ungültig")
    require(scanned.isdigit(), "ETYPES scanned ist ungültig")
    require(done in ("0", "1"), "ETYPES done ist ungültig")

    items = []
    if items_text:
        for encoded in items_text.split(";"):
            values = encoded.split(",")
            require(len(values) == 5, f"Ungültiger ETYPES-Eintrag: {encoded}")
            channel_id, event_id, start_time, end_time, media_token = values
            require(
                event_id.isdigit()
                and start_time.isdigit()
                and end_time.isdigit()
                and media_token in ("M", "S"),
                f"Ungültiger ETYPES-Eintrag: {encoded}",
            )
            items.append(
                {
                    "channelId": channel_id,
                    "eventId": event_id,
                    "startTime": int(start_time),
                    "endTime": int(end_time),
                    "mediaType": "movie" if media_token == "M" else "series",
                }
            )

    return {
        "nextOffset": int(next_offset),
        "scanned": int(scanned),
        "done": done == "1",
        "items": items,
    }


def fetch_etypes(client, from_epoch, until_epoch):
    offset = 0
    result = {}
    pages = 0
    while True:
        code, payload = client.execute(
            "PLUG suitebridge ETYPES "
            f"{from_epoch} {until_epoch} {offset} 64"
        )
        require(code == 250, f"ETYPES antwortet mit {code}: {payload[:500]}")
        page = parse_etypes_payload(payload)
        require(
            page["nextOffset"] >= offset,
            "ETYPES nextOffset läuft rückwärts",
        )
        require(
            page["scanned"] > 0 or page["done"],
            "ETYPES liefert leere, unvollständige Seite",
        )
        for item in page["items"]:
            key = (
                item["channelId"],
                item["eventId"],
                item["startTime"],
                item["endTime"],
            )
            result[key] = item["mediaType"]

        pages += 1
        require(pages <= 20000, "ETYPES überschreitet Seitengrenze")
        if page["done"]:
            return result, pages
        require(
            page["nextOffset"] > offset,
            "ETYPES macht keinen Fortschritt",
        )
        offset = page["nextOffset"]


def parse_json_reply(command_name, code, payload):
    require(code == 250, f"{command_name} antwortet mit {code}: {payload[:500]}")
    try:
        value = json.loads(payload)
    except json.JSONDecodeError as error:
        raise ComparisonFailure(
            f"{command_name} liefert ungültiges JSON: {error}"
        ) from error
    require(isinstance(value, dict), f"{command_name} liefert kein JSON-Objekt")
    return value


def normalized_identity(value):
    require(isinstance(value, dict), "MCOMPARE enthält keine Ereignisidentität")
    channel_id = value.get("channelId")
    event_id = value.get("eventId")
    start_time = value.get("startTime")
    end_time = value.get("endTime")
    title = value.get("title")
    require(isinstance(channel_id, str) and channel_id, "MCOMPARE channelId fehlt")
    require(
        isinstance(event_id, int) and not isinstance(event_id, bool),
        "MCOMPARE eventId ist ungültig",
    )
    require(
        isinstance(start_time, int) and not isinstance(start_time, bool),
        "MCOMPARE startTime ist ungültig",
    )
    require(
        isinstance(end_time, int) and not isinstance(end_time, bool),
        "MCOMPARE endTime ist ungültig",
    )
    require(isinstance(title, str), "MCOMPARE title ist ungültig")
    return {
        "channelId": channel_id,
        "eventId": event_id,
        "startTime": start_time,
        "endTime": end_time,
        "title": title,
    }


def normalized_metadata(value):
    require(isinstance(value, dict), "Metadatenvergleich enthält kein Objekt")
    found = value.get("found")
    media_type = value.get("mediaType")
    provider = value.get("provider")
    provider_id = value.get("providerId")
    genres = value.get("genres")
    require(isinstance(found, bool), "Metadatenvergleich: found fehlt")
    require(
        media_type in ("none", "movie", "series"),
        "Metadatenvergleich: mediaType ungültig",
    )
    require(
        provider in ("none", "tvscraper"),
        "Metadatenvergleich: provider ungültig",
    )
    require(isinstance(provider_id, int), "Metadatenvergleich: providerId ungültig")
    require(
        isinstance(genres, list) and all(isinstance(item, str) for item in genres),
        "Metadatenvergleich: genres ungültig",
    )
    return {
        "found": found,
        "provider": provider,
        "providerId": provider_id,
        "mediaType": media_type,
        "genres": genres,
    }


def validate_tvscraper_schema(connection):
    require(
        all(
            connection.execute(
                "SELECT 1 FROM sqlite_master WHERE type='table' AND name=?",
                (table,),
            ).fetchone()
            for table in ("event", "movies3", "tv2")
        ),
        "TVScraper-Datenbank besitzt nicht das erwartete 1.2.15-Schema",
    )


def resolve_raw_tvscraper(connection, channel_id, event_id):
    row = connection.execute(
        """
        SELECT movie_tv_id,season_number,episode_number,runtime
          FROM event
         WHERE event_id=? AND channel_id=?
         LIMIT 1
        """,
        (event_id, channel_id),
    ).fetchone()
    if row is None:
        return {
            "found": False,
            "provider": "none",
            "providerId": 0,
            "mediaType": "none",
            "genres": [],
            "reason": "event-mapping-missing",
        }

    provider_id = int(row["movie_tv_id"] or 0)
    season_number = int(row["season_number"] or 0)
    if season_number == -100:
        media_type = "movie"
        details = connection.execute(
            "SELECT movie_genres FROM movies3 WHERE movie_id=? LIMIT 1",
            (provider_id,),
        ).fetchone()
        raw_genres = details["movie_genres"] if details is not None else ""
        reason = "" if details is not None else "movie-row-missing"
    else:
        media_type = "series"
        details = connection.execute(
            "SELECT tv_genres FROM tv2 WHERE tv_id=? LIMIT 1",
            (provider_id,),
        ).fetchone()
        raw_genres = details["tv_genres"] if details is not None else ""
        reason = "" if details is not None else "series-row-missing"

    return {
        "found": details is not None,
        "provider": "tvscraper" if details is not None else "none",
        "providerId": provider_id,
        "mediaType": media_type if details is not None else "none",
        "genres": split_tvscraper_list(raw_genres),
        "seasonNumber": season_number,
        "episodeNumber": int(row["episode_number"] or 0),
        "runtime": int(row["runtime"] or 0),
        "reason": reason,
    }


def select_movie_candidates(connection, backend, from_epoch, until_epoch):
    rows = connection.execute(
        """
        SELECT b.metadata_target_id,
               b.backend_id,
               b.channel_id,
               b.native_id AS event_id,
               b.start_time,
               b.end_time,
               e.title,
               e.subtitle,
               CASE WHEN EXISTS(
                    SELECT 1
                      FROM suite_metadata_genre_assignments g
                     WHERE g.metadata_target_id=b.metadata_target_id
                       AND g.provider_id='tvscraper'
                       AND g.source_kind='scraper-metadata'
                       AND g.assignment_state IN('active','unknown','conflict')
                       AND g.genre_id IN(
                         'action','adventure','animation','drama','family',
                         'fantasy','history','horror','disaster','comedy',
                         'war','crime','musical','mystery','romance',
                         'science-fiction','thriller','western'
                       )
               ) THEN 1 ELSE 0 END AS has_film_genre
          FROM suite_metadata_target_bindings b
          JOIN epg_events e
            ON e.backend_id=b.backend_id
           AND e.channel_id=b.channel_id
           AND e.event_id=b.native_id
          JOIN suite_metadata_genre_assignments c
            ON c.metadata_target_id=b.metadata_target_id
         WHERE b.backend_id=?
           AND b.target_type='program-event'
           AND b.lifecycle_state='active'
           AND b.end_time>?
           AND b.start_time<?
           AND c.source_kind='epg-browse-content-class'
           AND c.assignment_state='active'
           AND c.genre_id='movie'
         GROUP BY b.metadata_target_id
         ORDER BY has_film_genre ASC,b.start_time,b.channel_id,b.native_id
        """,
        (backend, from_epoch, until_epoch),
    ).fetchall()
    return [dict(row) for row in rows]


def evidence_for(connection, target_id):
    rows = connection.execute(
        """
        SELECT provider_id,source_kind,genre_id,original_value,
               assignment_state,confidence,ordinal,observed_at
          FROM suite_metadata_genre_assignments
         WHERE metadata_target_id=?
         ORDER BY provider_id,source_kind,ordinal,genre_id,original_value
        """,
        (target_id,),
    ).fetchall()
    return [
        {
            "provider": row["provider_id"],
            "sourceKind": row["source_kind"],
            "genreId": row["genre_id"],
            "originalValue": row["original_value"],
            "state": row["assignment_state"],
            "confidence": float(row["confidence"]),
            "ordinal": int(row["ordinal"]),
            "observedAt": int(row["observed_at"]),
        }
        for row in rows
    ]


def metadata_signature(value):
    return (
        bool(value["found"]),
        str(value["provider"]),
        int(value["providerId"]),
        str(value["mediaType"]),
        tuple(value["genres"]),
    )


def genre_set(value):
    return set(value.get("genres", []))


def evaluate_record(record):
    failures = []
    diagnostics = []

    live = record["live"]
    detached = record["detached"]
    meta = record["meta"]
    raw = record["rawTvscraper"]
    etypes = record["etypes"]
    evidence = record["persistedEvidence"]
    identity = record["comparisonIdentity"]

    expected_identity = (
        str(record["channelId"]),
        str(record["eventId"]),
        int(record["startTime"]),
        int(record["endTime"]),
    )
    actual_identity = (
        str(identity.get("channelId", "")),
        str(identity.get("eventId", "")),
        int(identity.get("startTime", 0)),
        int(identity.get("endTime", 0)),
    )
    if actual_identity != expected_identity:
        failures.append("event-identity-mismatch")

    if metadata_signature(live) != metadata_signature(detached):
        failures.append("live-detached-mismatch")
    if metadata_signature(detached) != metadata_signature(meta):
        failures.append("detached-meta-mismatch")

    raw_signature = (
        bool(raw["found"]),
        str(raw["provider"]),
        int(raw["providerId"]),
        str(raw["mediaType"]),
        tuple(raw["genres"]),
    )
    if metadata_signature(live) != raw_signature:
        failures.append("raw-live-mismatch")

    if etypes != "movie":
        failures.append("etypes-not-movie")

    persisted_meta = [
        item
        for item in evidence
        if item["provider"] == "tvscraper"
        and item["sourceKind"] == "scraper-metadata"
    ]
    persisted_original_values = {
        item["originalValue"]
        for item in persisted_meta
        if item["originalValue"]
    }
    if persisted_original_values != genre_set(meta):
        failures.append("meta-persistence-mismatch")

    if meta["found"] and not meta["genres"]:
        missing = any(
            item["state"] == "missing" and not item["originalValue"]
            for item in persisted_meta
        )
        if not missing:
            failures.append("missing-state-not-persisted")
    elif not meta["found"]:
        stale = any(
            item["state"] == "stale" and not item["originalValue"]
            for item in persisted_meta
        )
        if not stale:
            failures.append("not-found-state-not-persisted")

    persisted_media_types = {
        item["originalValue"]
        for item in evidence
        if item["provider"] == "tvscraper-media-type"
        and item["sourceKind"] == "scraper-media-type"
        and item["state"] == "active"
        and item["originalValue"]
    }
    if persisted_media_types != {"movie"}:
        failures.append("etype-persistence-mismatch")

    browse_classes = {
        item["genreId"]
        for item in evidence
        if item["sourceKind"] == "epg-browse-content-class"
        and item["state"] == "active"
    }
    if browse_classes != {"movie"}:
        failures.append("browse-class-not-movie")

    canonical_film_genres = sorted(
        {
            item["genreId"]
            for item in persisted_meta
            if item["state"] in ("active", "unknown", "conflict")
            and item["genreId"] in FILM_GENRES
        }
    )

    if live["found"] and not live["genres"]:
        diagnostics.append("tvscraper-movie-without-genres")
    elif live["genres"] and not canonical_film_genres:
        diagnostics.append("genres-not-in-canonical-film-taxonomy")
    if raw.get("reason"):
        diagnostics.append(raw["reason"])

    record["persistedMetaOriginalValues"] = sorted(persisted_original_values)
    record["persistedCanonicalFilmGenres"] = canonical_film_genres
    record["persistedMediaTypes"] = sorted(persisted_media_types)
    record["browseClasses"] = sorted(browse_classes)
    record["failures"] = failures
    record["diagnostics"] = diagnostics
    record["result"] = "FAIL" if failures else "PASS"
    return record


def load_resume(path):
    if not path or not Path(path).is_file():
        return None
    try:
        value = json.loads(Path(path).read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as error:
        raise ComparisonFailure(f"Resume-Datei ist ungültig: {error}") from error
    require(isinstance(value, dict), "Resume-Datei enthält kein Objekt")
    require(isinstance(value.get("events", []), list), "Resume-Datei enthält keine Eventliste")
    return value


def make_report(args, from_epoch, until_epoch, tvscraper_database):
    return {
        "schema": 1,
        "tool": "compare_phase61_live_tvscraper",
        "generatedAt": utc_iso(),
        "backend": args.backend,
        "suiteDatabase": str(Path(args.database).resolve()),
        "tvscraperDatabase": str(Path(tvscraper_database).resolve()),
        "svdrp": {"host": args.svdrp_host, "port": args.svdrp_port},
        "window": {"from": from_epoch, "until": until_epoch},
        "requestedLimit": args.limit,
        "events": [],
        "summary": {},
        "result": "RUNNING",
    }


def update_summary(report, movie_total, etypes_pages):
    events = report["events"]
    counts = {
        "movieCandidates": movie_total,
        "inspected": len(events),
        "passed": sum(item.get("result") == "PASS" for item in events),
        "failed": sum(item.get("result") == "FAIL" for item in events),
        "liveFound": sum(item.get("live", {}).get("found", False) for item in events),
        "liveWithGenres": sum(bool(item.get("live", {}).get("genres")) for item in events),
        "detachedWithGenres": sum(
            bool(item.get("detached", {}).get("genres")) for item in events
        ),
        "metaWithGenres": sum(bool(item.get("meta", {}).get("genres")) for item in events),
        "rawWithGenres": sum(
            bool(item.get("rawTvscraper", {}).get("genres")) for item in events
        ),
        "persistedWithCanonicalFilmGenres": sum(
            bool(item.get("persistedCanonicalFilmGenres")) for item in events
        ),
        "coverageComplete": len(events) == movie_total,
        "etypesPages": etypes_pages,
    }
    report["summary"] = counts
    report["generatedAt"] = utc_iso()
    if counts["failed"] > 0:
        report["result"] = "FAIL"
        report["diagnosis"] = "pipeline-mismatch"
    elif events and counts["liveWithGenres"] == counts["rawWithGenres"]:
        report["result"] = "PASS"
        if counts["liveWithGenres"] < counts["inspected"]:
            report["diagnosis"] = (
                "tvscraper-data-sparse-complete"
                if counts["coverageComplete"]
                else "tvscraper-data-sparse-sample"
            )
        else:
            report["diagnosis"] = (
                "genres-present-and-preserved-complete"
                if counts["coverageComplete"]
                else "genres-present-and-preserved-sample"
            )
    else:
        report["result"] = "INCOMPLETE"
        report["diagnosis"] = "no-events-inspected"
    return report


def run_comparison(args):
    tvscraper_database = detect_tvscraper_database(args.tvscraper_database)
    resume = load_resume(args.json_output) if args.resume else None

    if resume:
        require(
            resume.get("backend") == args.backend,
            "Resume-Datei gehört zu anderem Backend",
        )
        from_epoch = int(resume["window"]["from"])
        until_epoch = int(resume["window"]["until"])
        report = resume
    else:
        from_epoch = args.from_epoch or int(time.time())
        until_epoch = args.until_epoch or (
            from_epoch + args.window_hours * 60 * 60
        )
        require(
            until_epoch > from_epoch,
            "Zeitfenster ist leer oder rückwärts",
        )
        require(
            until_epoch - from_epoch <= 72 * 60 * 60,
            "ETYPES erlaubt höchstens 72 Stunden",
        )
        report = make_report(
            args, from_epoch, until_epoch, tvscraper_database
        )

    client = SvdrpClient(
        args.svdrp_host,
        args.svdrp_port,
        args.svdrp_timeout,
    )

    print("Phase-61-Live-/TVScraper-Vergleich")
    print("==================================")
    print(f"Backend: {args.backend}")
    print(f"Fenster: {from_epoch} .. {until_epoch}")
    print(f"Suite-DB: {args.database}")
    print(f"TVScraper-DB: {tvscraper_database}")

    etypes, etypes_pages = fetch_etypes(client, from_epoch, until_epoch)

    with open_read_only_database(args.database) as suite_db:
        candidates = select_movie_candidates(
            suite_db, args.backend, from_epoch, until_epoch
        )
        require(candidates, "Keine Filmkandidaten im Vergleichsfenster")

        existing_keys = {
            item.get("key")
            for item in report.get("events", [])
            if isinstance(item, dict)
        }
        remaining = [
            candidate
            for candidate in candidates
            if (
                f"{candidate['backend_id']}|{candidate['channel_id']}|"
                f"{candidate['event_id']}|{candidate['start_time']}"
            )
            not in existing_keys
        ]
        if args.offset:
            remaining = remaining[args.offset :]
        if args.limit > 0:
            wanted_total = args.limit
            remaining_slots = max(0, wanted_total - len(report["events"]))
            remaining = remaining[:remaining_slots]

        with open_read_only_database(tvscraper_database) as scraper_db:
            validate_tvscraper_schema(scraper_db)
            for index, candidate in enumerate(remaining, start=1):
                channel_id = str(candidate["channel_id"])
                event_id = str(candidate["event_id"])
                start_time = int(candidate["start_time"])
                end_time = int(candidate["end_time"])
                key = (
                    f"{candidate['backend_id']}|{channel_id}|"
                    f"{event_id}|{start_time}"
                )

                comparison_code, comparison_payload = client.execute(
                    f"PLUG suitebridge MCOMPARE {channel_id} {event_id}"
                )
                comparison = parse_json_reply(
                    "MCOMPARE", comparison_code, comparison_payload
                )
                require(
                    comparison.get("schema") == 1,
                    "MCOMPARE-Schema ist unbekannt",
                )
                require(
                    comparison.get("eventAvailable") is True,
                    f"MCOMPARE findet Ereignis nicht: {channel_id}/{event_id}",
                )

                meta_code, meta_payload = client.execute(
                    f"PLUG suitebridge META {channel_id} {event_id}"
                )
                meta_json = parse_json_reply("META", meta_code, meta_payload)

                comparison_identity = normalized_identity(comparison)
                live = normalized_metadata(comparison.get("live"))
                detached = normalized_metadata(comparison.get("detached"))
                meta = normalized_metadata(meta_json)
                raw = resolve_raw_tvscraper(scraper_db, channel_id, event_id)
                persisted = evidence_for(
                    suite_db, candidate["metadata_target_id"]
                )
                etypes_key = (channel_id, event_id, start_time, end_time)

                record = {
                    "key": key,
                    "backend": candidate["backend_id"],
                    "channelId": channel_id,
                    "eventId": event_id,
                    "title": candidate["title"],
                    "subtitle": candidate["subtitle"],
                    "startTime": start_time,
                    "endTime": end_time,
                    "etypes": etypes.get(etypes_key, "none"),
                    "comparisonIdentity": comparison_identity,
                    "live": live,
                    "detached": detached,
                    "meta": meta,
                    "rawTvscraper": raw,
                    "persistedEvidence": persisted,
                }
                evaluate_record(record)
                report["events"].append(record)
                update_summary(report, len(candidates), etypes_pages)
                if args.json_output:
                    atomic_write_json(args.json_output, report)

                genres_text = ", ".join(live["genres"]) or "-"
                print(
                    f"[{len(report['events'])}/{args.limit or len(candidates)}] "
                    f"{record['result']} | {channel_id}/{event_id} | "
                    f"{record['title']} | Live={live['mediaType']} | "
                    f"Genres={genres_text}"
                )

    update_summary(report, len(candidates), etypes_pages)
    if args.json_output:
        atomic_write_json(args.json_output, report)

    summary = report["summary"]
    print()
    print("Zusammenfassung")
    print("===============")
    print(f"Filmkandidaten: {summary['movieCandidates']}")
    print(f"Untersucht: {summary['inspected']}")
    print(f"Live mit Genres: {summary['liveWithGenres']}")
    print(f"Detached META mit Genres: {summary['detachedWithGenres']}")
    print(f"Direktes META mit Genres: {summary['metaWithGenres']}")
    print(f"Rohe TVScraper-Daten mit Genres: {summary['rawWithGenres']}")
    print(
        "Persistierte kanonische Filmgenres: "
        f"{summary['persistedWithCanonicalFilmGenres']}"
    )
    print(f"Abweichungen: {summary['failed']}")
    print(
        "Vollständige Filmabdeckung: "
        + ("ja" if summary["coverageComplete"] else "nein")
    )
    print(f"DIAGNOSIS: {report['diagnosis']}")
    print(f"RESULT: {report['result']}")
    if args.json_output:
        print(f"JSON: {Path(args.json_output).resolve()}")

    return 0 if report["result"] == "PASS" else 2


def run_self_test():
    page = parse_etypes_payload(
        "1|64|64|0|C-1-2-3,7,100,200,M;C-1-2-4,8,120,220,S"
    )
    require(page["nextOffset"] == 64, "Self-Test ETYPES nextOffset")
    require(page["items"][0]["mediaType"] == "movie", "Self-Test ETYPES movie")
    require(
        split_tvscraper_list("|Drama|Comedy|Drama|") == ["Drama", "Comedy"],
        "Self-Test TVScraper-Listensemantik",
    )

    with tempfile.TemporaryDirectory() as directory:
        database = Path(directory) / "tvscraper2.db"
        connection = sqlite3.connect(database)
        connection.executescript(
            """
            CREATE TABLE event(
                event_id INTEGER,
                channel_id TEXT,
                movie_tv_id INTEGER,
                season_number INTEGER,
                episode_number INTEGER,
                runtime INTEGER
            );
            CREATE TABLE movies3(movie_id INTEGER,movie_genres TEXT);
            CREATE TABLE tv2(tv_id INTEGER,tv_genres TEXT);
            INSERT INTO event VALUES(7,'C-1-2-3',42,-100,0,95);
            INSERT INTO movies3 VALUES(42,'Drama|Comedy|Drama');
            """
        )
        connection.commit()
        connection.close()
        with open_read_only_database(database) as read:
            validate_tvscraper_schema(read)
            raw = resolve_raw_tvscraper(read, "C-1-2-3", "7")
        require(raw["mediaType"] == "movie", "Self-Test rohe Filmart")
        require(raw["genres"] == ["Drama", "Comedy"], "Self-Test rohe Genres")

    with tempfile.TemporaryDirectory() as directory:
        database = Path(directory) / "suite.db"
        connection = sqlite3.connect(database)
        connection.executescript(
            """
            CREATE TABLE suite_metadata_target_bindings(
                metadata_target_id TEXT, target_type TEXT, backend_id TEXT,
                resource_key TEXT, native_id TEXT, channel_id TEXT,
                start_time INTEGER, end_time INTEGER, lifecycle_state TEXT
            );
            CREATE TABLE epg_events(
                backend_id TEXT, channel_id TEXT, event_id TEXT, title TEXT,
                subtitle TEXT, description TEXT, start_time TEXT, end_time TEXT
            );
            CREATE TABLE suite_metadata_genre_assignments(
                metadata_target_id TEXT, backend_id TEXT, target_type TEXT,
                genre_id TEXT, provider_id TEXT, original_value TEXT,
                source_kind TEXT, assignment_state TEXT, confidence REAL,
                ordinal INTEGER, observed_at INTEGER
            );
            INSERT INTO suite_metadata_target_bindings VALUES(
                'mdtgt_test','program-event','default','C-1-2-3\n7',
                '7','C-1-2-3',100,200,'active'
            );
            INSERT INTO epg_events VALUES(
                'default','C-1-2-3','7','Test','','','100','200'
            );
            INSERT INTO suite_metadata_genre_assignments VALUES(
                'mdtgt_test','default','program-event','movie',
                'suite-epg-browse','movie','epg-browse-content-class',
                'active',0.99,0,1
            );
            INSERT INTO suite_metadata_genre_assignments VALUES(
                'mdtgt_test','default','program-event','movie',
                'tvscraper-media-type','movie','scraper-media-type',
                'active',0.99,0,1
            );
            INSERT INTO suite_metadata_genre_assignments VALUES(
                'mdtgt_test','default','program-event','drama',
                'tvscraper','Drama','scraper-metadata',
                'active',0.95,0,1
            );
            """
        )
        connection.commit()
        connection.close()
        with open_read_only_database(database) as read:
            candidates = select_movie_candidates(read, "default", 90, 210)
            require(len(candidates) == 1, "Self-Test Filmkandidat")
            persisted = evidence_for(read, "mdtgt_test")
            require(len(persisted) == 3, "Self-Test persistierte Evidenz")

    metadata = {
        "found": True,
        "provider": "tvscraper",
        "providerId": 42,
        "mediaType": "movie",
        "genres": ["Drama"],
    }
    record = {
        "live": dict(metadata),
        "detached": dict(metadata),
        "meta": dict(metadata),
        "rawTvscraper": dict(metadata, reason=""),
        "etypes": "movie",
        "channelId": "C-1-2-3",
        "eventId": "7",
        "startTime": 100,
        "endTime": 200,
        "comparisonIdentity": {
            "channelId": "C-1-2-3",
            "eventId": 7,
            "startTime": 100,
            "endTime": 200,
            "title": "Test",
        },
        "persistedEvidence": [
            {
                "provider": "tvscraper",
                "sourceKind": "scraper-metadata",
                "genreId": "drama",
                "originalValue": "Drama",
                "state": "active",
                "confidence": 0.95,
                "ordinal": 0,
                "observedAt": 1,
            },
            {
                "provider": "tvscraper-media-type",
                "sourceKind": "scraper-media-type",
                "genreId": "movie",
                "originalValue": "movie",
                "state": "active",
                "confidence": 0.99,
                "ordinal": 0,
                "observedAt": 1,
            },
            {
                "provider": "suite-epg-browse",
                "sourceKind": "epg-browse-content-class",
                "genreId": "movie",
                "originalValue": "movie",
                "state": "active",
                "confidence": 0.99,
                "ordinal": 0,
                "observedAt": 1,
            },
        ],
    }
    evaluate_record(record)
    require(record["result"] == "PASS", "Self-Test Vergleich")
    report = {"events": [record], "summary": {}, "result": "RUNNING"}
    update_summary(report, 1, 1)
    require(report["summary"]["coverageComplete"], "Self-Test Abdeckung")
    require(
        report["diagnosis"] == "genres-present-and-preserved-complete",
        "Self-Test Gesamtdiagnose",
    )
    print("phase61 live/tvscraper comparison tool self-test ok")
    return 0


def parse_args(argv):
    parser = argparse.ArgumentParser(
        description=(
            "Compare Live-equivalent real-event TVScraper resolution, the "
            "detached SuiteBridge META path, direct META, raw TVScraper 1.2.15 "
            "SQLite data and persisted genre/browse evidence."
        )
    )
    parser.add_argument("--run", action="store_true")
    parser.add_argument("--self-test", action="store_true")
    parser.add_argument(
        "--database",
        default=os.environ.get("VDR_SUITE_DATABASE", DEFAULT_SUITE_DATABASE),
    )
    parser.add_argument(
        "--tvscraper-database",
        default=os.environ.get("VDR_TVSCRAPER_DATABASE", ""),
    )
    parser.add_argument(
        "--backend",
        default=os.environ.get("VDR_SUITE_BACKEND", DEFAULT_BACKEND),
    )
    parser.add_argument(
        "--svdrp-host",
        default=os.environ.get("VDR_SUITE_SVDRP_HOST", DEFAULT_SVDRP_HOST),
    )
    parser.add_argument(
        "--svdrp-port",
        type=int,
        default=int(
            os.environ.get("VDR_SUITE_SVDRP_PORT", str(DEFAULT_SVDRP_PORT))
        ),
    )
    parser.add_argument("--svdrp-timeout", type=float, default=15.0)
    parser.add_argument("--from-epoch", type=int, default=0)
    parser.add_argument("--until-epoch", type=int, default=0)
    parser.add_argument("--window-hours", type=int, default=DEFAULT_WINDOW_HOURS)
    parser.add_argument(
        "--limit",
        type=int,
        default=DEFAULT_LIMIT,
        help="Maximum total events; 0 means every movie in the window.",
    )
    parser.add_argument("--offset", type=int, default=0)
    parser.add_argument("--json-output", default="")
    parser.add_argument(
        "--resume",
        action="store_true",
        help="Resume from an existing --json-output file.",
    )
    args = parser.parse_args(argv)
    require(args.limit >= 0, "--limit darf nicht negativ sein")
    require(args.offset >= 0, "--offset darf nicht negativ sein")
    require(1 <= args.window_hours <= 72, "--window-hours muss 1..72 sein")
    require(1 <= args.svdrp_port <= 65535, "SVDRP-Port ist ungültig")
    require(args.svdrp_timeout > 0, "SVDRP-Timeout ist ungültig")
    if args.resume:
        require(args.json_output, "--resume benötigt --json-output")
    return args


def main(argv=None):
    try:
        args = parse_args(argv or sys.argv[1:])
        if args.self_test:
            return run_self_test()
        if not args.run:
            print("Use --run for the live comparison or --self-test locally.")
            return 2
        return run_comparison(args)
    except ComparisonFailure as error:
        print()
        print(f"RESULT: FAIL - {error}")
        return 2
    except KeyboardInterrupt:
        print("\nRESULT: ABORTED")
        return 130


if __name__ == "__main__":
    raise SystemExit(main())
