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
            f"{path}: missing Native Fuzzy security contract: {needle}"
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
            f"{path}: forbidden Native Fuzzy security contract: {needle}"
        )


def validate_refresh_manifest(path: str) -> None:
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


def validate_stale_delete_manifest(path: str) -> None:
    with (ROOT / path).open("r", encoding="utf-8") as handle:
        manifest = json.load(handle)

    assert manifest == {
        "schemaVersion": 1,
        "id": "slice-2q-native-fuzzy-stale-probe-delete",
        "title": (
            "Phase 62 Slice 2Q Native Fuzzy stale probe deletion "
            "runtime acceptance"
        ),
        "permission": (
            "epgsearch.native-fuzzy.stale-probes.delete"
        ),
        "action": (
            "epgsearch.native-fuzzy.stale-probes.delete"
        ),
        "backendId": "*",
        "alternateBackendId": "default",
        "routes": [
            "/api/epgsearch/native-fuzzy/stale-probes/delete",
            "/api/vdr/epgsearch/native-fuzzy/stale-probes/delete",
        ],
        "safeBody": {},
        "scopeMode": "global",
        "requireEmptyStaleSnapshot": True,
        "querySuffix": "?source=phase62-runtime-acceptance",
        "expectedValidation": {
            "status": 200,
            "json": {
                "schemaReady": True,
                "staleResultsFound": 0,
                "deletedResults": 0,
                "deleteFailures": 0,
            },
        },
        "snapshot": {
            "mode": "sqlite",
            "table": "epgsearch_native_fuzzy_capability_probes",
            "maxAgeSeconds": 604800,
        },
    }


def forbid_frontend_stale_delete_owner() -> None:
    routes = (
        "/api/epgsearch/native-fuzzy/stale-probes/delete",
        "/api/vdr/epgsearch/native-fuzzy/stale-probes/delete",
    )

    for path in sorted((ROOT / "web/frontend").rglob("*.js")):
        content = path.read_text(encoding="utf-8")
        for route in routes:
            if route in content:
                raise AssertionError(
                    f"{path.relative_to(ROOT)}: unexpected Slice 2Q "
                    f"frontend owner for {route}"
                )


def main() -> int:
    gate = "core/security/include/SecurityHttpGate.h"
    authorization = "core/security/include/AuthorizationService.h"
    test = "core/security/tests/test_native_fuzzy_refresh_security.cpp"
    makefile = "mk/security-sources.mk"
    harness = "mk/phase62-runtime-acceptance.mk"
    refresh_manifest = (
        "tools/phase62-runtime-acceptance/"
        "slice-2o-native-fuzzy-refresh.json"
    )
    refresh_adapter = (
        "tools/phase62-runtime-acceptance/"
        "static-body-runner.py"
    )
    stale_manifest = (
        "tools/phase62-runtime-acceptance/"
        "slice-2q-native-fuzzy-stale-probe-delete.json"
    )
    stale_runner = (
        "tools/phase62-runtime-acceptance/"
        "global-stale-probe-delete-runner.py"
    )
    refresh_document = (
        "docs/development/"
        "phase-62-slice-2o-native-fuzzy-refresh-security-migration.md"
    )
    stale_document = (
        "docs/development/"
        "phase-62-slice-2q-native-fuzzy-stale-probe-delete-security-migration.md"
    )
    index = "docs/development/index.md"

    for relative in (
        test,
        refresh_manifest,
        refresh_adapter,
        refresh_document,
        stale_manifest,
        stale_runner,
        stale_document,
    ):
        if not (ROOT / relative).is_file():
            raise AssertionError(
                f"missing Native Fuzzy security file: {relative}"
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
        refresh_adapter,
        "safeBody.backendId must equal backendId",
    )
    require(
        index,
        "phase-62-slice-2o-native-fuzzy-refresh-security-migration.md",
    )

    for route in (
        '"/api/epgsearch/native-fuzzy/stale-probes/delete"',
        '"/api/vdr/epgsearch/native-fuzzy/stale-probes/delete"',
    ):
        require(gate, route)
        require(test, route)
        require(stale_manifest, route.strip('"'))

    permission = "epgsearch.native-fuzzy.stale-probes.delete"
    require(gate, "isNativeFuzzyStaleProbeDeleteAction")
    require(gate, f'"{permission}"')
    require(gate, 'requestToAuthorize.backendId = "*";')
    require(authorization, f'"{permission}"')
    require(test, '"role.admin"')
    require(test, '"role.read-only"')
    require(test, 'event.backendId == "*"')
    require(test, "body-scope-must-be-ignored")
    require(
        harness,
        "phase62-runtime-acceptance-global-stale-probe-delete:",
    )
    require(harness, "slice-2q-native-fuzzy-stale-probe-delete.json")
    require(harness, "global-stale-probe-delete-runner.py")
    require(stale_runner, "sqlite_stale_probe_snapshot")
    require(stale_runner, "DEFAULT_MAX_AGE_SECONDS")
    require(stale_runner, "strftime('%s', 'now')")
    require(stale_runner, "sqlite_snapshot_freshness_self_test_failed")
    require(stale_runner, "stale_probe_snapshot_not_empty")
    require(stale_runner, 'payload.get("staleProbes") == []')
    require(stale_runner, '"deletedResults": 0')
    require(stale_runner, "concrete_admin_scope_denied")
    require(stale_runner, "global_read_only_precedence")
    require(stale_runner, "DELETE_GUARD_TRIGGER")
    require(stale_runner, "CREATE TRIGGER")
    require(stale_runner, "BEFORE DELETE")
    require(stale_runner, "DROP TRIGGER IF EXISTS")
    require(stale_runner, "stale_probe_delete_guard_remains")
    require(
        index,
        "phase-62-slice-2q-native-fuzzy-stale-probe-delete-security-migration.md",
    )
    require(stale_document, f"{permission}@*")
    require(stale_document, '"staleProbes":[]')
    require(stale_document, "deletedResults=0")
    require(stale_document, "direct read-only SQLite snapshot")
    require(stale_document, "HTTP 404")
    require(stale_document, "BEFORE DELETE")
    require(stale_document, "no Webfrontend request owner")

    for route in (
        'path == "/api/epgsearch/native-fuzzy/refresh"',
        '"/api/epgsearch/native-fuzzy/stale-probes/delete"',
        '"/api/vdr/epgsearch/native-fuzzy/stale-probes/delete"',
    ):
        forbid_section(
            gate,
            "const bool isSafePost =",
            "const bool isProtectedMutation =",
            route,
        )

    forbid_frontend_stale_delete_owner()
    validate_refresh_manifest(refresh_manifest)
    validate_stale_delete_manifest(stale_manifest)

    print("Native Fuzzy security contracts passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
