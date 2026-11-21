#!/usr/bin/env python3
from __future__ import annotations

import importlib.util
import json
import sqlite3
from pathlib import Path
from types import ModuleType
from typing import Any


RUNNER_PATH = Path(__file__).with_name("runner.py")
REPOSITORY_ROOT = Path(__file__).resolve().parents[2]
ROUTE = "/api/epgsearch/native-fuzzy/stale-probes/delete"
PERMISSION = "epgsearch.native-fuzzy.stale-probes.delete"
ACTION = "epgsearch.native-fuzzy.stale-probes.delete"
BACKEND_SCOPE = "*"
TABLE = "epgsearch_native_fuzzy_capability_probes"
GUARD_TRIGGER = "phase62_slice2x_stale_probe_delete_guard"
TEST_BACKEND_ID = "phase62-slice2x-test-owned-stale-probe"
MAX_AGE_SECONDS = 7 * 24 * 60 * 60
SUCCESS_OPERATION_ID = "phase62-slice2x-success"
FAILURE_OPERATION_ID = "phase62-slice2x-failure"

MANIFEST: dict[str, Any] = {
    "schemaVersion": 1,
    "id": "slice-2x-protected-mutation-response-outcomes",
    "title": "Phase 62 Slice 2X protected mutation response outcomes",
    "permission": PERMISSION,
    "action": ACTION,
    "backendId": BACKEND_SCOPE,
    "alternateBackendId": "default",
    "routes": [ROUTE],
    "safeBody": {},
    "querySuffix": "?source=phase62-slice2x-runtime-acceptance",
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
        "method": "GET",
        "path": "/api/phase62/sqlite-snapshot-contract",
    },
}


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


def validate_contract() -> list[str]:
    errors: list[str] = []
    router_path = REPOSITORY_ROOT / "api/rest/src/ApiRouter.cpp"
    controller_path = (
        REPOSITORY_ROOT
        / "api/rest/src/"
        / "EpgSearchNativeFuzzyStaleProbeAdministrationController.cpp"
    )

    if not router_path.is_file():
        errors.append("ApiRouter.cpp is missing")
    else:
        router_text = router_path.read_text(encoding="utf-8")
        if f'"{ROUTE}"' not in router_text:
            errors.append("selected protected route is missing")

    if not controller_path.is_file():
        errors.append("stale-probe administration controller is missing")
    else:
        controller_text = controller_path.read_text(encoding="utf-8")
        if "summary.deleteFailures == 0 ? 200 : 500" not in controller_text:
            errors.append("selected route no longer has deterministic 200/500 outcomes")

    if MANIFEST["permission"] != PERMISSION:
        errors.append("manifest permission mismatch")
    if MANIFEST["action"] != ACTION:
        errors.append("manifest action mismatch")
    if MANIFEST["backendId"] != BACKEND_SCOPE:
        errors.append("manifest backend scope mismatch")

    return errors


def full_table_snapshot(database: sqlite3.Connection) -> list[tuple[Any, ...]]:
    return list(
        database.execute(
            f"""
            SELECT
                backend_id,
                create_accepted,
                readback_available,
                mode_preserved,
                tolerance_preserved,
                cleanup_succeeded,
                updated_at
            FROM {TABLE}
            ORDER BY backend_id
            """
        )
    )


def stale_row_count(database: sqlite3.Connection) -> int:
    return int(
        database.execute(
            f"""
            SELECT COUNT(*)
            FROM {TABLE}
            WHERE COALESCE(
                CAST(
                    strftime('%s', 'now') -
                    strftime('%s', updated_at)
                    AS INTEGER
                ),
                0
            ) < 0
               OR COALESCE(
                    CAST(
                        strftime('%s', 'now') -
                        strftime('%s', updated_at)
                        AS INTEGER
                    ),
                    0
                  ) > ?
            """,
            (MAX_AGE_SECONDS,),
        ).fetchone()[0]
    )


