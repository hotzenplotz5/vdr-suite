#!/usr/bin/env python3
from __future__ import annotations

import importlib.util
import json
import sqlite3
from pathlib import Path
from types import ModuleType
from typing import Any


RUNNER_PATH = Path(__file__).with_name("runner.py")
DELETE_GUARD_TRIGGER = (
    "phase62_runtime_native_fuzzy_stale_probe_delete_guard"
)
DELETE_GUARD_TABLE = (
    "epgsearch_native_fuzzy_capability_probes"
)
DEFAULT_MAX_AGE_SECONDS = 7 * 24 * 60 * 60


def load_runner() -> ModuleType:
    spec = importlib.util.spec_from_file_location(
        "phase62_runtime_acceptance_runner",
        RUNNER_PATH,
    )

    if spec is None or spec.loader is None:
        raise RuntimeError("runtime_acceptance_runner_unavailable")

    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


runner = load_runner()
base_validate_manifest = runner.validate_manifest
base_self_test = runner.self_test


def validate_global_manifest(
    manifest: dict[str, Any],
) -> list[str]:
    adapted = dict(manifest)
    adapted["snapshot"] = {
        "method": "GET",
        "path": "/api/phase62/sqlite-snapshot-contract",
    }
    errors = base_validate_manifest(adapted)

    if manifest.get("scopeMode") != "global":
        errors.append("scopeMode must be global")

    if manifest.get("backendId") != "*":
        errors.append("global scope backendId must be *")

    alternate = manifest.get("alternateBackendId")
    if not isinstance(alternate, str) or not alternate or alternate == "*":
        errors.append(
            "alternateBackendId must be a concrete non-global scope"
        )

    if manifest.get("requireEmptyStaleSnapshot") is not True:
        errors.append("requireEmptyStaleSnapshot must be true")

    snapshot = manifest.get("snapshot")
    expected_snapshot = {
        "mode": "sqlite",
        "table": DELETE_GUARD_TABLE,
        "maxAgeSeconds": DEFAULT_MAX_AGE_SECONDS,
    }
    if snapshot != expected_snapshot:
        errors.append(
            "snapshot must be the exact SQLite stale-probe contract"
        )

    expected = manifest.get("expectedValidation", {}).get("json")
    if not isinstance(expected, dict):
        errors.append("expectedValidation.json must be an object")
    else:
        required_zeroes = {
            "schemaReady": True,
            "staleResultsFound": 0,
            "deletedResults": 0,
            "deleteFailures": 0,
        }
        if expected != required_zeroes:
            errors.append(
                "expectedValidation.json must prove a zero-delete result"
            )

    return errors


def target_global_grants(
    manifest: dict[str, Any],
) -> tuple[tuple[str, str], ...]:
    permission = manifest["permission"]
    global_scope = manifest["backendId"]
    concrete_scope = manifest["alternateBackendId"]

    return (
        (permission, global_scope),
        (permission, concrete_scope),
        ("role.admin", global_scope),
        ("role.admin", concrete_scope),
        ("role.read-only", global_scope),
    )


def sqlite_stale_probe_snapshot(
    database: sqlite3.Connection,
    max_age_seconds: int,
) -> dict[str, Any]:
    table = database.execute(
        """
        SELECT 1
        FROM sqlite_master
        WHERE type = 'table'
          AND name = ?
        """,
        (DELETE_GUARD_TABLE,),
    ).fetchone()
    runner.require(
        table is not None,
        "stale_probe_table_missing",
    )

    rows = list(
        database.execute(
            f"""
            WITH persisted AS (
                SELECT
                    backend_id,
                    updated_at,
                    COALESCE(
                        CAST(
                            strftime('%s', 'now') -
                            strftime('%s', updated_at)
                            AS INTEGER
                        ),
                        0
                    ) AS age_seconds
                FROM {DELETE_GUARD_TABLE}
            )
            SELECT
                backend_id,
                updated_at,
                age_seconds
            FROM persisted
            WHERE age_seconds < 0
               OR age_seconds > ?
            ORDER BY backend_id
            """,
            (max_age_seconds,),
        )
    )

    stale_probes = []
    for backend_id, updated_at, age_seconds in rows:
        future = int(age_seconds) < 0
        stale_probes.append(
            {
                "backendId": str(backend_id),
                "updatedAt": "" if updated_at is None else str(updated_at),
                "ageSeconds": int(age_seconds),
                "maxAgeSeconds": max_age_seconds,
                "status": "future-timestamp" if future else "stale",
                "reason": (
                    "persisted probe timestamp is in the future"
                    if future
                    else (
                        "persisted probe result is older than the "
                        "freshness policy allows"
                    )
                ),
            }
        )

    return {"staleProbes": stale_probes}


