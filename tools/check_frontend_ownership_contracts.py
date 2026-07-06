#!/usr/bin/env python3
"""Validate lightweight web frontend ownership boundaries.

This is a static guard for the current Phase 58 frontend layout. It prevents
helper files from silently becoming feature loaders and keeps the documented
script-order contract visible in tests.
"""

from __future__ import annotations

import re
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
FRONTEND = ROOT / "web" / "frontend"

INDEX = FRONTEND / "index.html"
APP = FRONTEND / "app.js"
CHANNEL_LOGOS = FRONTEND / "channel-logos.js"
CHANNEL_BROWSER = FRONTEND / "channel-browser.js"
STYLE = FRONTEND / "style.css"
ARCH_DOC = ROOT / "docs" / "development" / "frontend-architecture.md"
HTTP_SERVER = ROOT / "core" / "http" / "src" / "TestHttpServer.cpp"


class ContractFailure(Exception):
    pass


def read(path: Path) -> str:
    if not path.exists():
        raise ContractFailure(f"required file missing: {path.relative_to(ROOT)}")
    return path.read_text(encoding="utf-8")


def require(condition: bool, message: str) -> None:
    if not condition:
        raise ContractFailure(message)


def script_positions(index_html: str) -> dict[str, int]:
    scripts = {
        "app": '<script src="/frontend/app.js"></script>',
        "channel_logos": '<script src="/frontend/channel-logos.js"></script>',
        "channel_browser": '<script src="/frontend/channel-browser.js"></script>',
    }

    positions: dict[str, int] = {}
    for name, marker in scripts.items():
        pos = index_html.find(marker)
        require(pos >= 0, f"index.html is missing required script tag: {marker}")
        positions[name] = pos

    return positions


def check_index_contract(index_html: str) -> None:
    positions = script_positions(index_html)

    require(
        positions["app"] < positions["channel_logos"] < positions["channel_browser"],
        "index.html script order must be app.js -> channel-logos.js -> channel-browser.js",
    )

    require(
        'src="/frontend/timer-conflicts.js"' not in index_html,
        "timer-conflicts.js must not be loaded as a separate bootstrap script; integrate Timer conflicts through app.js",
    )

    require(
        not re.search(r"createElement\s*\(\s*['\"]script['\"]\s*\)", index_html),
        "index.html must not create dynamic script loaders",
    )


def check_app_contract(app_js: str) -> None:
    require(
        re.search(r"function\s+renderTimerList\s*\(\s*data(?:\s*,[^)]*)?\)", app_js) is not None,
        "app.js must own renderTimerList(data[, ...])",
    )
    require("function loadTimers()" in app_js, "app.js must own loadTimers()")
    require(
        re.search(r"renderTimerList\s*\(\s*data(?:\s*,[^)]*)?\)\s*;", app_js) is not None,
        "loadTimers() must render the timer list through renderTimerList(data[, ...])",
    )

    if "Timer-Konflikte" in app_js or "Timerkonflikt" in app_js or "loadTimerConflictPanel" in app_js:
        require(
            "function loadTimerConflictPanel" in app_js,
            "Timer conflict UI text exists but loadTimerConflictPanel() is missing",
        )
        require(
            "loadTimerConflictPanel(" in app_js,
            "loadTimers() must call loadTimerConflictPanel() after renderTimerList(data[, ...])",
        )
        require(
            "timer-conflict-" in app_js,
            "Timer conflict UI must use the timer-conflict-* CSS prefix",
        )


def check_timer_loading_client_api_contract(app_js: str) -> None:
    start = app_js.find("function loadTimers() {")
    require(start >= 0, "app.js must define loadTimers()")

    end = app_js.find("function loadSearchTimers() {", start)
    require(end > start, "app.js loadTimers() boundary must end before loadSearchTimers()")

    load_timers_body = app_js[start:end]

    require(
        "window.VdrSuiteClientApi" in load_timers_body,
        "loadTimers() must use window.VdrSuiteClientApi"
    )

    require(
        "fetchClientTimers" in load_timers_body,
        "loadTimers() must use fetchClientTimers()"
    )

    forbidden_tokens = [
        "fetch('/api/vdr/timers/live'",
        "fetch(\"/api/vdr/timers/live\"",
        "fetch('/api/vdr/timers'",
        "fetch(\"/api/vdr/timers\"",
    ]

    for token in forbidden_tokens:
        require(
            token not in load_timers_body,
            "loadTimers() must not directly fetch Timer API route " + token
        )



