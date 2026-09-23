#!/usr/bin/env python3
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]

ROUTE_SOURCE_FILES = (
    "api/rest/include/ApiRouter.h",
    "api/rest/src/ApiRouter.cpp",
    "api/rest/src/RecordingCutApiRuntime.cpp",
    "api/rest/src/RecordingMarksApiRuntime.cpp",
    "api/rest/src/LegacyOsdApiRuntime.cpp",
    "api/rest/src/PublicApiRuntime.cpp",
    "api/rest/src/HbbtvApiRuntime.cpp",
    "api/rest/src/TeletextApiRuntime.cpp",
    "api/rest/src/ManualRecordingMetadataApiRuntime.cpp",
    "api/rest/src/RecordingSeriesHierarchyApiRuntime.cpp",
    "api/rest/src/MediaTranscodeSettingsApiRuntime.cpp",
    "api/rest/src/SeriesArtworkSettingsApiRuntime.cpp",
    "api/rest/src/LiveRemoteApiRuntime.cpp",
    "api/rest/src/GlobalSearchApiRuntime.cpp",
    "api/rest/src/GenreBrowserApiRuntime.cpp",
    "api/rest/src/ContinueWatchingApiRuntime.cpp",
)

EXPECTED_RUNTIME_OWNERS = {
    "ContinueWatchingApiRuntime",
    "GenreBrowserApiRuntime",
    "GlobalSearchApiRuntime",
    "HbbtvApiRuntime",
    "LegacyOsdApiRuntime",
    "LiveRemoteApiRuntime",
    "ManualRecordingMetadataApiRuntime",
    "PublicApiRuntime",
    "MediaTranscodeSettingsApiRuntime",
    "RecordingCutApiRuntime",
    "RecordingMarksApiRuntime",
    "RecordingSeriesHierarchyApiRuntime",
    "SeriesArtworkSettingsApiRuntime",
    "TeletextApiRuntime",
}

EXPECTED_ROUTE_LITERALS = {
    "/api/v1",
    "/api/v1/capabilities",
    "/api/backends",
    "/api/backends/",
    "/api/backends/default",
    "/api/dashboard",
    "/api/epg/cache/artwork",
    "/api/epg/cache/metadata",
    "/api/epg/cache/metadata/image",
    "/api/epg/cache/now-next",
    "/api/epg/cache/now-next-artwork",
    "/api/epg/cache/refresh",
    "/api/epg/cache/status",
    "/api/epg/cache/window",
    "/api/epg/channel-window",
    "/api/epg/now-next",
    "/api/epg/search",
    "/api/epg/time-window",
    "/api/epgsearch/native-fuzzy/refresh",
    "/api/epgsearch/native-fuzzy/stale-probes/delete",
    "/api/jobs",
    "/api/media/continue-watching",
    "/api/media/recently-watched",
    "/api/media/sessions",
    "/api/metadata",
    "/api/metadata/genres",
    "/api/metadata/genres/epg",
    "/api/metadata/genres/recordings",
    "/api/persons",
    "/api/recordings",
    "/api/recordings/actions/execute",
    "/api/recordings/actions/preview",
    "/api/recordings/actions/validate",
    "/api/recordings/metadata",
    "/api/recordings/metadata/image",
    "/api/recordings/persons/search",
    "/api/runtime",
    "/api/runtime/diagnostics",
    "/api/runtime/diagnostics/summary",
    "/api/runtime/summary",
    "/api/search",
    "/api/searchtimers",
    "/api/searchtimers/automation/preview",
    "/api/searchtimers/delete",
    "/api/searchtimers/discovery",
    "/api/searchtimers/execute",
    "/api/searchtimers/plan",
    "/api/searchtimers/preview",
    "/api/searchtimers/preview/cache/refresh",
    "/api/searchtimers/real-test",
    "/api/searchtimers/update",
    "/api/searchtimers/validate",
    "/api/vdr",
    "/api/vdr/broadcast/hbbtv/applications",
    "/api/vdr/broadcast/hbbtv/sessions",
    "/api/vdr/broadcast/hbbtv/sessions/close",
    "/api/vdr/broadcast/hbbtv/sessions/input",
    "/api/vdr/broadcast/hbbtv/sessions/media",
    "/api/vdr/broadcast/hbbtv/sessions/presentation",
    "/api/vdr/broadcast/hbbtv/sessions/status",
    "/api/vdr/broadcast/teletext/page",
    "/api/vdr/broadcast/teletext/service",
    "/api/vdr/capabilities",
    "/api/vdr/changes",
    "/api/vdr/channels",
    "/api/vdr/channels/actions/move",
    "/api/vdr/channels/move",
    "/api/vdr/epgsearch/native-fuzzy/refresh",
    "/api/vdr/epgsearch/native-fuzzy/stale-probes/delete",
    "/api/vdr/events",
    "/api/vdr/health",
    "/api/vdr/legacy-osd/controller-leases",
    "/api/vdr/legacy-osd/controller-leases/release",
    "/api/vdr/legacy-osd/controller-leases/renew",
    "/api/vdr/legacy-osd/controller-leases/status",
    "/api/vdr/legacy-osd/input",
    "/api/vdr/legacy-osd/sessions",
    "/api/vdr/legacy-osd/sessions/status",
    "/api/vdr/legacy-osd/viewers",
    "/api/vdr/legacy-osd/viewers/detach",
    "/api/vdr/live",
    "/api/vdr/live/overlay",
    "/api/vdr/overview",
    "/api/vdr/persons",
    "/api/vdr/recordings",
    "/api/vdr/recordings/actions/execute",
    "/api/vdr/recordings/actions/preview",
    "/api/vdr/recordings/actions/validate",
    "/api/vdr/recordings/cache/status",
    "/api/vdr/recordings/cut",
    "/api/vdr/recordings/folder",
    "/api/vdr/recordings/folders",
    "/api/vdr/recordings/marks",
    "/api/vdr/recordings/metadata",
    "/api/vdr/recordings/metadata/image",
    "/api/vdr/recordings/persons/search",
    "/api/vdr/recordings/query",
    "/api/vdr/remote/actions",
    "/api/vdr/search",
    "/api/vdr/searchtimers",
    "/api/vdr/searchtimers/automation/preview",
    "/api/vdr/searchtimers/delete",
    "/api/vdr/searchtimers/discovery",
    "/api/vdr/searchtimers/execute",
    "/api/vdr/searchtimers/plan",
    "/api/vdr/searchtimers/preview",
    "/api/vdr/searchtimers/preview/cache/refresh",
    "/api/vdr/searchtimers/real-test",
    "/api/vdr/searchtimers/update",
    "/api/vdr/searchtimers/validate",
    "/api/vdr/snapshot",
    "/api/vdr/snapshots",
    "/api/vdr/status",
    "/api/vdr/timer-conflicts/live",
    "/api/vdr/timers",
    "/api/vdr/timers/actions/create",
    "/api/vdr/timers/actions/delete",
    "/api/vdr/timers/actions/update",
    "/api/vdr/timers/conflicts/live",
    "/api/vdr/timers/live",
}