def verify_outcome_pair(
    rows: list[tuple[Any, ...]],
    *,
    request_id: str,
    operation_id: str,
    status_code: int,
    succeeded: bool,
) -> None:
    runner.require(
        len(rows) == 2,
        f"outcome_pair_count_mismatch:{request_id}:{len(rows)}",
    )

    decision, outcome = rows
    expected_event_type = (
        "operation.succeeded" if succeeded else "operation.failed"
    )
    expected_outcome = "succeeded" if succeeded else "failed"

    runner.require(
        decision[0] == "authorization.allowed"
        and decision[10] == "allowed"
        and decision[12] == "dispatch_authorized",
        f"predispatch_event_mismatch:{request_id}",
    )
    runner.require(
        outcome[0] == expected_event_type
        and outcome[10] == "allowed"
        and outcome[11] == f"http_status_{status_code}"
        and outcome[12] == expected_outcome,
        f"outcome_event_mismatch:{request_id}",
    )

    continuity_columns = (1, 2, 3, 4, 5, 6, 8, 9, 10)
    runner.require(
        all(decision[index] == outcome[index] for index in continuity_columns),
        f"outcome_context_mismatch:{request_id}",
    )
    runner.require(
        outcome[4] == PERMISSION
        and outcome[5] == BACKEND_SCOPE
        and outcome[6] == operation_id
        and outcome[7] == request_id
        and outcome[9] == ACTION,
        f"outcome_contract_mismatch:{request_id}",
    )


def self_test() -> None:
    errors = validate_contract()
    runner.require(not errors, ";".join(errors))

    shared = (
        "actor",
        "session",
        "authenticated",
        PERMISSION,
        BACKEND_SCOPE,
    )
    success_rows = [
        (
            "authorization.allowed",
            *shared,
            SUCCESS_OPERATION_ID,
            "request-success",
            "correlation",
            ACTION,
            "allowed",
            "permission_granted",
            "dispatch_authorized",
        ),
        (
            "operation.succeeded",
            *shared,
            SUCCESS_OPERATION_ID,
            "request-success",
            "correlation",
            ACTION,
            "allowed",
            "http_status_200",
            "succeeded",
        ),
    ]
    verify_outcome_pair(
        success_rows,
        request_id="request-success",
        operation_id=SUCCESS_OPERATION_ID,
        status_code=200,
        succeeded=True,
    )

    failure_rows = [
        (
            "authorization.allowed",
            *shared,
            FAILURE_OPERATION_ID,
            "request-failure",
            "correlation",
            ACTION,
            "allowed",
            "permission_granted",
            "dispatch_authorized",
        ),
        (
            "operation.failed",
            *shared,
            FAILURE_OPERATION_ID,
            "request-failure",
            "correlation",
            ACTION,
            "allowed",
            "http_status_500",
            "failed",
        ),
    ]
    verify_outcome_pair(
        failure_rows,
        request_id="request-failure",
        operation_id=FAILURE_OPERATION_ID,
        status_code=500,
        succeeded=False,
    )

    database = sqlite3.connect(":memory:", isolation_level=None)
    database.execute(
        f"""
        CREATE TABLE {TABLE} (
            backend_id TEXT PRIMARY KEY,
            create_accepted INTEGER NOT NULL DEFAULT 0,
            readback_available INTEGER NOT NULL DEFAULT 0,
            mode_preserved INTEGER NOT NULL DEFAULT 0,
            tolerance_preserved INTEGER NOT NULL DEFAULT 0,
            cleanup_succeeded INTEGER NOT NULL DEFAULT 0,
            updated_at TEXT DEFAULT CURRENT_TIMESTAMP
        )
        """
    )
    database.execute(
        f"""
        CREATE TRIGGER {GUARD_TRIGGER}
        BEFORE DELETE ON {TABLE}
        BEGIN
            SELECT RAISE(ABORT, 'phase62_slice2x_delete_blocked');
        END
        """
    )
    database.execute(
        f"""
        INSERT INTO {TABLE} (backend_id, updated_at)
        VALUES (?, datetime('now', '-8 days'))
        """,
        (TEST_BACKEND_ID,),
    )
    try:
        database.execute(
            f"DELETE FROM {TABLE} WHERE backend_id = ?",
            (TEST_BACKEND_ID,),
        )
    except sqlite3.DatabaseError:
        pass
    else:
        raise runner.AcceptanceError("delete_guard_self_test_failed")
    runner.require(
        stale_row_count(database) == 1,
        "stale_fixture_self_test_failed",
    )
    database.execute(f"DROP TRIGGER {GUARD_TRIGGER}")
    database.execute(
        f"DELETE FROM {TABLE} WHERE backend_id = ?",
        (TEST_BACKEND_ID,),
    )
    runner.require(
        full_table_snapshot(database) == [],
        "test_owned_row_cleanup_self_test_failed",
    )
    database.close()


