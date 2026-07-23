#!/usr/bin/env python3
"""Run frontend ownership contracts with Recordings 2 as sole recording UI owner."""

from __future__ import annotations

import re
import sys
from pathlib import Path

import frontend_ownership_contracts_core as core


ROOT = Path(__file__).resolve().parents[1]
FRONTEND = ROOT / "web" / "frontend"

_RETIRED_RECORDING_ACTION_ALIAS_ASSERTIONS = frozenset(
    {
        "fetchClientRecordingActionValidation() must own recording action validation route access",
        "fetchClientRecordingActionExecution() must own recording action execution route access",
    }
)


def read(path: Path) -> str:
    if not path.exists():
        raise core.ContractFailure(f"required file missing: {path.relative_to(ROOT)}")
    return path.read_text(encoding="utf-8")


def combined_http_server_source() -> str:
    paths = [ROOT / "core/http/src/TestHttpServer.cpp"]
    paths.extend(sorted((ROOT / "core/http/src").glob("TestHttpServer*.inc")))
    return "\n".join(read(path) for path in paths)


def check_index_contract(index_html: str) -> None:
    ordered_scripts = (
        '<script src="/frontend/platform/bootstrap.js"></script>',
        '<script src="/frontend/locales/de.js"></script>',
        '<script src="/frontend/locales/en.js"></script>',
        '<script src="/frontend/platform/i18n.js"></script>',
        '<script src="/frontend/platform/helpers.js"></script>',
        '<script src="/frontend/platform/deferred-runtime-loader.js"></script>',
        '<script src="/frontend/api/client-api.js"></script>',
        '<script src="/frontend/channel-logos.js"></script>',
        '<script src="/frontend/channel-browser.js"></script>',
        '<script src="/frontend/modules/timers.js"></script>',
        '<script src="/frontend/modules/searchtimers.js"></script>',
        '<script src="/frontend/channel-day-program.js"></script>',
        '<script src="/frontend/app.js"></script>',
    )
    positions = []
    for marker in ordered_scripts:
        position = index_html.find(marker)
        core.require(position >= 0, f"index.html is missing required script tag: {marker}")
        positions.append(position)
    core.require(
        positions == sorted(positions),
        "index.html frontend bootstrap scripts are out of order",
    )
    core.require(
        index_html.count('data-module="recordings2"') == 1,
        "index.html must expose exactly one Recordings 2 navigation entry",
    )
    core.require(
        'data-module="recordings"' not in index_html,
        "index.html must not expose the retired Recording browser module",
    )
    for retired in (
        '/frontend/recording-browser.js',
        '/frontend/modules/recordings.js',
        '/frontend/recording-artwork.js',
        '/frontend/recording-trash-ux.js',
    ):
        core.require(retired not in index_html, f"index.html still loads retired asset: {retired}")
    core.require(
        ".recording-list-item" not in index_html and ".recording-detail::before" not in index_html,
        "index.html must not keep retired Recording browser placeholder styles",
    )
    core.require(
        not re.search(r"createElement\s*\(\s*['\"]script['\"]\s*\)", index_html),
        "index.html must not create dynamic script loaders",
    )


def client_function_body(
    client_api: str,
    function_name: str,
    next_function_name: str,
) -> str:
    start_marker = f"function {function_name}(options)"
    end_marker = f"function {next_function_name}(options)"
    start = client_api.find(start_marker)
    end = client_api.find(end_marker, start)
    core.require(start >= 0, f"client-api.js must define {function_name}(options)")
    core.require(
        end > start,
        f"client-api.js {function_name} boundary must end before {next_function_name}",
    )
    return client_api[start:end]


def check_canonical_recording_action_client_contract(client_api: str) -> None:
    contracts = (
        (
            "fetchClientRecordingActionValidation",
            "fetchClientRecordingActionExecution",
            "/api/vdr/recordings/actions/validate",
            "/api/recordings/actions/validate",
        ),
        (
            "fetchClientRecordingActionExecution",
            "fetchClientSearchTimers",
            "/api/vdr/recordings/actions/execute",
            "/api/recordings/actions/execute",
        ),
    )

    for function_name, next_function_name, canonical_route, retired_alias in contracts:
        body = client_function_body(client_api, function_name, next_function_name)
        core.require(
            canonical_route in body,
            f"{function_name}() must own canonical route {canonical_route}",
        )
        core.require(
            "return requestJson(" in body,
            f"{function_name}() must use one canonical requestJson() mutation",
        )
        core.require(
            "jsonPostOptions(options)" in body,
            f"{function_name}() must preserve JSON POST request options",
        )
        core.require(
            "requestJsonWithFallback" not in body,
            f"{function_name}() must not use speculative mutation fallback",
        )
        core.require(
            retired_alias not in client_api,
            f"client-api.js must not retain retired Recording action alias {retired_alias}",
        )


