#!/usr/bin/env python3

from __future__ import annotations

import json
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


def read(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")


def require(path: str, needle: str) -> None:
    if needle not in read(path):
        raise AssertionError(
            f"{path}: missing SearchTimer execution contract: {needle}"
        )


def forbid_section(
    path: str,
    start: str,
    end: str,
    needle: str,
) -> None:
    content = read(path)
    section = content.split(start, 1)[1].split(end, 1)[0]
    if needle in section:
        raise AssertionError(
            f"{path}: forbidden SearchTimer execution contract: {needle}"
        )


def validate_manifest(path: str) -> None:
    with (ROOT / path).open("r", encoding="utf-8") as handle:
        manifest = json.load(handle)

    assert manifest == {
        "schemaVersion": 1,
        "id": "slice-2n-searchtimer-execution",
        "title": "Phase 62 Slice 2N SearchTimer Execution",
        "permission": "searchtimers.execute",
        "action": "searchtimers.execute",
        "backendId": "default",
        "alternateBackendId": "phase62-slice2n-other",
        "routes": [
            "/api/searchtimers/execute",
            "/api/vdr/searchtimers/execute",
            "/api/searchtimers/real-test",
            "/api/vdr/searchtimers/real-test",
        ],
        "querySuffix": "?source=browser",
        "safeBody": {},
        "expectedValidation": {
            "status": 200,
            "json": {
                "success": False,
                "executed": False,
                "blocked": True,
                "dispatchStage": "validation-blocked",
                "operation": "unknown",
                "message": "workflow plan is not executable",
                "errors": [
                    "workflow plan is not executable",
                ],
            },
        },
        "snapshot": {
            "method": "GET",
            "path": (
                "/api/searchtimers?backend=default"
                "&limit=1000&offset=0"
            ),
        },
    }


def main() -> int:
    gate = "core/security/include/SecurityHttpGate.h"
    authorization = "core/security/include/AuthorizationService.h"
    loader = "web/frontend/platform/deferred-runtime-loader.js"
    makefile = "mk/security-sources.mk"
    local_tests = "mk/local-test-groups.mk"
    harness = "mk/phase62-runtime-acceptance.mk"
    manifest = (
        "tools/phase62-runtime-acceptance/"
        "slice-2n-searchtimer-execution.json"
    )
    document = (
        "docs/development/"
        "phase-62-slice-2n-searchtimer-execution-security-migration.md"
    )
    index = "docs/development/index.md"

    for relative in (
        "core/security/tests/test_searchtimer_execution_security.cpp",
        "web/frontend/tests/test_searchtimer_execution_security_runtime.js",
        manifest,
        document,
    ):
        if not (ROOT / relative).is_file():
            raise AssertionError(
                f"missing SearchTimer execution file: {relative}"
            )

    for route in (
        '"/api/searchtimers/execute"',
        '"/api/vdr/searchtimers/execute"',
        '"/api/searchtimers/real-test"',
        '"/api/vdr/searchtimers/real-test"',
    ):
        require(gate, route)

    require(gate, "isSearchTimerExecuteAction")
    require(gate, "isSearchTimerRealTestAction")
    require(gate, '"searchtimers.execute"')
    require(authorization, '"searchtimers.execute"')
    require(
        loader,
        "__vdrSuiteSearchTimerExecutionMutationCsrfWrapped",
    )

    for route in (
        "'/api/searchtimers/execute'",
        "'/api/vdr/searchtimers/execute'",
        "'/api/searchtimers/real-test'",
        "'/api/vdr/searchtimers/real-test'",
    ):
        require(loader, route)

    require(
        makefile,
        "test-security-searchtimer-execution:",
    )
    require(
        makefile,
        "test_searchtimer_execution_security.cpp",
    )
    require(
        local_tests,
        "test_searchtimer_execution_security_runtime.js",
    )
    require(
        harness,
        "slice-2n-searchtimer-execution.json",
    )
    require(
        index,
        "phase-62-slice-2n-searchtimer-execution-security-migration.md",
    )

    forbid_section(
        gate,
        "const bool isSafePost =",
        "const bool isProtectedMutation =",
        'path == "/api/searchtimers/execute"',
    )
    forbid_section(
        gate,
        "const bool isSafePost =",
        "const bool isProtectedMutation =",
        'path == "/api/searchtimers/real-test"',
    )

    validate_manifest(manifest)

    print("SearchTimer execution security contracts passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
