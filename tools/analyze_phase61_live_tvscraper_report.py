#!/usr/bin/env python3

import argparse
import json
import sys
import tempfile
from collections import Counter
from pathlib import Path


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

# Keep this aligned with GenreIndexRepositoryLiveParity.inc.  Drama alone is
# deliberately not treated as a strong DVB film discriminator there.
SPECIFIC_DVB_FILM_GENRES = FILM_GENRES - {"drama"}
USABLE_STATES = frozenset({"active", "unknown", "conflict"})
SOURCE_FAILURES = frozenset({"etypes-not-movie", "etype-persistence-mismatch"})


class AnalysisFailure(RuntimeError):
    pass


def require(condition, message):
    if not condition:
        raise AnalysisFailure(message)


def load_report(path):
    report_path = Path(path)
    require(report_path.is_file(), f"Vergleichsbericht fehlt: {report_path}")
    try:
        value = json.loads(report_path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as error:
        raise AnalysisFailure(f"Vergleichsbericht ist ungültig: {error}") from error
    require(isinstance(value, dict), "Vergleichsbericht enthält kein Objekt")
    require(isinstance(value.get("events"), list), "Vergleichsbericht enthält keine Ereignisse")
    require(
        value.get("summary", {}).get("coverageComplete") is True,
        "Vergleichsbericht besitzt keine vollständige Filmabdeckung",
    )
    return value


def evidence(record, *, provider=None, source_kind=None, state=None, genre_id=None):
    for item in record.get("persistedEvidence", []):
        if provider is not None and item.get("provider") != provider:
            continue
        if source_kind is not None and item.get("sourceKind") != source_kind:
            continue
        if state is not None and item.get("state") != state:
            continue
        if genre_id is not None and item.get("genreId") != genre_id:
            continue
        yield item


def tvscraper_is_none(record):
    for key in ("live", "detached", "meta", "rawTvscraper"):
        value = record.get(key, {})
        if value.get("found") is not False:
            return False
        if value.get("mediaType") != "none":
            return False
        if value.get("genres"):
            return False
    return record.get("etypes") == "none"


def has_stale_movie_media_type(record):
    return any(
        evidence(
            record,
            provider="tvscraper-media-type",
            source_kind="scraper-media-type",
            state="stale",
            genre_id="movie",
        )
    )


def usable_dvb_genres(record):
    return {
        str(item.get("genreId", ""))
        for item in evidence(
            record,
            provider="vdr-epg",
            source_kind="dvb-content-descriptor",
        )
        if item.get("state") in USABLE_STATES
    }


def source_class(record):
    if record.get("etypes") == "movie":
        return "tvscraper-current"

    if not tvscraper_is_none(record):
        return "tvscraper-inconsistent"

    if has_stale_movie_media_type(record):
        return "tvscraper-last-known"

    genres = usable_dvb_genres(record)
    if "movie" not in genres:
        return "unsupported-movie"
    if genres & SPECIFIC_DVB_FILM_GENRES:
        return "dvb-specific-movie"
    return "dvb-generic-movie"


def adjusted_failures(record, classification):
    failures = list(record.get("failures", []))
    if classification in ("tvscraper-last-known", "dvb-specific-movie"):
        failures = [failure for failure in failures if failure not in SOURCE_FAILURES]
    elif classification == "dvb-generic-movie":
        if "generic-dvb-film-fallback" not in failures:
            failures.append("generic-dvb-film-fallback")
    elif classification == "unsupported-movie":
        if "unsupported-persisted-movie" not in failures:
            failures.append("unsupported-persisted-movie")
    return sorted(set(failures))


def analyze(report):
    classifications = Counter()
    failure_categories = Counter()
    failures = []

    for record in report["events"]:
        classification = source_class(record)
        classifications[classification] += 1
        remaining = adjusted_failures(record, classification)
        for failure in remaining:
            failure_categories[failure] += 1
        if remaining:
            failures.append(
                {
                    "channelId": record.get("channelId", ""),
                    "eventId": record.get("eventId", ""),
                    "title": record.get("title", ""),
                    "classification": classification,
                    "failures": remaining,
                    "dvbGenres": sorted(usable_dvb_genres(record)),
                }
            )

    if not failures:
        result = "PASS"
        diagnosis = "tvscraper-parity-plus-valid-dvb-fallback"
    elif failure_categories.get("generic-dvb-film-fallback", 0) > 0:
        result = "FAIL"
        diagnosis = "generic-dvb-film-fallback-mismatch"
    elif failure_categories.get("not-found-state-not-persisted", 0) > 0:
        result = "FAIL"
        diagnosis = "fallback-metadata-materialization-incomplete"
    else:
        result = "FAIL"
        diagnosis = "pipeline-mismatch"

    return {
        "schema": 1,
        "tool": "analyze_phase61_live_tvscraper_report",
        "sourceReport": report.get("tool", ""),
        "sourceResult": report.get("result", ""),
        "sourceDiagnosis": report.get("diagnosis", ""),
        "eventCount": len(report["events"]),
        "classifications": dict(sorted(classifications.items())),
        "failureCategories": dict(sorted(failure_categories.items())),
        "failures": failures,
        "result": result,
        "diagnosis": diagnosis,
    }


def print_analysis(analysis, maximum_examples):
    print("Phase-61-Quellenanalyse")
    print("========================")
    print(f"Ereignisse: {analysis['eventCount']}")
    print("Quellen:")
    for name, count in analysis["classifications"].items():
        print(f"  {name}: {count}")
    print("Verbleibende Fehlerkategorien:")
    if analysis["failureCategories"]:
        for name, count in analysis["failureCategories"].items():
            print(f"  {name}: {count}")
    else:
        print("  keine")

    examples = analysis["failures"][:maximum_examples]
    if examples:
        print("Beispiele:")
        for item in examples:
            print(
                "  "
                f"{item['channelId']}/{item['eventId']} | "
                f"{item['classification']} | {item['title']} | "
                f"Fehler={','.join(item['failures'])} | "
                f"DVB={','.join(item['dvbGenres']) or '-'}"
            )

    print(f"DIAGNOSIS: {analysis['diagnosis']}")
    print(f"RESULT: {analysis['result']}")


def run_self_test():
    current = {
        "etypes": "movie",
        "live": {"found": True, "mediaType": "movie", "genres": ["Drama"]},
        "detached": {"found": True, "mediaType": "movie", "genres": ["Drama"]},
        "meta": {"found": True, "mediaType": "movie", "genres": ["Drama"]},
        "rawTvscraper": {"found": True, "mediaType": "movie", "genres": ["Drama"]},
        "persistedEvidence": [],
        "failures": [],
    }
    assert source_class(current) == "tvscraper-current"

    fallback = {
        "etypes": "none",
        "live": {"found": False, "mediaType": "none", "genres": []},
        "detached": {"found": False, "mediaType": "none", "genres": []},
        "meta": {"found": False, "mediaType": "none", "genres": []},
        "rawTvscraper": {"found": False, "mediaType": "none", "genres": []},
        "persistedEvidence": [
            {
                "provider": "vdr-epg",
                "sourceKind": "dvb-content-descriptor",
                "state": "active",
                "genreId": "movie",
            },
            {
                "provider": "vdr-epg",
                "sourceKind": "dvb-content-descriptor",
                "state": "active",
                "genreId": "comedy",
            },
        ],
        "failures": ["etypes-not-movie", "etype-persistence-mismatch"],
    }
    assert source_class(fallback) == "dvb-specific-movie"
    assert adjusted_failures(fallback, source_class(fallback)) == []

    generic = dict(fallback)
    generic["persistedEvidence"] = [
        {
            "provider": "vdr-epg",
            "sourceKind": "dvb-content-descriptor",
            "state": "active",
            "genreId": "movie",
        }
    ]
    assert source_class(generic) == "dvb-generic-movie"
    assert "generic-dvb-film-fallback" in adjusted_failures(
        generic, source_class(generic)
    )

    with tempfile.TemporaryDirectory() as directory:
        path = Path(directory) / "report.json"
        report = {
            "tool": "compare_phase61_live_tvscraper",
            "result": "FAIL",
            "diagnosis": "pipeline-mismatch",
            "summary": {"coverageComplete": True},
            "events": [dict(current, channelId="C1", eventId="1", title="Current")],
        }
        path.write_text(json.dumps(report), encoding="utf-8")
        loaded = load_report(path)
        result = analyze(loaded)
        assert result["result"] == "PASS"

    print("phase61 live/tvscraper report analyzer self-test ok")
    return 0


def parse_args(argv):
    parser = argparse.ArgumentParser(
        description=(
            "Classify complete Phase 61 comparison results by current "
            "TVScraper, last-known TVScraper and DVB fallback provenance."
        )
    )
    parser.add_argument("--report")
    parser.add_argument("--json-output", default="")
    parser.add_argument("--examples", type=int, default=20)
    parser.add_argument("--self-test", action="store_true")
    return parser.parse_args(argv)


def main(argv=None):
    args = parse_args(sys.argv[1:] if argv is None else argv)
    if args.self_test:
        return run_self_test()

    try:
        require(args.report, "--report ist erforderlich")
        report = load_report(args.report)
        analysis = analyze(report)
        print_analysis(analysis, max(0, args.examples))
        if args.json_output:
            Path(args.json_output).write_text(
                json.dumps(analysis, ensure_ascii=False, indent=2, sort_keys=True)
                + "\n",
                encoding="utf-8",
            )
        return 0 if analysis["result"] == "PASS" else 2
    except AnalysisFailure as error:
        print(f"RESULT: FAIL - {error}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
