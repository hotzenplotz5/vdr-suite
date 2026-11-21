#!/usr/bin/env python3

from pathlib import Path
import sys


ROOT = Path(__file__).resolve().parents[1]

files = {
    "handler_header": ROOT / "vdr-plugin-suite-bridge" / "suitebridge_epg_command_handler.h",
    "handler": ROOT / "vdr-plugin-suite-bridge" / "suitebridge_epg_command_handler.cpp",
    "svdrp": ROOT / "vdr-plugin-suite-bridge" / "suitebridge_svdrp.cpp",
    "tool": ROOT / "tools" / "compare_phase61_live_tvscraper.py",
}

errors = []
for name, path in files.items():
    if not path.is_file():
        errors.append(f"missing {name}: {path.relative_to(ROOT)}")

if errors:
    for error in errors:
        print(f"ERROR: {error}", file=sys.stderr)
    raise SystemExit(1)

header = files["handler_header"].read_text(encoding="utf-8")
handler = files["handler"].read_text(encoding="utf-8")
svdrp = files["svdrp"].read_text(encoding="utf-8")
tool = files["tool"].read_text(encoding="utf-8")

required = {
    "handler_header": (
        "HandleMetadataComparison(",
    ),
    "handler": (
        'strcasecmp(command, "MCOMPARE")',
        "LOCK_CHANNELS_READ;",
        "cGetScraperVideo request(&event, nullptr);",
        "video.getMovieOrTv(",
        "LOCK_SCHEDULES_READ;",
        "liveMetadata = ResolveComparisonMetadata(*event);",
        "detachedMetadata =",
        "ResolveComparisonMetadata(detachedSnapshot->Event());",
        '"live\\":',
        '"detached\\":',
        "svdrp command=MCOMPARE result=served",
    ),
    "svdrp": (
        '"MCOMPARE <channel-id> <event-id>\\n"',
        "Compare Live-equivalent real-event TVScraper metadata",
        "HandleMetadataComparison(Command, Option)",
    ),
    "tool": (
        "PLUG suitebridge MCOMPARE",
        "PLUG suitebridge ETYPES",
        "PLUG suitebridge META",
        "PRAGMA query_only=ON",
        "SELECT movie_genres FROM movies3",
        "SELECT tv_genres FROM tv2",
        "live-detached-mismatch",
        "meta-persistence-mismatch",
        "event-identity-mismatch",
        "etype-persistence-mismatch",
        "not-found-state-not-persisted",
        "coverageComplete",
        "--resume",
    ),
}

contents = {
    "handler_header": header,
    "handler": handler,
    "svdrp": svdrp,
    "tool": tool,
}
for name, fragments in required.items():
    for fragment in fragments:
        if fragment not in contents[name]:
            errors.append(f"missing {name} contract: {fragment}")

live_position = handler.find("liveMetadata = ResolveComparisonMetadata(*event);")
detached_position = handler.find(
    "ResolveComparisonMetadata(detachedSnapshot->Event());"
)
if live_position < 0 or detached_position < 0 or live_position >= detached_position:
    errors.append("real-event comparison must execute before detached comparison")

lock_position = handler.find("LOCK_SCHEDULES_READ;")
if lock_position < 0 or lock_position >= live_position:
    errors.append("real-event comparison must execute under schedule lock")

for forbidden in (
    "fetch(",
    "urllib.request",
    "requests.",
    "/api/tvscraper",
):
    if forbidden in tool:
        errors.append(f"forbidden comparison-tool provider transport: {forbidden}")

if errors:
    for error in errors:
        print(f"ERROR: {error}", file=sys.stderr)
    raise SystemExit(1)

print("phase61 live/tvscraper comparison contracts ok")