def run_baseline_client_api_contract() -> None:
    """Run the pre-retirement baseline with superseded mutation aliases replaced.

    The extracted baseline predates the canonical-only Recording mutation contract
    introduced before this retirement. Every other baseline assertion remains
    active; the two obsolete alias assertions are replaced immediately afterwards
    by check_canonical_recording_action_client_contract().
    """

    baseline_require = core.require

    def require_current_contract(condition: bool, message: str) -> None:
        if message in _RETIRED_RECORDING_ACTION_ALIAS_ASSERTIONS:
            return
        baseline_require(condition, message)

    core.require = require_current_contract
    try:
        core.check_client_api_contract()
    finally:
        core.require = baseline_require


def check_recordings2_sole_owner_contract(
    index_html: str,
    loader_js: str,
    server_source: str,
    install_mk: str,
) -> None:
    retired_sources = (
        FRONTEND / "modules/recordings.js",
        FRONTEND / "recording-artwork.js",
        FRONTEND / "recording-trash-ux.js",
    )
    for path in retired_sources:
        core.require(
            not path.exists(),
            f"retired Recording browser source still exists: {path.relative_to(ROOT)}",
        )

    retired_tests = (
        "test_recording_move_new_folder.js",
        "test_recording_trash_workflow.js",
        "test_recording_poster_placeholder_contract.js",
        "test_recording_artwork_runtime.js",
        "test_recording_genre_artwork_runtime.js",
        "test_recording_trash_ux_runtime.js",
    )
    for filename in retired_tests:
        core.require(
            not (FRONTEND / "tests" / filename).exists(),
            f"retired Recording browser test still exists: {filename}",
        )

    required_recordings2_sources = (
        "recordings2-shared.js",
        "recordings2-folder-artwork.js",
        "recordings2-actions.js",
        "recordings2-browser-view.js",
        "recordings2-person-search-view.js",
        "recordings2-metadata-view.js",
        "recordings2-metadata-detail.js",
        "recordings2.js",
    )
    for filename in required_recordings2_sources:
        core.require(
            (FRONTEND / filename).exists(),
            f"Recordings 2 sole-owner source missing: {filename}",
        )

    recordings2_runtime = read(FRONTEND / "recordings2.js")
    core.require(
        "global.VdrSuiteRecordings2 = moduleApi;" in recordings2_runtime,
        "Recordings 2 must expose its module API",
    )
    core.require(
        "registerModule('recordings2', moduleApi)" in recordings2_runtime,
        "Recordings 2 must own the platform module registration",
    )
    core.require(
        "VdrSuiteRecordingBrowser" not in recordings2_runtime,
        "Recordings 2 must not depend on the retired Recording browser",
    )

    core.require(
        "'/frontend/recordings2.js'" in loader_js,
        "deferred runtime loader must load Recordings 2",
    )
    core.require(
        "recording-trash-ux" not in loader_js,
        "deferred runtime loader must not load retired Recording trash UX",
    )

    for retired_path in (
        "/frontend/recording-browser.js",
        "/frontend/modules/recordings.js",
        "/frontend/recording-trash-ux.js",
    ):
        core.require(
            retired_path not in server_source,
            f"HTTP server still exposes retired asset: {retired_path}",
        )

    for retired_source in (
        "web/frontend/modules/recordings.js",
        "web/frontend/recording-artwork.js",
        "web/frontend/recording-trash-ux.js",
    ):
        core.require(
            f"$(INSTALL) -m 0644 {retired_source}" not in install_mk,
            f"install-runtime still installs retired source: {retired_source}",
        )

    for retired_destination in (
        "$(DATADIR)/web/frontend/recording-browser.js",
        "$(DATADIR)/web/frontend/recording-artwork.js",
        "$(DATADIR)/web/frontend/recording-trash-ux.js",
        "$(DATADIR)/web/frontend/modules/recordings.js",
    ):
        core.require(
            retired_destination in install_mk,
            f"install-runtime must remove retired installed asset: {retired_destination}",
        )

    core.require(
        'data-module="recordings2"' in index_html,
        "Recordings 2 must be the static Aufnahme navigation owner",
    )


