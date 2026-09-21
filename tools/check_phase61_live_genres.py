#!/usr/bin/env python3

import argparse
import base64
import datetime as dt
import json
import os
import sqlite3
import subprocess
import sys
import time
import urllib.error
import urllib.parse
import urllib.request


DEFAULT_BASE_URL = "http://127.0.0.1:18080"
DEFAULT_DATABASE = "/var/lib/vdr-suite/vdr-suite.db"
DEFAULT_BACKEND = "default"
DEFAULT_SERVICE = "vdr-suite-daemon.service"
DEFAULT_VDR_SERVICE = "vdr.service"
FILM_GENRES = (
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
)


class AcceptanceFailure(RuntimeError):
    pass


def now_iso():
    return dt.datetime.now(dt.timezone.utc).astimezone().isoformat(timespec="seconds")


def run_command(command):
    return subprocess.run(
        command,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        check=False,
    )


def require(condition, message):
    if not condition:
        raise AcceptanceFailure(message)


def service_state(service):
    result = run_command(["systemctl", "is-active", service])
    return result.stdout.strip()


def require_active(service):
    state = service_state(service)
    require(state == "active", f"{service} ist nicht aktiv (state={state or 'unknown'})")


def ensure_daemon_active(service):
    if service_state(service) == "active":
        return
    result = run_command(["systemctl", "start", service])
    require(result.returncode == 0, f"{service} konnte nicht gestartet werden: {result.stdout.strip()}")


def authorization_header(username, password):
    if not username and not password:
        return ""
    token = base64.b64encode(f"{username}:{password}".encode("utf-8"))
    return "Basic " + token.decode("ascii")


def get_json(base_url, path, query, username, password, timeout_seconds):
    url = base_url.rstrip("/") + path
    if query:
        url += "?" + urllib.parse.urlencode(query)
    headers = {"Accept": "application/json"}
    authorization = authorization_header(username, password)
    if authorization:
        headers["Authorization"] = authorization
    request = urllib.request.Request(url, method="GET", headers=headers)
    try:
        with urllib.request.urlopen(request, timeout=timeout_seconds) as response:
            body = response.read().decode("utf-8", errors="replace")
            return response.status, json.loads(body)
    except urllib.error.HTTPError as error:
        body = error.read().decode("utf-8", errors="replace")
        raise AcceptanceFailure(
            f"HTTP {error.code} für {path}: {body[:500]}"
        ) from error
    except (urllib.error.URLError, TimeoutError, ConnectionError, json.JSONDecodeError) as error:
        raise OSError(str(error)) from error


def epg_overview_query(backend, from_epoch, until_epoch):
    return {
        "backend": backend,
        "scope": "epg",
        "locale": "de",
        "from": str(from_epoch),
        "until": str(until_epoch),
    }


def movie_summary(data):
    categories = data.get("categories", [])
    movie = next((item for item in categories if item.get("id") == "movie"), None)
    require(movie is not None, "EPG-Übersicht enthält keine Film-Hauptklasse")
    children = [
        item
        for item in movie.get("children", [])
        if int(item.get("count", 0)) > 0
    ]
    children.sort(
        key=lambda item: (
            -int(item.get("count", 0)),
            str(item.get("label", item.get("id", ""))),
        )
    )
    return int(movie.get("count", 0)), children


def wait_for_api(args, from_epoch, until_epoch):
    deadline = time.monotonic() + args.startup_timeout
    last_notice = 0.0
    last_error = ""
    while time.monotonic() < deadline:
        require_active(args.service)
        try:
            status, data = get_json(
                args.base_url,
                "/api/metadata/genres",
                epg_overview_query(args.backend, from_epoch, until_epoch),
                args.username,
                args.password,
                args.http_timeout,
            )
            require(status == 200, f"Genre-API antwortet mit HTTP {status}")
            return data
        except OSError as error:
            last_error = str(error)
        now = time.monotonic()
        if now - last_notice >= 10:
            print("Warte auf Genre-API ...")
            last_notice = now
        time.sleep(1)
    raise AcceptanceFailure(
        f"Genre-API wurde innerhalb von {args.startup_timeout}s nicht bereit"
        + (f": {last_error}" if last_error else "")
    )


