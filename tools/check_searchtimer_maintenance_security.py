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
            f"{path}: missing SearchTimer maintenance contract: {needle}"
        )


def validate_manifest(
    path: str,
    *,
    manifest_id: str,
    permission: str,
    action: str,
    routes: list[str],
) -> None:
    with (ROOT / path).open("r", encoding="utf-8") as handle:
        manifest = json.load(handle)

    assert manifest["schemaVersion"] == 1
    assert manifest["id"] == manifest_id
    assert manifest["permission"] == permission
    assert manifest["action"] == action
    assert manifest["backendId"] == "default"
    assert manifest["routes"] == routes
    assert manifest["safeBody"] == {}
    assert manifest["expectedValidation"] == {
        "status": 200,
        "json": {
            "success": False,
            "message": (
                "searchtimer backend native id is required"
            ),
            "errors": [
                "backendNativeId is required"
            ],
        },
    }


def main() -> int:
    required_files = [
        "core/security/tests/test_searchtimer_maintenance_security.cpp",
        "web/frontend/tests/test_searchtimer_maintenance_security_runtime.js",
        "tools/phase62-runtime-acceptance/slice-2l-searchtimer-update.json",
        "tools/phase62-runtime-acceptance/slice-2l-searchtimer-delete.json",
        "docs/development/phase-62-slice-2l-searchtimer-maintenance-security-migration.md",
        "docs/development/phase-62-security-contract-index.md",
    ]

    for relative in required_files:
        if not (ROOT / relative).is_file():
            raise AssertionError(
                f"missing SearchTimer maintenance file: {relative}"
            )

    gate = "core/security/include/SecurityHttpGate.h"
    authorization = "core/security/include/AuthorizationService.h"
    loader = "web/frontend/platform/deferred-runtime-loader.js"
    makefile = "mk/security-sources.mk"
    local_tests = "mk/local-test-groups.mk"
    harness_makefile = "mk/phase62-runtime-acceptance.mk"
    historical_index = "docs/development/phase-62-security-contract-index.md"

    for route in (
        '"/api/searchtimers/update"',
        '"/api/vdr/searchtimers/update"',
        '"/api/searchtimers/delete"',
        '"/api/vdr/searchtimers/delete"',
    ):
        require(gate, route)

    for permission in (
        '"searchtimers.modify"',
        '"searchtimers.delete"',
    ):
        require(gate, permission)
        require(authorization, permission)

    require(
        loader,
        "__vdrSuiteSearchTimerMaintenanceMutationCsrfWrapped",
    )

    for route in (
        "'/api/searchtimers/update'",
        "'/api/vdr/searchtimers/update'",
        "'/api/searchtimers/delete'",
        "'/api/vdr/searchtimers/delete'",
    ):
        require(loader, route)

    require(
        makefile,
        "test-security-searchtimer-maintenance:",
    )
    require(
        makefile,
        "test_searchtimer_maintenance_security.cpp",
    )
    require(
        local_tests,
        "test_searchtimer_maintenance_security_runtime.js",
    )
    require(
        harness_makefile,
        "slice-2l-searchtimer-update.json",
    )
    require(
        harness_makefile,
        "slice-2l-searchtimer-delete.json",
    )
    require(
        historical_index,
        "phase-62-slice-2l-searchtimer-maintenance-security-migration.md",
    )

    validate_manifest(
        "tools/phase62-runtime-acceptance/slice-2l-searchtimer-update.json",
        manifest_id="slice-2l-searchtimer-update",
        permission="searchtimers.modify",
        action="searchtimers.modify",
        routes=[
            "/api/searchtimers/update",
            "/api/vdr/searchtimers/update",
        ],
    )
    validate_manifest(
        "tools/phase62-runtime-acceptance/slice-2l-searchtimer-delete.json",
        manifest_id="slice-2l-searchtimer-delete",
        permission="searchtimers.delete",
        action="searchtimers.delete",
        routes=[
            "/api/searchtimers/delete",
            "/api/vdr/searchtimers/delete",
        ],
    )

    print(
        "SearchTimer maintenance security contracts passed"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
