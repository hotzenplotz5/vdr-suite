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
RECORDING_BROWSER = FRONTEND / "recording-browser.js"
STYLE = FRONTEND / "style.css"
ARCH_DOC = ROOT / "docs" / "development" / "frontend-architecture.md"
CLIENT_API_CONTRACT_SNAPSHOT = ROOT / "docs" / "development" / "web-client-api-contract-snapshot.md"
HTTP_SERVER = ROOT / "core" / "http" / "src" / "TestHttpServer.cpp"
INSTALL_MK = ROOT / "mk" / "install.mk"


class ContractFailure(Exception):
    pass


def read(path: Path) -> str:
    if not path.exists():
        raise ContractFailure(f"required file missing: {path.relative_to(ROOT)}")
    return path.read_text(encoding="utf-8")


def require(condition: bool, message: str) -> None:
    if not condition:
        raise ContractFailure(message)


def client_api_fetch_function_names(client_api: str) -> set[str]:
    return set(
        re.findall(
            r"\bfunction\s+(fetchClient[A-Za-z0-9_]+)\s*\(",
            client_api,
        )
    )


def client_api_export_names(client_api: str) -> set[str]:
    marker = "window.VdrSuiteClientApi = Object.freeze({"
    start = client_api.find(marker)

    require(
        start >= 0,
        "client-api.js must expose window.VdrSuiteClientApi through Object.freeze"
    )

    end = client_api.find("\n  });", start)

    require(
        end > start,
        "client-api.js export registry block must be closed by Object.freeze terminator"
    )

    export_block = client_api[start:end]

    return set(
        re.findall(
            r"\b(fetchClient[A-Za-z0-9_]+)\s*:",
            export_block,
        )
    )


def script_positions(index_html: str) -> dict[str, int]:
    scripts = {
        "app": '<script src="/frontend/app.js"></script>',
        "channel_logos": '<script src="/frontend/channel-logos.js"></script>',
        "channel_browser": '<script src="/frontend/channel-browser.js"></script>',
        "recording_browser": '<script src="/frontend/recording-browser.js"></script>',
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
        positions["app"]
        < positions["channel_logos"]
        < positions["channel_browser"]
        < positions["recording_browser"],
        "index.html script order must be app.js -> channel-logos.js -> channel-browser.js -> recording-browser.js",
    )
    require(
        '<script src="/frontend/channel-browser.js"></script>' in index_html,
        "index.html must keep channel-browser.js as the physical Channel browser asset path until the daemon whitelist is updated",
    )
    require(
        '<script src="/frontend/modules/channels.js"></script>' not in index_html,
        "index.html must not load /frontend/modules/channels.js before the physical Channel browser asset is moved",
    )

    require(
        "let recordingSortMode = " not in index_html,
        "index.html must not keep the extracted Recording browser runtime inline",
    )

    require(
        'src="/frontend/timer-conflicts.js"' not in index_html,
        "timer-conflicts.js must not be loaded as a separate bootstrap script; integrate Timer conflicts through app.js",
    )

    require(
        not re.search(r"createElement\s*\(\s*['\"]script['\"]\s*\)", index_html),
        "index.html must not create dynamic script loaders",
    )