def open_database(path):
    require(os.path.isfile(path), f"Datenbank fehlt: {path}")
    connection = sqlite3.connect(path, timeout=10)
    connection.row_factory = sqlite3.Row
    return connection


def count_tvscraper_targets(connection, backend, from_epoch, until_epoch):
    row = connection.execute(
        """
        SELECT COUNT(DISTINCT b.metadata_target_id) AS count
          FROM suite_metadata_target_bindings b
          JOIN suite_metadata_genre_assignments a
            ON a.metadata_target_id=b.metadata_target_id
         WHERE b.backend_id=?
           AND b.target_type='program-event'
           AND b.lifecycle_state='active'
           AND CAST(b.end_time AS INTEGER)>?
           AND CAST(b.start_time AS INTEGER)<?
           AND a.provider_id='tvscraper'
           AND a.source_kind='scraper-metadata'
        """,
        (backend, from_epoch, until_epoch),
    ).fetchone()
    return int(row["count"])


def count_movies_with_children(connection, backend, from_epoch, until_epoch):
    placeholders = ",".join("?" for _ in FILM_GENRES)
    row = connection.execute(
        f"""
        SELECT COUNT(DISTINCT b.metadata_target_id) AS count
          FROM suite_metadata_genre_assignments c
          JOIN suite_metadata_target_bindings b
            ON b.metadata_target_id=c.metadata_target_id
          JOIN suite_metadata_genre_assignments a
            ON a.metadata_target_id=b.metadata_target_id
         WHERE b.backend_id=?
           AND b.target_type='program-event'
           AND b.lifecycle_state='active'
           AND CAST(b.end_time AS INTEGER)>?
           AND CAST(b.start_time AS INTEGER)<?
           AND c.source_kind='epg-browse-content-class'
           AND c.assignment_state='active'
           AND c.genre_id='movie'
           AND a.provider_id='tvscraper'
           AND a.source_kind='scraper-metadata'
           AND a.assignment_state IN('active','unknown','conflict')
           AND a.genre_id IN({placeholders})
        """,
        (backend, from_epoch, until_epoch, *FILM_GENRES),
    ).fetchone()
    return int(row["count"])


def print_children(movie_count, children):
    print(f"Film: {movie_count}")
    print(f"Film-Untergenres: {len(children)}")
    for item in children:
        print(
            f"  {item.get('id')} | "
            f"{item.get('label', item.get('id'))} | "
            f"{int(item.get('count', 0))}"
        )


def wait_for_children(args, from_epoch, until_epoch):
    deadline = time.monotonic() + args.enrichment_timeout
    last_state = None
    last_notice = 0.0
    while time.monotonic() < deadline:
        require_active(args.service)
        try:
            status, data = get_json(
                args.base_url,
                "/api/metadata/genres",
                epg_overview_query(args.backend, from_epoch, until_epoch),
                args.username,
                args.password,
                args.http_timeout,
            )
            require(status == 200, f"Genre-API antwortet mit HTTP {status}")
            movie_count, children = movie_summary(data)
            state = (movie_count, len(children))
            now = time.monotonic()
            if state != last_state or now - last_notice >= 30:
                print(
                    f"Anreicherung: Film={movie_count}, "
                    f"Untergenres={len(children)}"
                )
                last_state = state
                last_notice = now
            if movie_count > 0 and children:
                return data, movie_count, children
        except OSError:
            pass
        time.sleep(args.poll_interval)
    raise AcceptanceFailure(
        f"Nach {args.enrichment_timeout}s sind weiterhin keine Film-Untergenres vorhanden"
    )