class ProtectedMutationOutcomeAcceptance(runner.RuntimeAcceptance):
    def __init__(self, arguments: Any) -> None:
        super().__init__(arguments, MANIFEST)
        self.guard_installed = False
        self.test_row_inserted = False
        self.table_before: list[tuple[Any, ...]] = []
        self.success_request_id = ""
        self.failure_request_id = ""

    def trigger_present(self) -> bool:
        return self.database.execute(
            """
            SELECT 1
            FROM sqlite_master
            WHERE type = 'trigger' AND name = ?
            """,
            (GUARD_TRIGGER,),
        ).fetchone() is not None

    def install_guard(self) -> None:
        runner.require(not self.trigger_present(), "delete_guard_already_exists")
        self.database.execute(
            f"""
            CREATE TRIGGER {GUARD_TRIGGER}
            BEFORE DELETE ON {TABLE}
            BEGIN
                SELECT RAISE(ABORT, 'phase62_slice2x_delete_blocked');
            END
            """
        )
        self.guard_installed = True
        runner.require(self.trigger_present(), "delete_guard_not_installed")
        self.tests_passed += 1

    def insert_test_row(self) -> None:
        existing = self.database.execute(
            f"SELECT 1 FROM {TABLE} WHERE backend_id = ?",
            (TEST_BACKEND_ID,),
        ).fetchone()
        runner.require(existing is None, "test_backend_id_already_exists")
        self.database.execute(
            f"""
            INSERT INTO {TABLE} (
                backend_id,
                create_accepted,
                readback_available,
                mode_preserved,
                tolerance_preserved,
                cleanup_succeeded,
                updated_at
            ) VALUES (?, 0, 0, 0, 0, 0, datetime('now', '-8 days'))
            """,
            (TEST_BACKEND_ID,),
        )
        self.test_row_inserted = True
        runner.require(stale_row_count(self.database) == 1, "stale_test_row_missing")
        self.tests_passed += 1

    def request_with_operation(
        self,
        operation_id: str,
    ) -> tuple[tuple[int, dict[str, str], str], str]:
        response = self.request(
            "POST",
            ROUTE,
            browser=True,
            csrf=True,
            body={"operationId": operation_id},
        )
        request_id = f"{self.request_prefix}{self.sequence:03d}"
        return response, request_id

    def event_rows(self, request_id: str) -> list[tuple[Any, ...]]:
        return list(
            self.database.execute(
                """
                SELECT
                    event_type,
                    actor_id,
                    session_id,
                    authentication_state,
                    permission,
                    backend_id,
                    operation_id,
                    request_id,
                    correlation_id,
                    action,
                    decision,
                    reason_code,
                    outcome
                FROM accountability_events
                WHERE request_id = ?
                ORDER BY rowid
                """,
                (request_id,),
            )
        )

    def require_secret_free(self, rows: list[tuple[Any, ...]]) -> None:
        forbidden = (
            self.authorization,
            self.authorization.removeprefix("Basic "),
            self.cookie,
            self.csrf_token,
        )
        for row in rows:
            combined = "\n".join("" if value is None else str(value) for value in row)
            runner.require(
                all(not secret or secret not in combined for secret in forbidden),
                "secret_in_outcome_accountability",
            )

    def run(self) -> dict[str, int]:
        self.preflight()
        errors = validate_contract()
        runner.require(not errors, ";".join(errors))

        self.table_before = full_table_snapshot(self.database)
        runner.require(stale_row_count(self.database) == 0, "preexisting_stale_probe_rows")
        self.install_guard()
        self.issue_browser_session()
        runner.require(self.grant_state is not None, "grant_state_unavailable")
        self.grant_state.set_active(PERMISSION, BACKEND_SCOPE, True)

        success_response, self.success_request_id = self.request_with_operation(
            SUCCESS_OPERATION_ID
        )
        self.expect(
            "successful_protected_mutation",
            success_response,
            200,
            expected_json=MANIFEST["expectedValidation"]["json"],
        )
        success_rows = self.event_rows(self.success_request_id)
        verify_outcome_pair(
            success_rows,
            request_id=self.success_request_id,
            operation_id=SUCCESS_OPERATION_ID,
            status_code=200,
            succeeded=True,
        )
        self.require_secret_free(success_rows)
        self.tests_passed += 2

        self.insert_test_row()
        failure_response, self.failure_request_id = self.request_with_operation(
            FAILURE_OPERATION_ID
        )
        self.expect(
            "failed_protected_mutation",
            failure_response,
            500,
            expected_json={
                "schemaReady": True,
                "staleResultsFound": 1,
                "deletedResults": 0,
                "deleteFailures": 1,
            },
        )
        failure_rows = self.event_rows(self.failure_request_id)
        verify_outcome_pair(
            failure_rows,
            request_id=self.failure_request_id,
            operation_id=FAILURE_OPERATION_ID,
            status_code=500,
            succeeded=False,
        )
        self.require_secret_free(failure_rows)
        self.tests_passed += 2

        return {
            "authorized": 2,
            "succeeded": 1,
            "failed": 1,
        }

    def remove_test_changes(self) -> None:
        if self.guard_installed:
            self.database.execute(f"DROP TRIGGER IF EXISTS {GUARD_TRIGGER}")
            self.guard_installed = False
        if self.test_row_inserted:
            self.database.execute(
                f"DELETE FROM {TABLE} WHERE backend_id = ?",
                (TEST_BACKEND_ID,),
            )
            self.test_row_inserted = False

    def cleanup(self) -> list[str]:
        errors: list[str] = []
        try:
            self.remove_test_changes()
        except Exception as error:
            errors.append("test_change_cleanup:" + type(error).__name__)
        errors.extend(super().cleanup())
        return errors

    def final_verify(self) -> None:
        super().final_verify()
        runner.require(not self.trigger_present(), "delete_guard_remains")
        runner.require(
            self.database.execute(
                f"SELECT 1 FROM {TABLE} WHERE backend_id = ?",
                (TEST_BACKEND_ID,),
            ).fetchone() is None,
            "test_owned_stale_probe_remains",
        )
        runner.require(
            full_table_snapshot(self.database) == self.table_before,
            "stale_probe_table_not_restored",
        )