def global_self_test(manifest: dict[str, Any]) -> None:
    base_self_test(manifest)

    database = sqlite3.connect(":memory:", isolation_level=None)
    database.execute(
        f"""
        CREATE TABLE {DELETE_GUARD_TABLE} (
            backend_id TEXT PRIMARY KEY,
            updated_at TEXT DEFAULT CURRENT_TIMESTAMP
        )
        """
    )

    empty = sqlite_stale_probe_snapshot(
        database,
        DEFAULT_MAX_AGE_SECONDS,
    )
    runner.require(
        empty == {"staleProbes": []},
        "sqlite_snapshot_empty_self_test_failed",
    )

    database.execute(
        f"""
        INSERT INTO {DELETE_GUARD_TABLE} (
            backend_id,
            updated_at
        ) VALUES
            ('fresh', datetime('now', '-1 day')),
            ('stale', datetime('now', '-8 days')),
            ('future', datetime('now', '+1 day'))
        """
    )

    payload = sqlite_stale_probe_snapshot(
        database,
        DEFAULT_MAX_AGE_SECONDS,
    )
    statuses = {
        item["backendId"]: item["status"]
        for item in payload["staleProbes"]
    }
    runner.require(
        statuses == {
            "future": "future-timestamp",
            "stale": "stale",
        },
        "sqlite_snapshot_freshness_self_test_failed",
    )
    database.close()