def check_searchtimer_loading_client_api_contract(app_js: str) -> None:
    start = app_js.find("function loadSearchTimers() {")
    require(start >= 0, "app.js must define loadSearchTimers()")

    end = app_js.find("function loadRecordings() {", start)
    require(
        end > start,
        "app.js loadSearchTimers() boundary must end before loadRecordings()"
    )

    body = app_js[start:end]

    require(
        "window.VdrSuiteClientApi" in body,
        "loadSearchTimers() must use window.VdrSuiteClientApi"
    )

    require(
        "fetchClientSearchTimers" in body,
        "loadSearchTimers() must use fetchClientSearchTimers()"
    )

    forbidden_tokens = [
        "fetch('/api/vdr/searchtimers'",
        'fetch("/api/vdr/searchtimers"',
        "fetch('/api/searchtimers'",
        'fetch("/api/searchtimers"',
    ]

    for token in forbidden_tokens:
        require(
            token not in body,
            "loadSearchTimers() must not directly fetch SearchTimer route " + token
        )


def check_timer_conflict_loading_client_api_contract(app_js: str) -> None:
    start = app_js.find("function loadTimerConflictPanel(timers) {")
    require(start >= 0, "app.js must define loadTimerConflictPanel(timers)")

    end = app_js.find("function renderSearchTimerList(data) {", start)
    require(
        end > start,
        "app.js loadTimerConflictPanel() boundary must end before renderSearchTimerList()"
    )

    body = app_js[start:end]

    require(
        "window.VdrSuiteClientApi" in body,
        "loadTimerConflictPanel() must use window.VdrSuiteClientApi"
    )

    require(
        "fetchClientTimerConflicts" in body,
        "loadTimerConflictPanel() must use fetchClientTimerConflicts()"
    )

    forbidden_tokens = [
        "fetch(\"/api/vdr/timers/conflicts/live\"",
        "fetch('/api/vdr/timers/conflicts/live'",
    ]

    for token in forbidden_tokens:
        require(
            token not in body,
            "loadTimerConflictPanel() must not directly fetch Timer conflict route " + token
        )



def check_channel_loading_client_api_contract(app_js: str) -> None:
    start = app_js.find("function loadChannels() {")
    require(start >= 0, "app.js must define loadChannels()")

    end = app_js.find("function loadTimers() {", start)
    require(end > start, "app.js loadChannels() boundary must end before loadTimers()")

    body = app_js[start:end]

    require(
        "window.VdrSuiteClientApi" in body,
        "loadChannels() must use window.VdrSuiteClientApi"
    )

    require(
        "fetchClientChannels" in body,
        "loadChannels() must use fetchClientChannels()"
    )

    require(
        "selectedEpgBackendId()" in body,
        "loadChannels() must preserve selectedEpgBackendId()"
    )

    require(
        "backend:" in body and "_:" in body,
        "loadChannels() must preserve backend and cache-busting query parameters"
    )

    require(
        ("cache: " + chr(39) + "no-store" + chr(39)) in body
        and ("credentials: " + chr(39) + "same-origin" + chr(39)) in body,
        "loadChannels() must preserve no-store and same-origin request options"
    )

    require(
        "fetch(" not in body,
        "loadChannels() must not directly fetch runtime data"
    )



def check_epg_timeline_channel_loading_client_api_contract(app_js: str) -> None:
    start = app_js.find("function loadEpgTimeline() {")
    require(start >= 0, "app.js must define loadEpgTimeline()")

    end = app_js.find("function renderEpgTimelinePlaceholder(data) {", start)
    require(
        end > start,
        "app.js loadEpgTimeline() boundary must end before renderEpgTimelinePlaceholder()"
    )

    body = app_js[start:end]

    require(
        "window.VdrSuiteClientApi" in body,
        "loadEpgTimeline() must use window.VdrSuiteClientApi"
    )

    require(
        "fetchClientChannels" in body,
        "loadEpgTimeline() must use fetchClientChannels() for Channel loading"
    )

    require(
        "fetchCachedOrLiveEpgWindow(channelData)" in body,
        "loadEpgTimeline() must keep fetchCachedOrLiveEpgWindow(channelData)"
    )

    require(
        "cache: " + chr(39) + "no-store" + chr(39) in body,
        "loadEpgTimeline() must preserve no-store Channel loading"
    )

    forbidden_tokens = [
        "fetch(" + chr(39) + "/api/vdr/channels" + chr(39),
        "fetch(" + chr(34) + "/api/vdr/channels" + chr(34),
    ]

    for token in forbidden_tokens:
        require(
            token not in body,
            "loadEpgTimeline() must not directly fetch Channel route " + token
        )



