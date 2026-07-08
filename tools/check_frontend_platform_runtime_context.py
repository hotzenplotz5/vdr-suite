#!/usr/bin/env python3
"""Validate the Phase 60.3 frontend platform runtime context seam.

The platform bootstrap may store runtime references supplied by the legacy
frontend bootstrap, but it must not render DOM, perform HTTP access or own
feature-specific UI behavior.
"""

from __future__ import annotations

import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
BOOTSTRAP = ROOT / "web" / "frontend" / "platform" / "bootstrap.js"
APP = ROOT / "web" / "frontend" / "app.js"
TIMER_MODULE = ROOT / "web" / "frontend" / "modules" / "timers.js"
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


def main() -> int:
    bootstrap = read(BOOTSTRAP)
    app_js = read(APP)
    timer_module = read(TIMER_MODULE)
    helpers_js = read(HELPERS)

    required_tokens = [
        "Phase 60.3: Frontend platform runtime context foundation.",
        "let runtimeContext = Object.freeze({});",
        "function configureRuntimeContext(context)",
        "Frontend platform runtime context must be an object",
        "runtimeContext = Object.freeze(Object.assign({}, context));",
        "function getRuntimeContext()",
        "function getClientApi()",
        "global.VdrSuiteClientApi || null",
        "function getMountTarget(name)",
        "runtimeContext.mountTargets",
        "runtimeContext.mountTarget || null",
        "function getSelectedBackendId()",
        "runtimeContext.getSelectedBackendId()",
        "runtimeContext.selectedBackendId",
        "function getSelectedModule()",
        "runtimeContext.getSelectedModule()",
        "runtimeContext.selectedModule",
        "configureRuntimeContext: configureRuntimeContext",
        "getRuntimeContext: getRuntimeContext",
        "getClientApi: getClientApi",
        "getMountTarget: getMountTarget",
        "getSelectedBackendId: getSelectedBackendId",
        "getSelectedModule: getSelectedModule",
    ]

    for token in required_tokens:
        require(token in bootstrap, "platform runtime context contract missing: " + token)

    helpers_required_tokens = [
        "Phase 60.6a: Shared frontend helper source foundation.",
        "Prepared DOM-free and HTTP-free helper namespace for future module extraction.",
        "This file is intentionally not loaded by index.html yet.",
        "function firstValue(source, keys, fallback)",
        "function listFromResponse(data, key)",
        "function numberOrZero(value)",
        "function formatEpochClock(epochSeconds)",
        "const helpersApi = Object.freeze({",
        "global.VdrSuiteFrontendHelpers = helpersApi;",
    ]

    for token in helpers_required_tokens:
        require(token in helpers_js, "prepared frontend helper source contract missing: " + token)

    helpers_forbidden_tokens = [
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
        "registerModule(",
    ]

    for token in helpers_forbidden_tokens:
        require(token not in helpers_js, "prepared frontend helper source must stay DOM/API-free: " + token)

    timer_module_required_tokens = [
        "Phase 60.5c: Timer module source placeholder.",
        "Prepared module source path for the future Timer browser extraction.",
        "This file is intentionally not loaded by index.html yet.",
        "Runtime ownership stays in app.js until the Timer renderer is extracted.",
        "const timerModulePreparation = Object.freeze({",
        "phase: '60.5c'",
        "isPrepared: function()",
        "global.VdrSuitePreparedModules = Object.create(null);",
        "global.VdrSuitePreparedModules.timers = timerModulePreparation;",
    ]

    for token in timer_module_required_tokens:
        require(token in timer_module, "prepared Timer module contract missing: " + token)

    timer_module_forbidden_tokens = [
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
        "registerModule('timers'",
        'registerModule("timers"',
        "renderTimerList",
        "loadTimers",
        "loadTimerConflictPanel",
    ]

    for token in timer_module_forbidden_tokens:
        require(token not in timer_module, "prepared Timer module must remain inert before extraction: " + token)

    app_required_tokens = [
        "function configurePlatformRuntimeContextBoundary()",
        "window.VdrSuitePlatform.configureRuntimeContext({",
        "clientApi: window.VdrSuiteClientApi || null",
        "mountTarget: detailDataElement",
        "mountTargets: {",
        "channels: detailDataElement",
        "recordings: detailDataElement",
        "timers: detailDataElement",
        "searchtimers: detailDataElement",
        "epg: detailDataElement",
        "channelsort: detailDataElement",
        "getSelectedBackendId: selectedEpgBackendId",
        "getSelectedModule: function()",
        "configurePlatformRuntimeContextBoundary();",
        "configureChannelBrowserContextBoundary();\nconfigurePlatformRuntimeContextBoundary();",
        "function frontendPlatformModule(name, legacyApi)",
        "function frontendPlatformClientApi()",
        "window.VdrSuitePlatform.getClientApi()",
        "const clientApi = frontendPlatformClientApi();",
        "function frontendPlatformMountTarget(name, fallback)",
        "window.VdrSuitePlatform.getMountTarget(name)",
        "frontendPlatformMountTarget('channels', detailDataElement)",
        "frontendPlatformMountTarget('recordings', detailDataElement)",
        "channelBrowser.configureContext({",
        "recordingBrowser.configureMountTarget(frontendPlatformMountTarget('recordings', detailDataElement));",
        "window.VdrSuitePlatform.getModule(name)",
        "frontendPlatformModule('channels', window.VdrSuiteChannelBrowser)",
        "frontendPlatformModule('recordings', window.VdrSuiteRecordingBrowser)",
        "function registerAppOwnedTimerModule()",
        "function renderTimersThroughModule(data, conflictReport)",
        "function configureTimerBrowserContextBoundary()",
        "timerBrowser.configureContext({",
        "detailDataElement: frontendPlatformMountTarget('timers', detailDataElement)",
        "helpers: window.VdrSuiteFrontendHelpers || null",
        "configureTimerBrowserContextBoundary();",
        "frontendPlatformModule('timers', window.VdrSuiteTimerBrowser)",
        "return timerBrowser.renderList(data, conflictReport)",
        "renderTimersThroughModule(data, null);",
        "const timerBrowserApi = Object.freeze({",
        "let timerBrowserContext = Object.freeze({});",
        "function configureAppOwnedTimerBrowserContext(context)",
        "timerBrowserContext = Object.freeze(Object.assign({}, context || {}));",
        "configureContext: configureAppOwnedTimerBrowserContext",
        "const mountTarget = timerBrowserContext.detailDataElement || detailDataElement;",
        "const previous = mountTarget.querySelector(\"[data-timer-conflict-panel=\\\"true\\\"]\");",
        "const target = mountTarget.querySelector(\".list\") || mountTarget;",
        "mountTarget.replaceChildren();",
        "mountTarget.appendChild(list);",
        "window.VdrSuiteTimerBrowser = timerBrowserApi",
        "window.VdrSuitePlatform.registerModule('timers', timerBrowserApi)",
        "renderList: renderTimerList",
        "load: loadTimers",
        "loadConflicts: loadTimerConflictPanel",
        "registerAppOwnedTimerModule();",
    ]

    for token in app_required_tokens:
        require(token in app_js, "app.js platform runtime context wiring missing: " + token)

    forbidden_tokens = [
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
    ]

    for token in forbidden_tokens:
        require(token not in bootstrap, "platform bootstrap must remain DOM/API-free: " + token)

    print("frontend platform runtime context contract ok")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except ContractFailure as error:
        print(str(error), file=sys.stderr)
        raise SystemExit(1)