def main() -> int:
    arguments = runner.parse_arguments()
    errors = validate_contract()
    if errors:
        for error in errors:
            print(f"validation_error={error}")
        return 1

    if arguments.validate_only:
        print("phase62_slice2x_runtime_acceptance_contract=valid")
        return 0
    if arguments.self_test:
        self_test()
        print("phase62_slice2x_runtime_acceptance_self_test=passed")
        return 0

    for name in (
        "backup_dir",
        "expected_branch",
        "expected_head",
        "expected_daemon_sha256",
        "expected_loader_sha256",
    ):
        runner.require(bool(getattr(arguments, name)), f"missing_run_argument:{name}")

    acceptance = ProtectedMutationOutcomeAcceptance(arguments)
    counts: dict[str, int] = {}
    error_message = ""
    try:
        counts = acceptance.run()
    except Exception as error:
        error_message = f"{type(error).__name__}:{error}"

    cleanup_errors = acceptance.cleanup()
    if cleanup_errors:
        error_message = (error_message + ";" if error_message else "") + ",".join(cleanup_errors)

    try:
        acceptance.final_verify()
    except Exception as error:
        error_message = (
            (error_message + ";" if error_message else "")
            + f"final_verify:{type(error).__name__}:{error}"
        )

    report = {
        "schemaVersion": 1,
        "manifestId": MANIFEST["id"],
        "passed": not error_message,
        "testsPassed": acceptance.tests_passed,
        "httpRequests": acceptance.sequence,
        "accountability": counts,
        "successRequestId": acceptance.success_request_id,
        "failureRequestId": acceptance.failure_request_id,
        "error": error_message,
    }
    if arguments.report_json:
        report_path = Path(arguments.report_json)
        report_path.parent.mkdir(parents=True, exist_ok=True)
        report_path.write_text(
            json.dumps(report, indent=2, sort_keys=True) + "\n",
            encoding="utf-8",
        )

    acceptance.database.close()
    if error_message:
        print(f"phase62_slice2x_runtime_acceptance_error={error_message}")
        return 1

    print("slice=slice-2x-protected-mutation-response-outcomes")
    print(f"tests_passed={acceptance.tests_passed}")
    print(f"runtime_http_requests={acceptance.sequence}")
    print("protected_mutation_succeeded_outcome=yes")
    print("protected_mutation_failed_outcome=yes")
    print("outcome_context_continuity=yes")
    print("accountability_secret_free=yes")
    print("test_owned_stale_probe_removed=yes")
    print("target_grants_restored=yes")
    print("browser_session_revoked=yes")
    print("database_integrity=yes")
    print("service_pid_unchanged=yes")
    print("phase62_slice2x_runtime_acceptance=passed")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except runner.AcceptanceError as error:
        print(f"phase62_slice2x_runtime_acceptance_error={error}")
        raise SystemExit(1)