EXPECTED_PUBLIC_V1_ROUTE_LITERALS = {
    "/api/v1",
    "/api/v1/capabilities",
}

DYNAMIC_ROUTE_MARKERS = {
    "api/rest/src/ManualRecordingMetadataApiRuntime.cpp": (
        '"/recordings/metadata/"',
        'route.operation == "manual"',
        'route.operation == "trailer"',
        'route.operation == "search"',
        'route.operation == "seasons"',
        'route.operation == "episodes"',
        'route.operation == "assign"',
        'route.operation == "withdraw"',
    ),
    "api/rest/src/RecordingSeriesHierarchyApiRuntime.cpp": (
        '"/recordings/series-hierarchy"',
    ),
    "api/rest/src/MediaTranscodeSettingsApiRuntime.cpp": (
        '"/settings/media-transcode"',
    ),
    "api/rest/src/SeriesArtworkSettingsApiRuntime.cpp": (
        '"/settings/series-artwork"',
    ),
}

STRING_LITERAL = re.compile(r'"((?:\\.|[^"\\])*)"')
RUNTIME_OWNER = re.compile(r'([A-Za-z0-9_]+ApiRuntime)::instance\(\)\.tryHandle')


def read(relative):
    return (ROOT / relative).read_text(encoding="utf-8")


def route_literals(text):
    routes = set()
    for match in STRING_LITERAL.finditer(text):
        value = match.group(1).replace(r'\"', '"')
        if value.startswith("/api/"):
            routes.add(value)
    return routes


def main():
    errors = []
    observed = set()

    for relative in ROUTE_SOURCE_FILES:
        path = ROOT / relative
        if not path.is_file():
            errors.append(f"missing inventoried route owner: {relative}")
            continue
        observed.update(route_literals(read(relative)))

    missing = sorted(EXPECTED_ROUTE_LITERALS - observed)
    added = sorted(observed - EXPECTED_ROUTE_LITERALS)
    if missing:
        errors.append("inventoried route literals disappeared: " + ", ".join(missing))
    if added:
        errors.append("unclassified route literals appeared: " + ", ".join(added))

    router_text = read("api/rest/include/ApiRouter.h") + "\n" + read("api/rest/src/ApiRouter.cpp")
    owners = set(RUNTIME_OWNER.findall(router_text))
    missing_owners = sorted(EXPECTED_RUNTIME_OWNERS - owners)
    added_owners = sorted(owners - EXPECTED_RUNTIME_OWNERS)
    if missing_owners:
        errors.append("inventoried delegated API runtimes disappeared: " + ", ".join(missing_owners))
    if added_owners:
        errors.append("unclassified delegated API runtimes appeared: " + ", ".join(added_owners))

    for relative, markers in DYNAMIC_ROUTE_MARKERS.items():
        source = read(relative)
        for marker in markers:
            if marker not in source:
                errors.append(f"{relative} misses dynamic-route marker: {marker}")

    v1 = {route for route in observed if route.startswith("/api/v1")}
    missing_v1 = sorted(EXPECTED_PUBLIC_V1_ROUTE_LITERALS - v1)
    added_v1 = sorted(v1 - EXPECTED_PUBLIC_V1_ROUTE_LITERALS)
    if missing_v1:
        errors.append("inventoried public v1 routes disappeared: " + ", ".join(missing_v1))
    if added_v1:
        errors.append("unclassified public v1 routes appeared: " + ", ".join(added_v1))

    if errors:
        print("Phase 69 public API inventory check failed:")
        for error in errors:
            print("- " + error)
        return 1

    print("Phase 69 public API inventory check passed.")
    print(f"Inventoried route literals: {len(observed)}")
    print(f"Delegated API runtime owners: {len(owners)}")
    print(f"Public /api/v1 route literals: {len(EXPECTED_PUBLIC_V1_ROUTE_LITERALS)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