def validate_genre_results(args, genre_id, from_epoch, until_epoch):
    status, data = get_json(
        args.base_url,
        "/api/metadata/genres/epg",
        {
            "backend": args.backend,
            "contentClass": "movie",
            "genre": genre_id,
            "from": str(from_epoch),
            "until": str(until_epoch),
            "limit": "12",
            "offset": "0",
        },
        args.username,
        args.password,
        args.http_timeout,
    )
    require(status == 200, f"Filmgenre-Ergebnisse antworten mit HTTP {status}")
    require(data.get("backendId") == args.backend, "falsches Backend in Filmgenre-Antwort")
    require(data.get("contentClass") == "movie", "falsche Content-Class")
    require(data.get("genreId") == genre_id, "falsche Genre-ID")
    require(int(data.get("total", 0)) > 0, "Film-Untergenre hat keine Ergebnisse")
    items = data.get("items", [])
    require(items, "Film-Untergenre liefert keine Items")
    for item in items:
        require(item.get("backendId") == args.backend, "Item verletzt Backend-Isolation")
        require(item.get("contentClass") == "movie", "Item verletzt Filmklasse")
        require(genre_id in item.get("genreIds", []), "Item enthält Genre nicht")
    print(
        f"Ergebnisse: {genre_id}, total={data.get('total')}, "
        f"geliefert={len(items)}"
    )


def restart_and_verify(args, genre_id, from_epoch, until_epoch):
    started = time.monotonic()
    stop = run_command(["systemctl", "stop", args.service])
    require(stop.returncode == 0, f"Stop fehlgeschlagen: {stop.stdout.strip()}")
    duration_ms = int((time.monotonic() - started) * 1000)
    require(duration_ms < 15000, f"Stop dauerte {duration_ms}ms")
    require(service_state(args.service) == "inactive", "Daemon nach Stop nicht inactive")

    start = run_command(["systemctl", "start", args.service])
    require(start.returncode == 0, f"Start fehlgeschlagen: {start.stdout.strip()}")
    data = wait_for_api(args, from_epoch, until_epoch)
    movie_count, children = movie_summary(data)
    child = next((item for item in children if item.get("id") == genre_id), None)
    require(child is not None, f"{genre_id} fehlt nach Neustart")
    require(int(child.get("count", 0)) > 0, f"{genre_id} ist nach Neustart leer")
    print(
        f"Neustart-Persistenz: Stop={duration_ms}ms, "
        f"Film={movie_count}, {genre_id}={child.get('count')}"
    )


def print_diagnostics(args, since):
    print()
    print("Diagnose")
    print("========")
    status = run_command(
        ["systemctl", "--no-pager", "--full", "status", args.service]
    )
    print(status.stdout[-8000:])
    journal = run_command(
        [
            "journalctl",
            "-u",
            args.service,
            "-u",
            args.vdr_service,
            "--since",
            since,
            "--no-pager",
            "-n",
            "300",
        ]
    )
    print(journal.stdout[-24000:])
    try:
        with open_database(args.database) as connection:
            rows = connection.execute(
                """
                SELECT provider_id,source_kind,assignment_state,
                       COUNT(*) AS assignments,
                       COUNT(DISTINCT metadata_target_id) AS targets
                  FROM suite_metadata_genre_assignments
                 WHERE backend_id=?
                   AND target_type='program-event'
                 GROUP BY provider_id,source_kind,assignment_state
                 ORDER BY provider_id,source_kind,assignment_state
                """,
                (args.backend,),
            ).fetchall()
            print("Persistierte Evidenz:")
            for row in rows:
                print(
                    f"  {row['provider_id']} | {row['source_kind']} | "
                    f"{row['assignment_state']} | "
                    f"assignments={row['assignments']} targets={row['targets']}"
                )
    except Exception as error:
        print(f"Datenbankdiagnose fehlgeschlagen: {error}")