def check_epg_cache_status_client_api_contract(app_js: str) -> None:
    start = app_js.find("function fetchEpgCacheStatusForBackend(backendId) {")
    require(start >= 0, "app.js must define fetchEpgCacheStatusForBackend(backendId)")

    end = app_js.find("function listEventsFromEpgResponse(data) {", start)
    require(
        end > start,
        "app.js fetchEpgCacheStatusForBackend() boundary must end before listEventsFromEpgResponse()"
    )

    body = app_js[start:end]

    require(
        "window.VdrSuiteClientApi" in body,
        "fetchEpgCacheStatusForBackend() must use window.VdrSuiteClientApi"
    )

    require(
        "fetchClientEpgCacheStatus" in body,
        "fetchEpgCacheStatusForBackend() must use fetchClientEpgCacheStatus()"
    )

    require(
        "backend:" in body and "_:" in body,
        "fetchEpgCacheStatusForBackend() must preserve backend and cache-busting query parameters"
    )

    require(
        "cache: " + chr(39) + "no-store" + chr(39) in body,
        "fetchEpgCacheStatusForBackend() must preserve no-store loading"
    )

    require(
        "__statusError" in body,
        "fetchEpgCacheStatusForBackend() must preserve status error fallback object"
    )

    require(
        "fetch(" not in body,
        "fetchEpgCacheStatusForBackend() must not directly fetch runtime data"
    )



def check_epg_cache_window_client_api_contract(app_js: str) -> None:
    start = app_js.find("function fetchCachedEpgWindowForVisibleChannels(visibleChannels) {")
    require(
        start >= 0,
        "app.js must define fetchCachedEpgWindowForVisibleChannels(visibleChannels)"
    )

    end = app_js.find("function fetchVisibleCachedEpgWindow(channelData) {", start)
    require(
        end > start,
        "app.js fetchCachedEpgWindowForVisibleChannels() boundary must end before fetchVisibleCachedEpgWindow()"
    )

    body = app_js[start:end]

    require(
        "window.VdrSuiteClientApi" in body,
        "fetchCachedEpgWindowForVisibleChannels() must use window.VdrSuiteClientApi"
    )

    require(
        "fetchClientEpgCacheWindow" in body,
        "fetchCachedEpgWindowForVisibleChannels() must use fetchClientEpgCacheWindow()"
    )

    require(
        "channelIds:" in body and "channelIds.join(" in body,
        "fetchCachedEpgWindowForVisibleChannels() must batch visible channel IDs"
    )

    require(
        "cache: " + chr(39) + "no-store" + chr(39) in body,
        "fetchCachedEpgWindowForVisibleChannels() must preserve no-store loading"
    )

    require(
        "Promise.all" not in body,
        "fetchCachedEpgWindowForVisibleChannels() must not use per-channel Promise.all loading"
    )

    require(
        "fetch(" not in body,
        "fetchCachedEpgWindowForVisibleChannels() must not directly fetch runtime data"
    )


def check_channel_logos_contract(channel_logos_js: str) -> None:
    forbidden_patterns = [
        (r"createElement\s*\(\s*['\"]script['\"]\s*\)", "must not create script elements"),
        (r"\bfetch\s*\(", "must not fetch runtime data"),
        (r"timer-conflict", "must not contain Timer conflict logic"),
        (r"loadTimer", "must not contain Timer loading logic"),
        (r"renderTimerList", "must not contain Timer rendering logic"),
        (r"SearchTimer", "must not contain SearchTimer logic"),
        (r"recording", "must not contain recording module logic"),
    ]

    for pattern, reason in forbidden_patterns:
        require(
            not re.search(pattern, channel_logos_js, flags=re.IGNORECASE),
            f"channel-logos.js {reason}",
        )

    require(
        "function createChannelLogoElement" in channel_logos_js,
        "channel-logos.js must remain the owner of createChannelLogoElement()",
    )


