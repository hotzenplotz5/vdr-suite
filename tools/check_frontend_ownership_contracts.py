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
        "fetchClientEpgWindow",
        "fetchClientRecordings",
        "fetchClientSearchTimers",
    ]

    for export_name in required_exports:
        require(
            export_name in client_api,
            "client-api.js must export " + export_name
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

        check_index_contract(index_html)
        check_app_contract(app_js)
        check_client_api_contract()
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