def run_acceptance(args):
    require_active(args.vdr_service)
    ensure_daemon_active(args.service)

    from_epoch = int(time.time())
    until_epoch = from_epoch + 48 * 60 * 60

    initial = wait_for_api(args, from_epoch, until_epoch)
    movie_count, children = movie_summary(initial)

    with open_database(args.database) as connection:
        before_targets = count_tvscraper_targets(
            connection, args.backend, from_epoch, until_epoch
        )
        before_movies = count_movies_with_children(
            connection, args.backend, from_epoch, until_epoch
        )

    print("Phase-61-Live-Abnahme")
    print("=====================")
    print(f"Backend: {args.backend}")
    print(f"Film vorher: {movie_count}")
    print(f"Untergenres vorher: {len(children)}")
    print(f"TVScraper-META-Ziele vorher: {before_targets}")
    print(f"Filme mit Untergenre-Evidenz vorher: {before_movies}")

    _, movie_count, children = wait_for_children(
        args, from_epoch, until_epoch
    )
    print_children(movie_count, children)

    selected = children[0]
    genre_id = selected["id"]
    validate_genre_results(args, genre_id, from_epoch, until_epoch)

    with open_database(args.database) as connection:
        after_targets = count_tvscraper_targets(
            connection, args.backend, from_epoch, until_epoch
        )
        after_movies = count_movies_with_children(
            connection, args.backend, from_epoch, until_epoch
        )

    require(after_movies > 0, "keine persistierten Filme mit Untergenre-Evidenz")
    print(f"TVScraper-META-Ziele nachher: {after_targets}")
    print(f"Filme mit Untergenre-Evidenz nachher: {after_movies}")

    if args.restart_check:
        restart_and_verify(args, genre_id, from_epoch, until_epoch)

    require_active(args.vdr_service)
    require_active(args.service)
    print()
    print("RESULT: OK")
    return 0


def run_self_test():
    sample = {
        "categories": [
            {
                "id": "movie",
                "count": 4,
                "children": [
                    {"id": "drama", "label": "Drama", "count": 1},
                    {"id": "thriller", "label": "Thriller", "count": 3},
                    {"id": "western", "label": "Western", "count": 0},
                ],
            },
            {"id": "series", "count": 1, "children": []},
            {"id": "documentary", "count": 1, "children": []},
            {"id": "sports", "count": 1, "children": []},
        ]
    }
    movie_count, children = movie_summary(sample)
    require(movie_count == 4, "Self-Test: Filmzahl")
    require([item["id"] for item in children] == ["thriller", "drama"], "Self-Test: Sortierung")
    require(authorization_header("admin", "secret").startswith("Basic "), "Self-Test: Auth")
    print("phase61 live genre acceptance tool self-test ok")
    return 0


def parse_args(argv):
    parser = argparse.ArgumentParser(
        description=(
            "Resume and verify the Phase 61 live EPG film-genre enrichment "
            "without reinstalling the daemon."
        )
    )
    parser.add_argument("--run", action="store_true", help="Run against the live system.")
    parser.add_argument("--self-test", action="store_true", help="Run local parser checks.")
    parser.add_argument(
        "--restart-check",
        action="store_true",
        help="Restart the daemon once and verify SQLite persistence.",
    )
    parser.add_argument(
        "--base-url",
        default=os.environ.get("VDR_SUITE_API_BASE_URL", DEFAULT_BASE_URL),
    )
    parser.add_argument(
        "--database",
        default=os.environ.get("VDR_SUITE_DATABASE", DEFAULT_DATABASE),
    )
    parser.add_argument(
        "--backend",
        default=os.environ.get("VDR_SUITE_BACKEND", DEFAULT_BACKEND),
    )
    parser.add_argument(
        "--username",
        default=os.environ.get("VDR_SUITE_API_USER", "admin"),
    )
    parser.add_argument(
        "--password",
        default=os.environ.get("VDR_SUITE_API_PASSWORD", "vdr-suite"),
    )
    parser.add_argument("--service", default=DEFAULT_SERVICE)
    parser.add_argument("--vdr-service", default=DEFAULT_VDR_SERVICE)
    parser.add_argument("--startup-timeout", type=int, default=180)
    parser.add_argument("--enrichment-timeout", type=int, default=1200)
    parser.add_argument("--poll-interval", type=float, default=5.0)
    parser.add_argument("--http-timeout", type=int, default=10)
    return parser.parse_args(argv)


def main(argv=None):
    args = parse_args(argv or sys.argv[1:])
    if args.self_test:
        return run_self_test()
    if not args.run:
        print("Use --run for the live check or --self-test locally.")
        return 2

    since = now_iso()
    try:
        return run_acceptance(args)
    except AcceptanceFailure as error:
        print()
        print(f"RESULT: FAIL - {error}")
        print_diagnostics(args, since)
        return 2
    except KeyboardInterrupt:
        print("\nRESULT: ABORTED")
        return 130


if __name__ == "__main__":
    raise SystemExit(main())