def check_install_contract(install_mk: str) -> None:
    required_absence_checks = (
        "! test -e /tmp/vdr-suite-pkgroot/usr/share/vdr-suite/web/frontend/recording-browser.js",
        "! test -e /tmp/vdr-suite-pkgroot/usr/share/vdr-suite/web/frontend/recording-artwork.js",
        "! test -e /tmp/vdr-suite-pkgroot/usr/share/vdr-suite/web/frontend/recording-trash-ux.js",
        "! test -e /tmp/vdr-suite-pkgroot/usr/share/vdr-suite/web/frontend/modules/recordings.js",
    )
    for token in required_absence_checks:
        core.require(token in install_mk, f"test-install-staging missing retirement assertion: {token}")


def main() -> int:
    try:
        index_html = read(FRONTEND / "index.html")
        app_js = read(FRONTEND / "app.js")
        client_api = read(FRONTEND / "api/client-api.js")
        channel_logos_js = read(FRONTEND / "channel-logos.js")
        channel_browser_js = read(FRONTEND / "modules/channels.js")
        style_css = read(FRONTEND / "style.css")
        frontend_architecture_md = read(ROOT / "docs/development/frontend-architecture.md")
        boundary_doc = read(ROOT / "docs/development/client-api-frontend-module-boundary-plan.md")
        platform_bootstrap_js = read(FRONTEND / "platform/bootstrap.js")
        loader_js = read(FRONTEND / "platform/deferred-runtime-loader.js")
        install_mk = read(ROOT / "mk/install.mk")
        server_source = combined_http_server_source()

        check_index_contract(index_html)
        core.check_app_contract(app_js)
        core.check_app_channel_browser_module_bridge_contract(app_js)
        core.check_app_direct_api_fetch_contract(app_js)
        core.check_channel_loading_client_api_contract(app_js)
        core.check_epg_timeline_channel_loading_client_api_contract(app_js)
        core.check_epg_cache_status_client_api_contract(app_js)
        core.check_epg_cache_window_client_api_contract(app_js)
        core.check_timer_loading_client_api_contract(app_js)
        core.check_searchtimer_loading_client_api_contract(app_js)
        core.check_timer_conflict_loading_client_api_contract(app_js)
        run_baseline_client_api_contract()
        check_canonical_recording_action_client_contract(client_api)
        core.check_channel_browser_module_path_serving_contract(server_source, install_mk)
        core.check_frontend_static_serving_contract(index_html, server_source, install_mk)
        check_install_contract(install_mk)
        core.check_frontend_platform_bootstrap_contract(platform_bootstrap_js)
        core.check_frontend_module_runtime_smoke_check_documentation(boundary_doc)
        core.check_channel_logos_contract(channel_logos_js)
        core.check_channel_browser_registry_registration_contract(channel_browser_js)
        core.check_channel_browser_contract(channel_browser_js)
        core.check_channel_browser_context_boundary_contract(app_js, channel_browser_js)
        core.check_app_searchtimer_renderer_extraction_contract(
            app_js,
            read(FRONTEND / "modules/searchtimers.js"),
        )
        core.check_active_timer_module_source_contract()
        core.check_app_owned_timer_context_consumption_contract(app_js)
        core.check_app_timer_context_boundary_contract(app_js)
        core.check_app_timer_conflict_fallback_removed_contract(app_js)
        core.check_timer_conflict_module_bridge_contract(
            app_js,
            read(FRONTEND / "modules/timers.js"),
        )
        core.check_app_timer_module_bridge_contract(app_js)
        core.check_app_timer_renderer_extraction_contract(app_js)
        core.check_app_timer_registry_registration_contract(app_js)
        core.check_prepared_frontend_helpers_source_contract()
        core.check_style_contract(style_css, app_js)
        core.check_documentation_contract(frontend_architecture_md)
        check_recordings2_sole_owner_contract(
            index_html,
            loader_js,
            server_source,
            install_mk,
        )
    except core.ContractFailure as exc:
        print(f"frontend ownership contract failed: {exc}", file=sys.stderr)
        return 1

    print("frontend ownership contracts ok")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
