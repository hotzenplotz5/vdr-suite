#!/usr/bin/env python3
"""Validate the frontend platform runtime context seam."""

from __future__ import annotations

import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
BOOTSTRAP = ROOT / "web" / "frontend" / "platform" / "bootstrap.js"
APP = ROOT / "web" / "frontend" / "app.js"
TIMER_MODULE = ROOT / "web" / "frontend" / "modules" / "timers.js"
SEARCHTIMER_MODULE = ROOT / "web" / "frontend" / "modules" / "searchtimers.js"
HELPERS = ROOT / "web" / "frontend" / "platform" / "helpers.js"


class ContractFailure(Exception):
    pass


def read(path: Path) -> str:
    if not path.exists():
        raise ContractFailure(f"required file missing: {path.relative_to(ROOT)}")
    return path.read_text(encoding="utf-8")


def require(condition: bool, message: str) -> None:
    if not condition:
        raise ContractFailure(message)


def require_tokens(source: str, tokens: list[str], prefix: str) -> None:
    for token in tokens:
        require(token in source, prefix + token)


def main() -> int:
    bootstrap = read(BOOTSTRAP)
    app_js = read(APP)
    timer_module = read(TIMER_MODULE)
    searchtimer_module = read(SEARCHTIMER_MODULE)
    helpers_js = read(HELPERS)

    require_tokens(
        bootstrap,
        [
            "Phase 60.3: Frontend platform runtime context foundation.",
            "let runtimeContext = Object.freeze({});",
            "function configureRuntimeContext(context)",
            "runtimeContext = Object.freeze(Object.assign({}, context));",
            "function getRuntimeContext()",
            "function getClientApi()",
            "global.VdrSuiteClientApi || null",
            "function getMountTarget(name)",
            "runtimeContext.mountTargets",
            "function getSelectedBackendId()",
            "runtimeContext.getSelectedBackendId()",
            "function getSelectedModule()",
            "runtimeContext.getSelectedModule()",
            "configureRuntimeContext: configureRuntimeContext",
            "getRuntimeContext: getRuntimeContext",
            "getClientApi: getClientApi",
            "getMountTarget: getMountTarget",
            "getSelectedBackendId: getSelectedBackendId",
            "getSelectedModule: getSelectedModule",
        ],
        "platform runtime context contract missing: ",
    )

    require_tokens(
        helpers_js,
        [
            "Phase 60.6a: Shared frontend helper source foundation.",
            "function firstValue(source, keys, fallback)",
            "function listFromResponse(data, key)",
            "function numberOrZero(value)",
            "function formatEpochClock(epochSeconds)",
            "const helpersApi = Object.freeze({",
            "global.VdrSuiteFrontendHelpers = helpersApi;",
            "function configureWorkflowModule(name, legacyApi)",
            "platform.getSelectedBackendId()",
            "clientApi: platform && typeof platform.getClientApi === 'function'",
            "reload: workflowReload",
            "configureWorkflowModule('timers'",
            "configureWorkflowModule('searchtimers'",
        ],
        "frontend helper/runtime bridge contract missing: ",
    )

    for token in ["XMLHttpRequest", "EventSource", "WebSocket", "fetch("]:
        require(token not in helpers_js, "frontend helper source must stay transport-free: " + token)

    require_tokens(
        timer_module,
        [
            "Active Timer browser with safe create, edit, toggle and delete workflows.",
            "function configureContext(context)",
            "function renderList(data, conflictReport)",
            "function renderConflicts(report, timers, error)",
            "function timerActionPayload(timer, overrides)",
            "function validateTimerPayload(payload, requireTimerId)",
            "fetchClientTimerCreateAction",
            "fetchClientTimerUpdateAction",
            "fetchClientTimerDeleteAction",
            "const timerBrowserApi = Object.freeze({",
            "global.VdrSuiteTimerBrowser = timerBrowserApi;",
            "global.VdrSuitePlatform.registerModule('timers', timerBrowserApi);",
            "Timer browser mount target is not configured",
        ],
        "active Timer workflow contract missing: ",
    )

    require_tokens(
        searchtimer_module,
        [
            "SearchTimer browser with compact cards and safe create/preview/delete workflows.",
            "function configureContext(context)",
            "function renderList(data)",
            "function normalizeSearchTimer(searchTimer, index)",
            "function buildCreatePayload(form, template)",
            "function validateCreatePayload(payload)",
            "fetchClientSearchTimerPreview",
            "fetchClientSearchTimerCreateAction",
            "fetchClientSearchTimerDeleteAction",
            "global.VdrSuiteSearchTimerBrowser = api;",
            "global.VdrSuitePlatform.registerModule('searchtimers', api);",
            "SearchTimer browser mount target is not configured",
        ],
        "active SearchTimer workflow contract missing: ",
    )

    for source, label in [
        (timer_module, "Timer module"),
        (searchtimer_module, "SearchTimer module"),
    ]:
        for token in ["XMLHttpRequest", "EventSource", "WebSocket", "fetch("]:
            require(token not in source, f"{label} must use the Client API wrapper: {token}")

    require_tokens(
        app_js,
        [
            "function configurePlatformRuntimeContextBoundary()",
            "window.VdrSuitePlatform.configureRuntimeContext({",
            "clientApi: window.VdrSuiteClientApi || null",
            "mountTarget: detailDataElement",
            "mountTargets: {",
            "channels: detailDataElement",
            "recordings: detailDataElement",
            "timers: detailDataElement",
            "searchtimers: detailDataElement",
            "getSelectedBackendId: selectedEpgBackendId",
            "getSelectedModule: function()",
            "configurePlatformRuntimeContextBoundary();",
            "function frontendPlatformModule(name, legacyApi)",
            "function frontendPlatformClientApi()",
            "window.VdrSuitePlatform.getClientApi()",
            "function frontendPlatformMountTarget(name, fallback)",
            "window.VdrSuitePlatform.getMountTarget(name)",
            "function configureTimerBrowserContextBoundary()",
            "frontendPlatformModule('timers', window.VdrSuiteTimerBrowser)",
            "return timerBrowser.renderList(data, conflictReport)",
            "function renderTimerConflictsThroughModule(report, timers, error)",
            "return timerBrowser.renderConflicts(report, timers, error)",
            "function configureSearchTimerBrowserContextBoundary()",
            "frontendPlatformModule('searchtimers', window.VdrSuiteSearchTimerBrowser)",
            "function renderSearchTimersThroughModule(data)",
            "renderSearchTimersThroughModule(data);",
        ],
        "app.js platform runtime context wiring missing: ",
    )

    for token in [
        "document.",
        "document[",
        "createElement",
        "appendChild",
        "replaceChildren",
        "innerHTML",
        "fetch(",
        "XMLHttpRequest",
        "EventSource",
        "WebSocket",
    ]:
        require(token not in bootstrap, "platform bootstrap must remain DOM/API-free: " + token)

    print("frontend platform runtime context contract ok")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except ContractFailure as error:
        print(str(error), file=sys.stderr)
        raise SystemExit(1)