class GlobalStaleProbeDeleteAcceptance(runner.RuntimeAcceptance):
    def __init__(
        self,
        arguments: Any,
        manifest: dict[str, Any],
    ) -> None:
        super().__init__(arguments, manifest)
        self.delete_guard_installed = False

    def resource_snapshot(self) -> str:
        snapshot = sqlite_stale_probe_snapshot(
            self.database,
            self.manifest["snapshot"]["maxAgeSeconds"],
        )
        self.tests_passed += 1
        return json.dumps(
            snapshot,
            sort_keys=True,
            separators=(",", ":"),
        )

    def require_empty_stale_snapshot(self, snapshot: str) -> None:
        try:
            payload = json.loads(snapshot)
        except json.JSONDecodeError as error:
            raise runner.AcceptanceError(
                "stale_probe_snapshot_invalid_json"
            ) from error

        runner.require(
            payload.get("staleProbes") == [],
            "stale_probe_snapshot_not_empty",
        )
        self.tests_passed += 1

    def delete_guard_present(self) -> bool:
        row = self.database.execute(
            """
            SELECT sql
            FROM sqlite_master
            WHERE type = 'trigger'
              AND name = ?
            """,
            (DELETE_GUARD_TRIGGER,),
        ).fetchone()
        return row is not None

    def install_delete_guard(self) -> None:
        table = self.database.execute(
            """
            SELECT 1
            FROM sqlite_master
            WHERE type = 'table'
              AND name = ?
            """,
            (DELETE_GUARD_TABLE,),
        ).fetchone()
        runner.require(
            table is not None,
            "stale_probe_table_missing",
        )
        runner.require(
            not self.delete_guard_present(),
            "stale_probe_delete_guard_already_exists",
        )

        self.database.execute(
            f"""
            CREATE TRIGGER {DELETE_GUARD_TRIGGER}
            BEFORE DELETE ON {DELETE_GUARD_TABLE}
            BEGIN
                SELECT RAISE(
                    ABORT,
                    'phase62_runtime_stale_probe_delete_blocked'
                );
            END
            """
        )
        self.delete_guard_installed = True
        runner.require(
            self.delete_guard_present(),
            "stale_probe_delete_guard_not_installed",
        )
        self.tests_passed += 1

    def remove_delete_guard(self) -> None:
        if not self.delete_guard_installed:
            return

        self.database.execute(
            f"DROP TRIGGER IF EXISTS {DELETE_GUARD_TRIGGER}"
        )
        self.delete_guard_installed = False
        runner.require(
            not self.delete_guard_present(),
            "stale_probe_delete_guard_not_removed",
        )

    def run(self) -> dict[str, int]:
        self.preflight()

        resource_before = self.resource_snapshot()
        self.require_empty_stale_snapshot(resource_before)
        self.install_delete_guard()

        routes = self.manifest["routes"]
        safe_body = self.manifest["safeBody"]
        expected_validation = self.manifest["expectedValidation"]
        permission = self.manifest["permission"]
        global_scope = self.manifest["backendId"]
        concrete_scope = self.manifest["alternateBackendId"]

        for route in routes:
            self.expect(
                "unauthenticated",
                self.request("POST", route, body=safe_body),
                401,
            )

        for route in routes:
            self.expect(
                "legacy_basic_zero_delete",
                self.request(
                    "POST",
                    route,
                    basic=True,
                    body=safe_body,
                ),
                expected_validation["status"],
                expected_json=expected_validation["json"],
            )

        self.issue_browser_session()
        runner.require(
            self.grant_state is not None,
            "grant_state_unavailable",
        )

        for route in routes:
            self.expect(
                "missing_csrf",
                self.request(
                    "POST",
                    route,
                    browser=True,
                    body=safe_body,
                ),
                403,
                "csrf_validation_failed",
            )

        for route in routes:
            self.expect(
                "permission_denied",
                self.request(
                    "POST",
                    route,
                    browser=True,
                    csrf=True,
                    body=safe_body,
                ),
                403,
                "permission_denied",
            )

        self.grant_state.set_active(
            permission,
            concrete_scope,
            True,
        )

        for route in routes:
            self.expect(
                "concrete_permission_scope_denied",
                self.request(
                    "POST",
                    route,
                    browser=True,
                    csrf=True,
                    body=safe_body,
                ),
                403,
                "backend_scope_denied",
            )

        self.grant_state.set_active(
            permission,
            concrete_scope,
            False,
        )
        self.grant_state.set_active(
            permission,
            global_scope,
            True,
        )

        for route in routes:
            self.expect(
                "direct_global_permission",
                self.request(
                    "POST",
                    route,
                    browser=True,
                    csrf=True,
                    body=safe_body,
                ),
                expected_validation["status"],
                expected_json=expected_validation["json"],
            )

            self.expect(
                "query_string",
                self.request(
                    "POST",
                    route + self.manifest["querySuffix"],
                    browser=True,
                    csrf=True,
                    body=safe_body,
                ),
                expected_validation["status"],
                expected_json=expected_validation["json"],
            )

            self.expect(
                "trailing_slash",
                self.request(
                    "POST",
                    route + "/",
                    browser=True,
                    csrf=True,
                    body=safe_body,
                ),
                503,
                "security_policy_not_migrated",
            )

        self.grant_state.set_active(
            permission,
            global_scope,
            False,
        )
        self.grant_state.set_active(
            "role.admin",
            concrete_scope,
            True,
        )

        for route in routes:
            self.expect(
                "concrete_admin_scope_denied",
                self.request(
                    "POST",
                    route,
                    browser=True,
                    csrf=True,
                    body=safe_body,
                ),
                403,
                "backend_scope_denied",
            )

        self.grant_state.set_active(
            "role.admin",
            concrete_scope,
            False,
        )
        self.grant_state.set_active(
            "role.admin",
            global_scope,
            True,
        )

        for route in routes:
            self.expect(
                "global_admin_role",
                self.request(
                    "POST",
                    route,
                    browser=True,
                    csrf=True,
                    body=safe_body,
                ),
                expected_validation["status"],
                expected_json=expected_validation["json"],
            )

        self.grant_state.set_active(
            "role.read-only",
            global_scope,
            True,
        )

        for route in routes:
            self.expect(
                "global_read_only_precedence",
                self.request(
                    "POST",
                    route,
                    browser=True,
                    csrf=True,
                    body=safe_body,
                ),
                403,
                "role_read_only",
            )

        counts = self.verify_accountability()

        resource_after = self.resource_snapshot()
        self.require_empty_stale_snapshot(resource_after)
        runner.require(
            resource_after == resource_before,
            "resource_state_changed",
        )
        self.tests_passed += 1

        return counts

    def cleanup(self) -> list[str]:
        errors: list[str] = []

        try:
            self.remove_delete_guard()
        except Exception as error:
            errors.append(
                "delete_guard_cleanup:"
                + type(error).__name__
            )

        errors.extend(super().cleanup())
        return errors

    def final_verify(self) -> None:
        super().final_verify()
        runner.require(
            not self.delete_guard_present(),
            "stale_probe_delete_guard_remains",
        )


def main() -> int:
    runner.validate_manifest = validate_global_manifest
    runner.target_grants = target_global_grants
    runner.self_test = global_self_test
    runner.RuntimeAcceptance = GlobalStaleProbeDeleteAcceptance
    return runner.main()


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except runner.AcceptanceError as error:
        print(
            "phase62_runtime_acceptance_error="
            f"{error}"
        )
        raise SystemExit(1)