def check_channel_browser_contract(channel_browser_js: str) -> None:
    require(
        "renderChannelList" in channel_browser_js,
        "channel-browser.js must remain the owner of renderChannelList registration",
    )
    require(
        "timer-conflict" not in channel_browser_js.lower(),
        "channel-browser.js must not contain Timer conflict logic",
    )


def check_style_contract(style_css: str, app_js: str) -> None:
    if "timer-conflict-panel" in app_js:
        require(
            ".timer-conflict-panel" in style_css,
            "style.css must define .timer-conflict-panel when app.js uses it",
        )
        require(
            ".timer-conflict-panel-alert" in style_css,
            "style.css must define .timer-conflict-panel-alert when Timer conflicts are rendered",
        )


def check_documentation_contract(frontend_architecture_md: str) -> None:
    required_terms = [
        "channel-logos.js",
        "Timer conflict panel",
        "Patch Placement Decision Tree",
        "document.createElement('script')",
        "web/frontend/app.js",
    ]

    for term in required_terms:
        require(
            term in frontend_architecture_md,
            f"frontend architecture documentation is missing required term: {term}",
        )




def check_frontend_static_serving_contract(test_http_server_cpp: str) -> None:
    require(
        '"frontend/api/client-api.js"' in test_http_server_cpp or '"/frontend/api/client-api.js"' in test_http_server_cpp,
        "TestHttpServer must serve /frontend/api/client-api.js"
    )

    require(
        '"api/client-api.js"' in test_http_server_cpp,
        "TestHttpServer must map /frontend/api/client-api.js to api/client-api.js"
    )




def check_recording_loading_client_api_contract(client_api: str) -> None:
    require(
        "function fetchClientRecordings(options)" in client_api,
        "client-api.js must define fetchClientRecordings(options)"
    )

    start = client_api.find("function fetchClientRecordings(options)")
    recording_action_boundary = client_api.find(
        "function fetchClientRecordingActionValidation(options)",
        start
    )
    search_timer_boundary = client_api.find(
        "function fetchClientSearchTimers(options)",
        start
    )

    end = (
        recording_action_boundary
        if recording_action_boundary > start
        else search_timer_boundary
    )

    require(
        end > start,
        "client-api.js fetchClientRecordings() boundary must end before the next client API function"
    )

    body = client_api[start:end]

    require(
        "requestJson('/api/vdr/recordings/query'" in body,
        "fetchClientRecordings() must use /api/vdr/recordings/query"
    )

    require(
        "requestJsonWithFallback" not in body,
        "fetchClientRecordings() must not fall back to the empty snapshot recordings route"
    )

    require(
        "'/api/vdr/recordings/live'" not in body,
        "fetchClientRecordings() must not use missing /api/vdr/recordings/live"
    )


def check_recording_module_loading_client_api_contract(app_js: str) -> None:
    start = app_js.find("function loadRecordings() {")
    require(start >= 0, "app.js must define loadRecordings()")

    end = app_js.find("function appendSettingsLine", start)
    require(
        end > start,
        "app.js loadRecordings() boundary must end before appendSettingsLine()"
    )

    body = app_js[start:end]

    require(
        "window.VdrSuiteClientApi" in body,
        "loadRecordings() must use window.VdrSuiteClientApi"
    )

    require(
        "fetchClientRecordings" in body,
        "loadRecordings() must use fetchClientRecordings()"
    )

    require(
        "selectedEpgBackendId()" in body,
        "loadRecordings() must pass the selected backend"
    )

    require(
        "backend:" in body and "limit:" in body and "_:" in body,
        "loadRecordings() must pass backend, limit and cache-busting query parameters"
    )

    require(
        "/api/vdr/recordings/query" in body,
        "loadRecordings() loading text must document /api/vdr/recordings/query"
    )

    require(
        "fetch('/api/vdr/recordings')" not in body and 'fetch("/api/vdr/recordings")' not in body,
        "loadRecordings() must not directly fetch the empty snapshot recordings route"
    )


