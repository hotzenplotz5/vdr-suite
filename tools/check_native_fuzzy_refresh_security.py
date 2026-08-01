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
            f"{path}: missing Native Fuzzy refresh contract: {needle}"
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
            f"{path}: forbidden Native Fuzzy refresh contract: {needle}"
        )


def validate_manifest(path: str) -> None:
    with (ROOT / path).open("r", encoding="utf-8") as handle:
        manifest = json.load(handle)

    backend_id = "phase62-slice2o-missing-backend"

    assert manifest["schemaVersion"] == 1
    assert manifest["id"] == "slice-2o-native-fuzzy-refresh"
    assert manifest["permission"] == \
        "epgsearch.native-fuzzy.refresh"
    assert manifest["action"] == \
        "epgsearch.native-fuzzy.refresh"
    assert manifest["backendId"] == backend_id
    assert manifest["routes"] == [
        "/api/epgsearch/native-fuzzy/refresh",
        "/api/vdr/epgsearch/native-fuzzy/refresh",
    ]
    assert manifest["safeBody"] == {
        "backendId": backend_id,
        "probeQuery": "Phase 62 Slice 2O Acceptance",
        "tolerance": 2,
        "keepProbeSearchTimer": False,
        "updateBackendCapabilities": False,
    }
    assert manifest["expectedValidation"] == {
        "status": 404,
        "json": {
            "backendId": backend_id,
            "backendKnown": False,
            "createAttempted": False,
            "persisted": False,
            "backendCapabilitiesUpdated": False,
            "status": "backend-not-found",
            "errors": [
                f"backend not found: {backend_id}",
            ],
        },
    }
    assert manifest["snapshot"] == {
        "method": "GET",
        "path": "/api/backends",
    }


def main() -> int:
    gate = "core/security/include/SecurityHttpGate.h"
    authorization = "core/security/include/AuthorizationService.h"
    makefile = "mk/security-sources.mk"
    harness = "mk/phase62-runtime-acceptance.mk"
    manifest = (
        "tools/phase62-runtime-acceptance/"
        "slice-2o-native-fuzzy-refresh.json"
    )
    adapter = (
        "tools/phase62-runtime-acceptance/"
        "static-body-runner.py"
    )
    document = (
        "docs/development/"
        "phase-62-slice-2o-native-fuzzy-refresh-security-migration.md"
    )
    index = "docs/development/index.md"

    for relative in (
        "core/security/tests/test_native_fuzzy_refresh_security.cpp",
        manifest,
        adapter,
        document,
    ):
        if not (ROOT / relative).is_file():
            raise AssertionError(
                f"missing Native Fuzzy refresh file: {relative}"
            )

    for route in (
        '"/api/epgsearch/native-fuzzy/refresh"',
        '"/api/vdr/epgsearch/native-fuzzy/refresh"',
    ):
        require(gate, route)

    require(gate, "isNativeFuzzyRefreshAction")
    require(gate, '"epgsearch.native-fuzzy.refresh"')
    require(authorization, '"epgsearch.native-fuzzy.refresh"')
    require(
        makefile,
        "test-security-native-fuzzy-refresh:",
    )
    require(
        makefile,
        "test_native_fuzzy_refresh_security.cpp",
    )
    require(
        harness,
        "slice-2o-native-fuzzy-refresh.json",
    )
    require(
        harness,
        "phase62-runtime-acceptance-static-body:",
    )
    require(
        adapter,
        "safeBody.backendId must equal backendId",
    )
    require(
        index,
        "phase-62-slice-2o-native-fuzzy-refresh-security-migration.md",
    )

    forbid_section(
        gate,
        "const bool isSafePost =",
        "const bool isProtectedMutation =",
        'path == "/api/epgsearch/native-fuzzy/refresh"',
    )

    validate_manifest(manifest)

    print("Native Fuzzy refresh security contracts passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