def check_app_channel_browser_module_bridge_contract(app_js: str) -> None:
    require(
        "function renderChannelsThroughModule(data)" in app_js,
        "app.js must define renderChannelsThroughModule(data)",
    )
    require(
        "window.VdrSuiteChannelBrowser.renderList(data)" in app_js,
        "app.js must render Channels through window.VdrSuiteChannelBrowser.renderList(data)",
    )
    require(
        "Channel browser module render API is not available" in app_js,
        "app.js Channel browser bridge must fail clearly when renderList is missing",
    )
    require(
        "renderChannelList(" not in app_js,
        "app.js must not call the legacy global renderChannelList bridge",
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


def check_app_direct_api_fetch_contract(app_js: str) -> None:
    legacy_direct_api_fetches = set()

    direct_api_fetches = []

    for match in re.finditer(
        r"\bfetch\s*\(\s*([\"'`])(/api(?:/[^\"'`]*)?)\1",
        app_js,
    ):
        direct_api_fetches.append(match.group(2))

    direct_fetch_calls = re.findall(r"\bfetch\s*\(", app_js)
    unexpected_direct_api_fetches = sorted(
        set(direct_api_fetches) - legacy_direct_api_fetches
    )

    require(
        not direct_fetch_calls,
        "app.js must not call fetch() directly; use window.VdrSuiteClientApi"
    )
    require(
        not unexpected_direct_api_fetches,
        "app.js must not add new direct API fetch routes; use window.VdrSuiteClientApi: "
        + ", ".join(unexpected_direct_api_fetches)
    )

    for legacy_route in legacy_direct_api_fetches:
        require(
            legacy_route in direct_api_fetches,
            "app.js direct API fetch legacy inventory changed unexpectedly for "
            + legacy_route
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


def check_channel_browser_context_boundary_contract(
    app_js: str,
    channel_browser_js: str,
) -> None:
    required_channel_context_tokens = [
        "let channelBrowserContext = {}",
        "function configureChannelBrowserContext(context)",
        "function channelBrowserDetailDataElement()",
        "Channel browser detail data element is not configured",
        "channelBrowserContext.detailDataElement",
        "channelBrowserDetailDataElement().replaceChildren()",
        "channelBrowserDetailDataElement().appendChild(",
        "function channelBrowserAddText(element, text)",
        "channelBrowserAddText(document.createElement(",
        "function channelBrowserFirstValue(object, keys, fallback)",
        "channelBrowserFirstValue(channel, [",
        "function channelBrowserListFromResponse(data, key)",
        "function channelBrowserListEventsFromEpgResponse(data)",
        "channelBrowserListEventsFromEpgResponse(eventData)",
        "window.VdrSuiteChannelBrowser = Object.freeze({",
        "configureContext: configureChannelBrowserContext",
    ]

    for token in required_channel_context_tokens:
        require(
            token in channel_browser_js,
            "channel-browser.js context boundary missing: " + token,
        )

    forbidden_direct_mount_tokens = [
        "detailDataElement.replaceChildren",
        "detailDataElement.appendChild",
        "addText(document.createElement(",
        "firstValue(",
        "listFromResponse(",
        "listEventsFromEpgResponse(",
    ]

    for token in forbidden_direct_mount_tokens:
        require(
            token not in channel_browser_js,
            "channel-browser.js must use channelBrowserDetailDataElement() instead of: " + token,
        )

    require(
        "function configureChannelBrowserContextBoundary()" in app_js,
        "app.js must define configureChannelBrowserContextBoundary()",
    )
    require(
        "window.VdrSuiteChannelBrowser.configureContext({" in app_js,
        "app.js must configure the Channel browser context boundary",
    )
    require(
        "detailDataElement" in app_js,
        "app.js must provide the Channel browser mount target",
    )


def check_channel_browser_contract(channel_browser_js: str) -> None:
    require(
        "Phase 59.11b" in channel_browser_js,
        "channel-browser.js must document its selected-channel programme drag-scroll phase",
    )
    require(
        "renderChannelList" in channel_browser_js,
        "channel-browser.js must remain the owner of renderChannelList registration",
    )
    require(
        "window.VdrSuiteClientApi" in channel_browser_js,
        "channel-browser.js must use the Client API wrapper for runtime HTTP boundaries",
    )
    require(
        "fetchClientEpgCacheRefresh" in channel_browser_js,
        "channel-browser.js must route EPG cache refresh through fetchClientEpgCacheRefresh()",
    )
    require(
        "function epgEventsForChannel(channel, sourceEvents, nowSeconds)" in channel_browser_js,
        "channel-browser.js must own epgEventsForChannel(channel, sourceEvents, nowSeconds)",
    )
    require(
        "enableChannelMouseDragScroll(detailPane.querySelector('.channel-agenda-scroll'), 'y')" in channel_browser_js,
        "channel-browser.js must enable drag-scroll for the selected-channel programme list",
    )
    require(
        "Programm mit gedrückter Maustaste hoch/runter ziehen." in channel_browser_js,
        "channel-browser.js must expose programme drag-scroll affordance text",
    )
    require(
        "Programme must use native browser scrolling" not in channel_browser_js,
        "channel-browser.js must not keep the old programme drag-scroll exclusion comment",
    )
    require(
        "return epgEventsForChannel(channel, events, nowSeconds)" in channel_browser_js,
        "channel-browser.js channelEntries() must use epgEventsForChannel()",
    )
    require(
        "/api/epg/cache/refresh" not in channel_browser_js,
        "channel-browser.js must not own the EPG cache refresh route literal",
    )
    require(
        not re.search(r"\bfetch\s*\(", channel_browser_js),
        "channel-browser.js must not call fetch() directly",
    )
    require(
        "timer-conflict" not in channel_browser_js.lower(),
        "channel-browser.js must not contain Timer conflict logic",
    )
    require(
        "window.VdrSuiteChannelBrowser = Object.freeze({" in channel_browser_js,
        "channel-browser.js must expose the Channel browser module API surface",
    )
    require(
        "configureContext: configureChannelBrowserContext" in channel_browser_js,
        "Channel browser module API must expose configureContext",
    )
    require(
        "renderList: renderChannelBrowserList" in channel_browser_js,
        "Channel browser module API must expose renderList",
    )
    require(
        "function renderChannelBrowserList(data)" in channel_browser_js,
        "channel-browser.js must own renderChannelBrowserList(data)",
    )
    require(
        "renderChannelList = function(data)" not in channel_browser_js,
        "channel-browser.js must not keep the legacy renderChannelList bridge after Module API migration",
    )


def check_recording_browser_contract(recording_browser_js: str) -> None:
    require(
        "Phase 59.10q" in recording_browser_js,
        "recording-browser.js must document its mount target boundary phase",
    )
    require(
        "function recordingBrowserNormalizePathText(value)" in recording_browser_js,
        "recording-browser.js must own normalized Recording path text helper after app cleanup",
    )
    require(
        "function recordingBrowserDisplayParts(recording, index)" in recording_browser_js,
        "recording-browser.js must own Recording display parts after app cleanup",
    )
    require(
        "function renderRecordingNode(node)" in recording_browser_js,
        "recording-browser.js must own renderRecordingNode(node)",
    )
    require(
        ".replace(/^%+/, '')" in recording_browser_js,
        "recording-browser.js must strip leading VDR percent markers from display text",
    )
    require(
        "function recordingBrowserDisplayParts(recording, index)" in recording_browser_js,
        "recording-browser.js must keep a local Recording display text normalization helper",
    )
    require(
        "recordingDisplayParts = function(recording, index)" not in recording_browser_js,
        "recording-browser.js must not mutate the global recordingDisplayParts helper",
    )
    require(
        "recordingBrowserDetailDataElement().replaceChildren(container)" in recording_browser_js,
        "recording-browser.js must keep rendering into the shared detail data element through its context accessor",
    )
    require(
        "document.createElement" in recording_browser_js,
        "recording-browser.js must remain an explicit DOM-rendering module",
    )
    require(
        not re.search(r"\bfetch\s*\(", recording_browser_js),
        "recording-browser.js must not fetch runtime data directly",
    )

    forbidden_runtime_api_tokens = [
        "window.VdrSuiteClientApi",
        "/api/",
        "XMLHttpRequest",
        "EventSource",
        "WebSocket",
    ]

    for token in forbidden_runtime_api_tokens:
        require(
            token not in recording_browser_js,
            "recording-browser.js must not own runtime API access token: " + token,
        )


def check_recording_browser_dependency_contract(
    index_html: str,
    app_js: str,
    recording_browser_js: str,
) -> None:
    require(
        '<script src="/frontend/recording-browser.js"></script>' in index_html,
        "index.html must load recording-browser.js",
    )

    required_app_globals = [
        "const detailDataElement = document.getElementById('detail-data');",
        "function renderRecordingsThroughModule(data)",
    ]

    for token in required_app_globals:
        require(
            token in app_js,
            "app.js must keep shared Recording browser dependency: " + token,
        )

    forbidden_app_recording_legacy_helpers = [
        "function normalizePathText(value)",
        "function recordingDisplayParts(recording, index)",
        "function groupRecordings(recordings)",
        "const display = recordingDisplayParts(recording, index)",
        "normalizePathText(firstValue(recording",
    ]

    for token in forbidden_app_recording_legacy_helpers:
        require(
            token not in app_js,
            "app.js must not keep migrated Recording browser legacy helper: " + token,
        )

    required_recording_browser_dependencies = [
        "let recordingBrowserMountTarget = null",
        "function configureRecordingBrowserMountTarget(element)",
        "Recording browser mount target is invalid",
        "Recording browser mount target is not configured",
        "recordingBrowserMountTarget = element;",
        "return recordingBrowserMountTarget;",
        "function configureRecordingBrowserContext(context)",
        "configureRecordingBrowserMountTarget(value.detailDataElement);",
        "function recordingBrowserDetailDataElement()",
        "function recordingBrowserAddText(element, text)",
        "function recordingBrowserFirstValue(object, keys, fallback)",
        "for (const key of keys)",
        "return fallback;",
        "function recordingBrowserListFromResponse(data, key)",
        "if (Array.isArray(data)) {",
        "if (data && Array.isArray(data[key])) {",
        "if (data && Array.isArray(data.items)) {",
        "function recordingBrowserFormatDurationSeconds(value)",
        "function recordingBrowserFormatSizeMb(value)",
        "function recordingBrowserFormatRecordingStart(value)",
        "recordingBrowserFirstValue(recording, ['startTime', 'start', 'date'], '')",
        "function recordingBrowserNormalizePathText(value)",
        "function recordingBrowserDisplayParts(recording, index)",
        "const rawTitle = String(recordingBrowserFirstValue(",
        "const titleParts = rawTitle.split('/').filter(part => part !== '')",
        "const path = recordingBrowserNormalizePathText(recordingBrowserFirstValue(",
        "const display = recordingBrowserDisplayParts(recording, index)",
        "recordingBrowserAddText(document.createElement('h3'), node.name)",
        "recordingBrowserDetailDataElement().replaceChildren(container)",
        "function setRecordingBrowserRecords(records)",
        "function renderRecordingList(data)",
        "window.VdrSuiteRecordingBrowser = Object.freeze({",
        "configureContext: configureRecordingBrowserContext",
        "configureMountTarget: configureRecordingBrowserMountTarget",
        "decodeRecordingText: decodeRecordingText",
        "setRecords: setRecordingBrowserRecords",
        "renderList: renderRecordingList",
        "renderRoot: renderRecordingRoot",
        "renderNode: renderRecordingNode",
    ]

    for token in required_recording_browser_dependencies:
        require(
            token in recording_browser_js,
            "recording-browser.js dependency contract missing: " + token,
        )


def check_recording_browser_context_boundary_contract(
    app_js: str,
    recording_browser_js: str,
) -> None:
    require(
        "const RECORDING_BROWSER_CONTEXT_DEPENDENCIES = Object.freeze([" not in recording_browser_js,
        "recording-browser.js must no longer need a shared context dependency list",
    )
    require(
        "function configureRecordingBrowserMountTarget(element)" in recording_browser_js,
        "recording-browser.js must define configureRecordingBrowserMountTarget(element)",
    )
    require(
        "configureMountTarget: configureRecordingBrowserMountTarget" in recording_browser_js,
        "window.VdrSuiteRecordingBrowser must expose configureMountTarget",
    )
    require(
        "function configureRecordingBrowserContext(context)" in recording_browser_js,
        "recording-browser.js must keep configureContext compatibility through the mount target boundary",
    )
    require(
        "configureContext: configureRecordingBrowserContext" in recording_browser_js,
        "window.VdrSuiteRecordingBrowser must keep configureContext compatibility",
    )
    require(
        "contextDependencies:" not in recording_browser_js,
        "window.VdrSuiteRecordingBrowser must not expose contextDependencies after mount target extraction",
    )

    required_mount_target_tokens = [
        "let recordingBrowserMountTarget = null",
        "function configureRecordingBrowserMountTarget(element)",
        "Recording browser mount target is invalid",
        "Recording browser mount target is not configured",
        "recordingBrowserMountTarget = element;",
        "return recordingBrowserMountTarget;",
        "function recordingBrowserDetailDataElement()",
    ]

    for token in required_mount_target_tokens:
        require(
            token in recording_browser_js,
            "recording-browser.js mount target boundary missing: " + token,
        )

    forbidden_direct_helper_calls = [
        "addText(document",
        "firstValue(recording",
        "firstValue(data || {}",
        "listFromResponse(data, 'recordings')",
        "formatRecordingStart(",
        "formatDurationSeconds(",
        "formatSizeMb(",
        "detailDataElement.replaceChildren",
        "detailDataElement.appendChild",
    ]

    for token in forbidden_direct_helper_calls:
        require(
            token not in recording_browser_js,
            "recording-browser.js must route shared helper through context accessor instead of: " + token,
        )

    forbidden_fallback_tokens = [
        "function recordingBrowserFallbackContextValue(name)",
        "recordingBrowserFallbackContextValue(",
        "typeof detailDataElement",
        "typeof addText",
        "typeof firstValue",
        "typeof listFromResponse",
        "typeof formatDurationSeconds",
        "typeof formatSizeMb",
        "typeof formatRecordingStart",
        "typeof recordingDisplayParts",
        "let recordingBrowserContext = null",
        "function recordingBrowserContextValue(name)",
        "recordingBrowserContextValue(",
        "Recording browser context is not configured",
        "Recording browser context value missing:",
    ]

    for token in forbidden_fallback_tokens:
        require(
            token not in recording_browser_js,
            "recording-browser.js must not keep global helper fallback token: " + token,
        )

    required_local_response_helper_tokens = [
        "function recordingBrowserFirstValue(object, keys, fallback)",
        "for (const key of keys)",
        "function recordingBrowserListFromResponse(data, key)",
        "if (Array.isArray(data)) {",
        "if (data && Array.isArray(data[key])) {",
        "if (data && Array.isArray(data.items)) {",
    ]

    for token in required_local_response_helper_tokens:
        require(
            token in recording_browser_js,
            "recording-browser.js local response helper missing: " + token,
        )

    required_local_formatting_helper_tokens = [
        "function recordingBrowserFormatDurationSeconds(value)",
        "function recordingBrowserFormatSizeMb(value)",
        "function recordingBrowserFormatRecordingStart(value)",
    ]

    for token in required_local_formatting_helper_tokens:
        require(
            token in recording_browser_js,
            "recording-browser.js local formatting helper missing: " + token,
        )

    required_local_dom_helper_tokens = [
        "function recordingBrowserAddText(element, text)",
        "element.textContent = text;",
        "return element;",
    ]

    for token in required_local_dom_helper_tokens:
        require(
            token in recording_browser_js,
            "recording-browser.js local DOM text helper missing: " + token,
        )

    required_local_display_helper_tokens = [
        "function recordingBrowserNormalizePathText(value)",
        "function recordingBrowserDisplayParts(recording, index)",
        "const rawTitle = String(recordingBrowserFirstValue(",
        "const titleParts = rawTitle.split('/').filter(part => part !== '')",
        "const path = recordingBrowserNormalizePathText(recordingBrowserFirstValue(",
        "folder: decodeRecordingText(",
        "title: decodeRecordingText(",
    ]

    for token in required_local_display_helper_tokens:
        require(
            token in recording_browser_js,
            "recording-browser.js local display parts helper missing: " + token,
        )

    forbidden_context_helper_tokens = [
        "'addText'",
        "'firstValue'",
        "'listFromResponse'",
        "'formatDurationSeconds'",
        "'formatSizeMb'",
        "'formatRecordingStart'",
        "'recordingDisplayParts'",
        "recordingBrowserContextValue('addText')",
        "recordingBrowserContextValue('firstValue')",
        "recordingBrowserContextValue('listFromResponse')",
        "recordingBrowserContextValue('formatDurationSeconds')",
        "recordingBrowserContextValue('formatSizeMb')",
        "recordingBrowserContextValue('formatRecordingStart')",
        "recordingBrowserContextValue('recordingDisplayParts')",
        "sourceDisplayParts(recording, index)",
    ]

    for token in forbidden_context_helper_tokens:
        require(
            token not in recording_browser_js,
            "recording-browser.js must not require local helper from context: " + token,
        )

    require(
        "Recording browser mount target is not configured" in recording_browser_js,
        "recording-browser.js must fail clearly when mount target is not configured",
    )
    require(
        "recordingDisplayParts = function(recording, index)" not in recording_browser_js,
        "recording-browser.js context migration must not mutate recordingDisplayParts globally",
    )
    require(
        "detailDataElement" in app_js,
        "app.js must still provide the Recording browser mount target",
    )

    require(
        "window.VdrSuiteClientApi" not in recording_browser_js,
        "recording-browser.js context boundary must not include runtime Client API ownership",
    )
    require(
        "/api/" not in recording_browser_js,
        "recording-browser.js context boundary must not include literal runtime API routes",
    )

    bridge_start = app_js.find("function renderRecordingsThroughModule(data)")
    bridge_end = app_js.find("function loadRecordings()", bridge_start)
    require(
        bridge_start >= 0 and bridge_end > bridge_start,
        "app.js must keep the Recording browser context handshake before loadRecordings()",
    )

    bridge_body = app_js[bridge_start:bridge_end]

    require(
        "recordingBrowser.configureMountTarget(detailDataElement);" in bridge_body,
        "app.js must configure the Recording browser mount target before rendering",
    )

    forbidden_bridge_context_tokens = [
        "recordingBrowser.configureContext({",
        "detailDataElement: detailDataElement",
        "addText: addText",
        "firstValue: firstValue",
        "listFromResponse: listFromResponse",
        "formatDurationSeconds: formatDurationSeconds",
        "formatSizeMb: formatSizeMb",
        "formatRecordingStart: formatRecordingStart",
        "recordingDisplayParts: recordingDisplayParts",
    ]

    for token in forbidden_bridge_context_tokens:
        require(
            token not in bridge_body,
            "app.js Recording browser context handshake must not pass local helper: " + token,
        )


def check_recording_rich_renderer_migration_contract(
    app_js: str,
    recording_browser_js: str,
) -> None:
    require(
        "function renderRecordingList(data) {" not in app_js,
        "app.js must no longer own renderRecordingList(data) after rich renderer migration",
    )
    require(
        "function renderRecordingsThroughModule(data)" in app_js,
        "app.js must define renderRecordingsThroughModule(data)",
    )

    bridge_start = app_js.find("function renderRecordingsThroughModule(data)")
    bridge_end = app_js.find("function appendSettingsLine", bridge_start)
    require(
        bridge_start >= 0 and bridge_end > bridge_start,
        "app.js Recording module bridge boundary must end before appendSettingsLine()",
    )

    bridge_body = app_js[bridge_start:bridge_end]

    require(
        "window.VdrSuiteRecordingBrowser" in bridge_body,
        "app.js Recording module bridge must use window.VdrSuiteRecordingBrowser",
    )
    require(
        "configureMountTarget(detailDataElement)" in bridge_body,
        "app.js Recording module bridge must configure mount target before rendering",
    )
    require(
        "renderList(data)" in bridge_body,
        "app.js Recording module bridge must call renderList(data)",
    )
    require(
        "fetchClientRecordings" in bridge_body,
        "loadRecordings() must still load data through fetchClientRecordings()",
    )
    require(
        "renderRecordingList(data)" not in bridge_body,
        "app.js must not call renderRecordingList(data) directly after migration",
    )

    start = recording_browser_js.find("function renderRecordingList(data) {")
    require(
        start >= 0,
        "recording-browser.js must own rich renderRecordingList(data)",
    )

    end = recording_browser_js.find("function setRecordingBrowserRecords(records)", start)
    require(
        end > start,
        "recording-browser.js rich renderRecordingList(data) boundary must end before setRecordingBrowserRecords()",
    )

    body = recording_browser_js[start:end]
    body_with_constants = recording_browser_js[max(0, start - 500):end]

    require(
        "RECORDING_FOLDER_BATCH_SIZE" in body_with_constants,
        "recording-browser.js rich renderer must keep folder batching constant",
    )
    require(
        "RECORDING_ITEM_PAGE_SIZE = 20" in body_with_constants,
        "recording-browser.js rich renderer must keep 20-item paging constant",
    )

    required_rich_renderer_tokens = [
        "function buildRecordingFolderTree(items)",
        "function createRecordingPagerControls(",
        "function renderRecordingDetail(",
        "function createRecordingListItem(",
        "function renderFolderNode(",
        "Vorherige 20",
        "Nächste 20",
        "leafRecordingFolders",
        "displayChildFolders",
        "recordingEntries.slice(recordingStartIndex, recordingEndIndex)",
        "renderRecordingDetail(entry, node, visibleFolderCount, recordingPageIndex)",
        "Recording-Query-Endpunkt",
    ]

    for token in required_rich_renderer_tokens:
        require(
            token in body,
            "recording-browser.js rich renderer missing token: " + token,
        )

    require(
        "renderList: renderRecordingList" in recording_browser_js,
        "window.VdrSuiteRecordingBrowser must expose renderList: renderRecordingList",
    )
    require(
        "/api/" not in recording_browser_js,
        "recording-browser.js must not contain literal API routes after renderer migration",
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




def check_frontend_install_contract(install_mk: str) -> None:
    require(
        "web/frontend/recording-browser.js $(DESTDIR)$(DATADIR)/web/frontend/recording-browser.js" in install_mk,
        "install-runtime must install web/frontend/recording-browser.js"
    )
    require(
        "test -f /tmp/vdr-suite-pkgroot/usr/share/vdr-suite/web/frontend/recording-browser.js" in install_mk,
        "test-install-staging must verify recording-browser.js"
    )


def check_channel_browser_module_path_serving_contract(
    test_http_server_cpp: str,
    install_mk: str,
) -> None:
    require(
        'path == "/frontend/modules/channels.js"' in test_http_server_cpp,
        "TestHttpServer.cpp must whitelist /frontend/modules/channels.js before the physical Channel browser asset move",
    )
    require(
        '"modules/channels.js"' in test_http_server_cpp,
        "TestHttpServer.cpp must serve modules/channels.js before the physical Channel browser asset move",
    )
    require(
        "$(DATADIR)/web/frontend/modules" in install_mk,
        "install.mk must create the frontend modules directory",
    )
    require(
        "web/frontend/modules/channels.js" in install_mk,
        "install.mk must be ready to install web/frontend/modules/channels.js when it exists",
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

    require(
        '"/frontend/recording-browser.js"' in test_http_server_cpp,
        "TestHttpServer must allow /frontend/recording-browser.js"
    )

    require(
        '"recording-browser.js"' in test_http_server_cpp,
        "TestHttpServer must map /frontend/recording-browser.js to recording-browser.js"
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


def check_recording_bounded_rendering_contract(
    app_js: str,
    recording_browser_js: str,
) -> None:
    require(
        "function renderRecordingList(data) {" not in app_js,
        "app.js must not own renderRecordingList(data) after Recording browser extraction",
    )

    start = recording_browser_js.find("function renderRecordingList(data) {")
    require(start >= 0, "recording-browser.js must define renderRecordingList(data)")

    end = recording_browser_js.find("function setRecordingBrowserRecords(records)", start)
    require(
        end > start,
        "recording-browser.js renderRecordingList() boundary must end before setRecordingBrowserRecords()"
    )

    body_with_constants = recording_browser_js[max(0, start - 500):end]
    body = recording_browser_js[start:end]

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
        "Recording-Query-Endpunkt" in body,
        "Recording empty-state text must reference the query endpoint without owning literal API routes"
    )


def check_client_api_contract_snapshot(
    snapshot_md: str,
    defined_fetch_functions: set[str],
    exported_fetch_functions: set[str],
) -> None:
    require(
        "# Web Client API Contract Snapshot" in snapshot_md,
        "Web Client API contract snapshot must have a clear title"
    )
    require(
        "Phase 59.09f" in snapshot_md,
        "Web Client API contract snapshot must document the owning phase"
    )
    require(
        "No direct `fetch()` calls in `web/frontend/app.js`." in snapshot_md,
        "Web Client API contract snapshot must document the no-direct-fetch rule"
    )
    require(
        "Remaining known direct API fetch inventory" in snapshot_md
        and "- none" in snapshot_md,
        "Web Client API contract snapshot must document an empty direct fetch inventory"
    )
    require(
        "Missing Backend Route Gaps" in snapshot_md,
        "Web Client API contract snapshot must document missing backend route gaps"
    )

    for function_name in sorted(exported_fetch_functions):
        require(
            "- `" + function_name + "`" in snapshot_md,
            "Web Client API contract snapshot is missing exported function "
            + function_name
        )

    for function_name in sorted(defined_fetch_functions):
        require(
            "- `" + function_name + "`" in snapshot_md,
            "Web Client API contract snapshot is missing defined function "
            + function_name
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
    client_api_contract_snapshot = read(CLIENT_API_CONTRACT_SNAPSHOT)

    require(
        "window.VdrSuiteClientApi" in client_api,
        "client-api.js must expose window.VdrSuiteClientApi"
    )

    required_exports = [
        "fetchClientTimers",
        "fetchClientTimerConflicts",
        "fetchClientTimerCreateAction",
        "fetchClientTimerUpdateAction",
        "fetchClientTimerDeleteAction",
        "fetchClientChannels",
        "fetchClientChannelMoveAction",
        "fetchClientCapabilities",
        "fetchClientVdrOverview",
        "fetchClientVdrStatus",
        "fetchClientVdrHealth",
        "fetchClientVdrSnapshotSummary",
        "fetchClientVdrSnapshots",
        "fetchClientBackends",
        "fetchClientDefaultBackend",
        "fetchClientBackendSnapshot",
        "fetchClientEpgWindow",
        "fetchClientEpgSearch",
        "fetchClientEpgCacheStatus",
        "fetchClientEpgCacheWindow",
        "fetchClientEpgCacheRefresh",
        "fetchClientEpgNowNext",
        "fetchClientEpgTimeWindow",
        "fetchClientEpgChannelWindow",
        "fetchClientMetadata",
        "fetchClientPersons",
        "fetchClientRecordingPersons",
        "fetchClientRecordings",
        "fetchClientRecordingActionValidation",
        "fetchClientRecordingActionExecution",
        "fetchClientSearchTimers",
        "fetchClientSearchTimerDiscovery",
        "fetchClientSearchTimerPreview",
        "fetchClientSearchTimerPlan",
        "fetchClientSearchTimerValidate",
        "fetchClientSearchTimerExecute",
        "fetchClientSearchTimerRealTest",
        "fetchClientSearchTimerCreateAction",
        "fetchClientSearchTimerUpdateAction",
        "fetchClientSearchTimerDeleteAction",
    ]

    defined_fetch_functions = client_api_fetch_function_names(client_api)
    exported_fetch_functions = client_api_export_names(client_api)

    missing_exported_functions = sorted(
        defined_fetch_functions - exported_fetch_functions
    )
    unknown_exported_functions = sorted(
        exported_fetch_functions - defined_fetch_functions
    )

    check_client_api_contract_snapshot(
        client_api_contract_snapshot,
        defined_fetch_functions,
        exported_fetch_functions,
    )

    require(
        not missing_exported_functions,
        "client-api.js must export every fetchClient function: "
        + ", ".join(missing_exported_functions)
    )
    require(
        not unknown_exported_functions,
        "client-api.js must not export undefined fetchClient functions: "
        + ", ".join(unknown_exported_functions)
    )

    for export_name in required_exports:
        require(
            export_name in exported_fetch_functions,
            "client-api.js must export " + export_name
            + " through window.VdrSuiteClientApi"
        )


    require(
        "function fetchClientTimerCreateAction(options)" in client_api
        and "/api/vdr/timers/actions/create" in client_api,
        "fetchClientTimerCreateAction() must own /api/vdr/timers/actions/create access"
    )
    require(
        "function fetchClientTimerUpdateAction(options)" in client_api
        and "/api/vdr/timers/actions/update" in client_api,
        "fetchClientTimerUpdateAction() must own /api/vdr/timers/actions/update access"
    )
    require(
        "function fetchClientTimerDeleteAction(options)" in client_api
        and "/api/vdr/timers/actions/delete" in client_api,
        "fetchClientTimerDeleteAction() must own /api/vdr/timers/actions/delete access"
    )
    require(
        "function fetchClientChannelMoveAction(options)" in client_api
        and "/api/vdr/channels/move" in client_api
        and "jsonPostOptions(options)" in client_api,
        "fetchClientChannelMoveAction() must own /api/vdr/channels/move access"
    )
    require(
        "requestJson(" + chr(39) + "/api/vdr/capabilities" + chr(39) in client_api,
        "fetchClientCapabilities() must own /api/vdr/capabilities access"
    )

    vdr_runtime_state_routes = {
        "fetchClientVdrOverview": ("/api/vdr/overview", "/api/vdr"),
        "fetchClientVdrStatus": ("/api/vdr/status",),
        "fetchClientVdrHealth": ("/api/vdr/health",),
        "fetchClientVdrSnapshotSummary": ("/api/vdr/snapshot",),
        "fetchClientVdrSnapshots": ("/api/vdr/snapshots",),
    }

    for function_name, routes in vdr_runtime_state_routes.items():
        require(
            "function " + function_name + "(options)" in client_api,
            "client-api.js must define " + function_name + "(options)"
        )
        for route in routes:
            require(
                route in client_api,
                function_name + "() must own VDR runtime state route access for " + route
            )
    require(
        "requestJson(" + chr(39) + "/api/backends" + chr(39) in client_api,
        "fetchClientBackends() must own /api/backends access"
    )
    require(
        "requestJson(" + chr(39) + "/api/backends/default" + chr(39) in client_api,
        "fetchClientDefaultBackend() must own /api/backends/default access"
    )
    require(
        "function fetchClientBackendSnapshot(backendId, options)" in client_api
        and "/api/backends/" in client_api
        and "encodeURIComponent(id)" in client_api
        and "/snapshot" in client_api,
        "fetchClientBackendSnapshot() must own /api/backends/<id>/snapshot access"
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

    searchtimer_workflow_routes = {
        "fetchClientSearchTimerPlan": (
            "/api/vdr/searchtimers/plan",
            "/api/searchtimers/plan",
        ),
        "fetchClientSearchTimerValidate": (
            "/api/vdr/searchtimers/validate",
            "/api/searchtimers/validate",
        ),
        "fetchClientSearchTimerExecute": (
            "/api/vdr/searchtimers/execute",
            "/api/searchtimers/execute",
        ),
        "fetchClientSearchTimerRealTest": (
            "/api/vdr/searchtimers/real-test",
            "/api/searchtimers/real-test",
        ),
        "fetchClientSearchTimerCreateAction": (
            "/api/vdr/searchtimers",
            "/api/searchtimers",
        ),
        "fetchClientSearchTimerUpdateAction": (
            "/api/vdr/searchtimers/update",
            "/api/searchtimers/update",
        ),
        "fetchClientSearchTimerDeleteAction": (
            "/api/vdr/searchtimers/delete",
            "/api/searchtimers/delete",
        ),
    }

    for function_name, routes in searchtimer_workflow_routes.items():
        require(
            "function " + function_name + "(options)" in client_api,
            "client-api.js must define " + function_name + "(options)"
        )
        for route in routes:
            require(
                route in client_api,
                function_name + "() must own SearchTimer route access for " + route
            )

    require(
        "requestJson(" + chr(39) + "/api/epg/cache/status" + chr(39) in client_api,
        "fetchClientEpgCacheStatus() must own /api/epg/cache/status access"
    )


    require(
        "requestJson(" + chr(39) + "/api/epg/cache/window" + chr(39) in client_api,
        "fetchClientEpgCacheWindow() must own /api/epg/cache/window access"
    )
    require(
        "function fetchClientEpgCacheRefresh(options)" in client_api
        and "requestJson(" + chr(39) + "/api/epg/cache/refresh" + chr(39) in client_api
        and "method: normalized.method || " + chr(39) + "POST" + chr(39) in client_api,
        "fetchClientEpgCacheRefresh() must own POST access to /api/epg/cache/refresh"
    )

    read_route_checks = {
        "fetchClientEpgNowNext": ("/api/epg/now-next",),
        "fetchClientEpgTimeWindow": ("/api/epg/time-window",),
        "fetchClientEpgChannelWindow": ("/api/epg/channel-window",),
        "fetchClientMetadata": ("/api/metadata",),
        "fetchClientPersons": ("/api/vdr/persons", "/api/persons"),
        "fetchClientRecordingPersons": (
            "/api/vdr/recordings/persons/search",
            "/api/recordings/persons/search",
        ),
    }

    for function_name, routes in read_route_checks.items():
        require(
            "function " + function_name + "(options)" in client_api,
            "client-api.js must define " + function_name + "(options)"
        )
        for route in routes:
            require(
                route in client_api,
                function_name + "() must own read route access for " + route
            )

    missing_backend_route_gap_tokens = [
        "fetchClientPermissionReport",
        "fetchClientEventDetail",
        "fetchClientEventArtwork",
        "fetchClientEventMedia",
        "fetchClientRecordingMarks",
        "fetchClientRecordingResume",
        "fetchClientRecordingCut",
        "fetchClientRecordingPlayback",
        "/api/vdr/permissions",
        "/api/permissions",
        "/api/vdr/events/detail",
        "/api/vdr/events/artwork",
        "/api/vdr/events/media",
        "/api/vdr/recordings/marks",
        "/api/vdr/recordings/resume",
        "/api/vdr/recordings/cut",
        "/api/vdr/recordings/playback",
    ]

    for token in missing_backend_route_gap_tokens:
        require(
            token not in client_api,
            "client-api.js must not fake missing backend route gap token: " + token
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
        recording_browser_js = read(RECORDING_BROWSER)
        style_css = read(STYLE)
        frontend_architecture_md = read(ARCH_DOC)
        test_http_server_cpp = read(HTTP_SERVER)
        install_mk = read(INSTALL_MK)

        check_index_contract(index_html)
        check_app_contract(app_js)
        check_app_channel_browser_module_bridge_contract(app_js)
        check_app_direct_api_fetch_contract(app_js)
        check_channel_loading_client_api_contract(app_js)
        check_epg_timeline_channel_loading_client_api_contract(app_js)
        check_epg_cache_status_client_api_contract(app_js)
        check_epg_cache_window_client_api_contract(app_js)
        check_timer_loading_client_api_contract(app_js)
        check_searchtimer_loading_client_api_contract(app_js)
        check_recording_module_loading_client_api_contract(app_js)
        check_recording_bounded_rendering_contract(app_js, recording_browser_js)
        check_timer_conflict_loading_client_api_contract(app_js)
        check_client_api_contract()
        check_channel_browser_module_path_serving_contract(
            test_http_server_cpp,
            install_mk,
        )
        check_frontend_static_serving_contract(test_http_server_cpp)
        check_frontend_install_contract(install_mk)
        check_channel_logos_contract(channel_logos_js)
        check_channel_browser_contract(channel_browser_js)
        check_channel_browser_context_boundary_contract(
            app_js,
            channel_browser_js,
        )
        check_recording_browser_contract(recording_browser_js)
        check_recording_browser_dependency_contract(
            index_html,
            app_js,
            recording_browser_js,
        )
        check_recording_browser_context_boundary_contract(
            app_js,
            recording_browser_js,
        )
        check_recording_rich_renderer_migration_contract(
            app_js,
            recording_browser_js,
        )
        check_style_contract(style_css, app_js)
        check_documentation_contract(frontend_architecture_md)
    except ContractFailure as exc:
        print(f"frontend ownership contract failed: {exc}", file=sys.stderr)
        return 1

    print("frontend ownership contracts ok")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