def check_recording_bounded_rendering_contract(app_js: str) -> None:
    start = app_js.find("function renderRecordingList(data) {")
    require(start >= 0, "app.js must define renderRecordingList(data)")

    end = app_js.find("function formatEpgClockFromEpoch", start)
    require(
        end > start,
        "app.js renderRecordingList() boundary must end before formatEpgClockFromEpoch()"
    )

    body_with_constants = app_js[max(0, start - 500):end]
    body = app_js[start:end]

    require(
        "RECORDING_FOLDER_BATCH_SIZE" in body_with_constants,
        "renderRecordingList() must use RECORDING_FOLDER_BATCH_SIZE"
    )

    require(
        "RECORDING_ITEM_PAGE_SIZE" in body_with_constants,
        "renderRecordingList() must use RECORDING_ITEM_PAGE_SIZE"
    )

    require(
        "RECORDING_ITEM_PAGE_SIZE = 20" in body_with_constants,
        "Recording folder pagination must use 20 recordings per page"
    )

    require(
        "buildRecordingFolderTree" in body,
        "Recording module must build a hierarchical folder tree"
    )

    require(
        "createRecordingFolderNode" in body,
        "Recording module must use explicit folder tree nodes"
    )

    require(
        "node.folders" in body,
        "Recording module must render nested subfolders from folder tree nodes"
    )

    require(
        "leafRecordingFolders" in body,
        "Recording folder tree must detect single-recording leaf folders"
    )

    require(
        "displayChildFolders" in body,
        "Recording folder tree must separate display folders from promoted recording leaves"
    )

    require(
        "recordingEntries = node.recordings.concat" in body,
        "Recording folder tree must promote single-recording leaf folders into recording entries"
    )

    require(
        "displayChildFolders.slice(0, visibleFolderCount)" in body,
        "Recording folder overview must render a bounded display-folder slice"
    )

    require(
        "Eine Ebene zurück" in body,
        "Recording folder tree must provide parent navigation"
    )

    require(
        "createRecordingPagerControls" in body,
        "Recording folder view must expose pager controls"
    )

    require(
        "Vorherige 20" in body,
        "Recording folder view must expose a previous-page control"
    )

    require(
        "Nächste 20" in body,
        "Recording folder view must expose a next-page control"
    )

    require(
        "recordingEntries.slice(recordingStartIndex, recordingEndIndex)" in body,
        "Recording folder view must render the selected recording page only"
    )

    require(
        "/api/vdr/recordings/query" in body,
        "Recording empty-state text must reference /api/vdr/recordings/query"
    )



def check_client_api_contract():
    client_api_path = ROOT / "web/frontend/api/client-api.js"
    index_path = ROOT / "web/frontend/index.html"

    require(
        client_api_path.exists(),
        "web/frontend/api/client-api.js must exist as the DOM-free Client API wrapper"
    )

    client_api = client_api_path.read_text()
    index_html = index_path.read_text()

    require(
        "window.VdrSuiteClientApi" in client_api,
        "client-api.js must expose window.VdrSuiteClientApi"
    )

    required_exports = [
        "fetchClientTimers",
        "fetchClientTimerConflicts",
        "fetchClientChannels",
        "fetchClientCapabilities",
        "fetchClientBackends",
        "fetchClientDefaultBackend",
        "fetchClientEpgWindow",
        "fetchClientEpgSearch",
        "fetchClientEpgCacheStatus",
        "fetchClientEpgCacheWindow",
        "fetchClientRecordings",
        "fetchClientRecordingActionValidation",
        "fetchClientRecordingActionExecution",
        "fetchClientSearchTimers",
        "fetchClientSearchTimerDiscovery",
        "fetchClientSearchTimerPreview",
    ]

    for export_name in required_exports:
        require(
            export_name in client_api,
            "client-api.js must export " + export_name
        )


    require(
        "requestJson(" + chr(39) + "/api/vdr/capabilities" + chr(39) in client_api,
        "fetchClientCapabilities() must own /api/vdr/capabilities access"
    )
    require(
        "requestJson(" + chr(39) + "/api/backends" + chr(39) in client_api,
        "fetchClientBackends() must own /api/backends access"
    )
    require(
        "requestJson(" + chr(39) + "/api/backends/default" + chr(39) in client_api,
        "fetchClientDefaultBackend() must own /api/backends/default access"
    )
    check_recording_loading_client_api_contract(client_api)
    require(
        "function jsonPostOptions(options)" in client_api,
        "client-api.js must provide jsonPostOptions(options)"
    )
    require(
        "Content-Type" in client_api and "application/json" in client_api,
        "jsonPostOptions() must preserve JSON content type"
    )
    require(
        "function fetchClientRecordingActionValidation(options)" in client_api,
        "client-api.js must define fetchClientRecordingActionValidation(options)"
    )
    require(
        "/api/vdr/recordings/actions/validate" in client_api
        and "/api/recordings/actions/validate" in client_api,
        "fetchClientRecordingActionValidation() must own recording action validation route access"
    )
    require(
        "function fetchClientRecordingActionExecution(options)" in client_api,
        "client-api.js must define fetchClientRecordingActionExecution(options)"
    )
    require(
        "/api/vdr/recordings/actions/execute" in client_api
        and "/api/recordings/actions/execute" in client_api,
        "fetchClientRecordingActionExecution() must own recording action execution route access"
    )
    require(
        "function requestJsonWithFallbacks(paths, options)" in client_api,
        "client-api.js must provide requestJsonWithFallbacks(paths, options)"
    )
    require(
        "function fetchClientSearchTimers(options)" in client_api
        and "/api/vdr/searchtimers/live" in client_api
        and "/api/vdr/searchtimers" in client_api
        and "/api/searchtimers" in client_api,
        "fetchClientSearchTimers() must own live and snapshot SearchTimer route access"
    )
    require(
        "function fetchClientSearchTimerDiscovery(options)" in client_api,
        "client-api.js must define fetchClientSearchTimerDiscovery(options)"
    )
    require(
        "backendQueryOptions(options)" in client_api,
        "client-api.js must provide backendQueryOptions(options)"
    )
    require(
        "requestJson(" + chr(39) + "/api/epg/search" + chr(39) in client_api,
        "fetchClientEpgSearch() must own /api/epg/search access"
    )
    require(
        "requestJsonWithFallback(" in client_api
        and "/api/vdr/searchtimers/discovery" in client_api
        and "/api/searchtimers/discovery" in client_api,
        "fetchClientSearchTimerDiscovery() must own SearchTimer discovery route access"
    )
    require(
        "function fetchClientSearchTimerPreview(options)" in client_api,
        "client-api.js must define fetchClientSearchTimerPreview(options)"
    )
    require(
        "requestJsonWithFallback(" in client_api
        and "/api/vdr/searchtimers/preview" in client_api
        and "/api/searchtimers/preview" in client_api,
        "fetchClientSearchTimerPreview() must own SearchTimer preview route access"
    )
    require(
        "requestJson(" + chr(39) + "/api/epg/cache/status" + chr(39) in client_api,
        "fetchClientEpgCacheStatus() must own /api/epg/cache/status access"
    )


    require(
        "requestJson(" + chr(39) + "/api/epg/cache/window" + chr(39) in client_api,
        "fetchClientEpgCacheWindow() must own /api/epg/cache/window access"
    )

    forbidden_tokens = [
        "document.",
        "document[",
        "createElement",
        "className",
        "classList",
        "detailDataElement",
        "innerHTML",
        "appendChild",
    ]

    for token in forbidden_tokens:
        require(
            token not in client_api,
            "client-api.js must stay DOM-free and must not contain " + token
        )

    require(
        "client-api.js" in index_html,
        "index.html must load client-api.js before app.js"
    )

    require(
        index_html.index("client-api.js") < index_html.index("app.js"),
        "client-api.js must be loaded before app.js"
    )


def main() -> int:
    try:
        index_html = read(INDEX)
        app_js = read(APP)
        channel_logos_js = read(CHANNEL_LOGOS)
        channel_browser_js = read(CHANNEL_BROWSER)
        style_css = read(STYLE)
        frontend_architecture_md = read(ARCH_DOC)
        test_http_server_cpp = read(HTTP_SERVER)

        check_index_contract(index_html)
        check_app_contract(app_js)
        check_channel_loading_client_api_contract(app_js)
        check_epg_timeline_channel_loading_client_api_contract(app_js)
        check_epg_cache_status_client_api_contract(app_js)
        check_epg_cache_window_client_api_contract(app_js)
        check_timer_loading_client_api_contract(app_js)
        check_searchtimer_loading_client_api_contract(app_js)
        check_recording_module_loading_client_api_contract(app_js)
        check_recording_bounded_rendering_contract(app_js)
        check_timer_conflict_loading_client_api_contract(app_js)
        check_client_api_contract()
        check_frontend_static_serving_contract(test_http_server_cpp)
        check_channel_logos_contract(channel_logos_js)
        check_channel_browser_contract(channel_browser_js)
        check_style_contract(style_css, app_js)
        check_documentation_contract(frontend_architecture_md)
    except ContractFailure as exc:
        print(f"frontend ownership contract failed: {exc}", file=sys.stderr)
        return 1

    print("frontend ownership contracts ok")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
