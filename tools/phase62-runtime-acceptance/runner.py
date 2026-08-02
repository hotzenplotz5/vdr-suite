#!/usr/bin/env python3
from __future__ import annotations

import argparse
import hashlib
import http.client
import json
import re
import sqlite3
import subprocess
import sys
import tempfile
import uuid
from pathlib import Path
from typing import Any
from urllib.parse import urlsplit


REPOSITORY_ROOT = Path(__file__).resolve().parents[2]
DEFAULT_MANIFEST = Path(__file__).with_name("slice-2j.json")


class AcceptanceError(RuntimeError):
    pass


def require(condition: bool, message: str) -> None:
    if not condition:
        raise AcceptanceError(message)


def load_manifest(path: Path) -> dict[str, Any]:
    with path.open("r", encoding="utf-8") as handle:
        manifest = json.load(handle)

    require(
        isinstance(manifest, dict),
        "manifest_must_be_object",
    )

    return manifest


def validate_manifest(
    manifest: dict[str, Any],
) -> list[str]:
    errors: list[str] = []

    if manifest.get("schemaVersion") != 1:
        errors.append("schemaVersion must be 1")

    for key in (
        "id",
        "title",
        "permission",
        "action",
        "backendId",
        "alternateBackendId",
    ):
        value = manifest.get(key)

        if not isinstance(value, str) or not value:
            errors.append(
                f"{key} must be a non-empty string"
            )

    routes = manifest.get("routes")

    if not isinstance(routes, list) or not routes:
        errors.append(
            "routes must be a non-empty array"
        )
    elif len(routes) != len(set(routes)):
        errors.append("routes must be unique")
    elif not all(
        isinstance(route, str)
        and route.startswith("/api/")
        and not route.endswith("/")
        for route in routes
    ):
        errors.append(
            "routes must contain exact "
            "non-trailing-slash /api/ paths"
        )

    if manifest.get("safeBody") != {}:
        errors.append("safeBody must be exactly {}")

    query_suffix = manifest.get("querySuffix")

    if (
        not isinstance(query_suffix, str)
        or not query_suffix.startswith("?")
    ):
        errors.append(
            "querySuffix must start with ?"
        )

    validation = manifest.get(
        "expectedValidation"
    )

    if not isinstance(validation, dict):
        errors.append(
            "expectedValidation must be an object"
        )
    else:
        status = validation.get("status")

        if (
            not isinstance(status, int)
            or status < 100
            or status > 599
        ):
            errors.append(
                "expectedValidation.status must be "
                "an HTTP status"
            )

        if not isinstance(
            validation.get("json"),
            dict,
        ):
            errors.append(
                "expectedValidation.json must be "
                "an object"
            )

    snapshot = manifest.get("snapshot")

    if not isinstance(snapshot, dict):
        errors.append(
            "snapshot must be an object"
        )
    else:
        if snapshot.get("method") != "GET":
            errors.append(
                "snapshot.method must be GET"
            )

        path = snapshot.get("path")

        if (
            not isinstance(path, str)
            or not path.startswith("/api/")
        ):
            errors.append(
                "snapshot.path must start with /api/"
            )

    return errors


def validate_repository_routes(
    manifest: dict[str, Any],
) -> list[str]:
    router_path = (
        REPOSITORY_ROOT
        / "api"
        / "rest"
        / "src"
        / "ApiRouter.cpp"
    )

    if not router_path.is_file():
        return [
            f"missing router source: {router_path}"
        ]

    source = router_path.read_text(
        encoding="utf-8"
    )

    errors: list[str] = []

    for route in manifest["routes"]:
        if f'"{route}"' not in source:
            errors.append(
                "route missing from "
                f"ApiRouter.cpp: {route}"
            )

    return errors


def sha256(path: Path) -> str:
    digest = hashlib.sha256()

    with path.open("rb") as handle:
        for chunk in iter(
            lambda: handle.read(1024 * 1024),
            b"",
        ):
            digest.update(chunk)

    return digest.hexdigest()


def run_command(*arguments: str) -> str:
    return subprocess.check_output(
        arguments,
        text=True,
        cwd=REPOSITORY_ROOT,
    ).strip()


def verify_backup(directory: Path) -> None:
    checksum_path = directory / "SHA256SUMS"

    require(
        checksum_path.is_file(),
        "backup_checksums_missing",
    )

    for raw_line in checksum_path.read_text(
        encoding="utf-8"
    ).splitlines():
        line = raw_line.strip()

        if not line:
            continue

        parts = line.split(maxsplit=1)

        require(
            len(parts) == 2,
            "invalid_backup_checksum_line",
        )

        expected, filename = parts
        filename = filename.lstrip("*")
        path = directory / filename

        require(
            path.is_file(),
            f"backup_file_missing:{filename}",
        )

        require(
            sha256(path) == expected,
            f"backup_checksum_mismatch:{filename}",
        )


def target_grants(
    manifest: dict[str, Any],
) -> tuple[tuple[str, str], ...]:
    permission = manifest["permission"]
    backend_id = manifest["backendId"]
    alternate_backend_id = manifest[
        "alternateBackendId"
    ]

    return (
        (permission, backend_id),
        (permission, alternate_backend_id),
        ("role.admin", backend_id),
        ("role.admin", "*"),
        ("role.read-only", backend_id),
    )


def response_matches(
    body: str,
    expected: dict[str, Any],
) -> bool:
    try:
        decoded = json.loads(body)
    except json.JSONDecodeError:
        return False

    return (
        isinstance(decoded, dict)
        and all(
            decoded.get(key) == value
            for key, value in expected.items()
        )
    )


def summarize_accountability(
    rows: list[tuple[Any, ...]],
    *,
    permission: str,
    action: str,
    backend_id: str,
    route_count: int,
) -> dict[str, int]:
    relevant = [
        row
        for row in rows
        if row[4] == permission
    ]

    expected = {
        "csrf": route_count,
        "permission": route_count,
        "scope": route_count * 2,
        "read_only": route_count,
        "authorized": route_count * 4,
    }

    actual = {
        "csrf": sum(
            row[10] == "csrf_validation_failed"
            and row[11] == "dispatch_denied"
            for row in relevant
        ),
        "permission": sum(
            row[10] == "permission_denied"
            and row[11] == "dispatch_denied"
            for row in relevant
        ),
        "scope": sum(
            row[10] == "backend_scope_denied"
            and row[11] == "dispatch_denied"
            for row in relevant
        ),
        "read_only": sum(
            row[10] == "role_read_only"
            and row[11] == "dispatch_denied"
            for row in relevant
        ),
        "authorized": sum(
            row[11] == "dispatch_authorized"
            for row in relevant
        ),
    }

    require(
        actual == expected,
        "accountability_count_mismatch:"
        f"{actual}:expected:{expected}",
    )

    require(
        all(
            row[5] == backend_id
            and row[8] == action
            for row in relevant
        ),
        "accountability_contract_mismatch",
    )

    require(
        len(relevant) == sum(expected.values()),
        "unexpected_accountability_event_count:"
        f"{len(relevant)}",
    )

    return actual


class GrantState:
    def __init__(
        self,
        database: sqlite3.Connection,
        actor_id: str,
        grants: tuple[tuple[str, str], ...],
    ) -> None:
        self.database = database
        self.actor_id = actor_id
        self.grants = grants
        self.snapshot: dict[
            tuple[str, str],
            tuple[Any, ...] | None,
        ] = {}

    def capture_and_neutralize(self) -> None:
        for permission, backend_id in self.grants:
            self.snapshot[
                (permission, backend_id)
            ] = self.database.execute(
                """
                SELECT
                    active,
                    revoked_at,
                    created_at,
                    updated_at
                FROM security_actor_permission_grants
                WHERE actor_id = ?
                  AND permission = ?
                  AND backend_id = ?
                """,
                (
                    self.actor_id,
                    permission,
                    backend_id,
                ),
            ).fetchone()

        self.database.execute("BEGIN IMMEDIATE")

        try:
            for permission, backend_id in self.grants:
                self.database.execute(
                    """
                    UPDATE
                        security_actor_permission_grants
                    SET
                        active = 0,
                        revoked_at = CURRENT_TIMESTAMP,
                        updated_at = CURRENT_TIMESTAMP
                    WHERE actor_id = ?
                      AND permission = ?
                      AND backend_id = ?
                    """,
                    (
                        self.actor_id,
                        permission,
                        backend_id,
                    ),
                )

            self.database.commit()
        except Exception:
            self.database.rollback()
            raise

    def set_active(
        self,
        permission: str,
        backend_id: str,
        active: bool,
    ) -> None:
        self.database.execute("BEGIN IMMEDIATE")

        try:
            if active:
                self.database.execute(
                    """
                    INSERT INTO
                        security_actor_permission_grants
                        (
                            actor_id,
                            permission,
                            backend_id
                        )
                    VALUES (?, ?, ?)
                    ON CONFLICT(
                        actor_id,
                        permission,
                        backend_id
                    )
                    DO UPDATE SET
                        active = 1,
                        revoked_at = '',
                        updated_at = CURRENT_TIMESTAMP
                    """,
                    (
                        self.actor_id,
                        permission,
                        backend_id,
                    ),
                )
            else:
                self.database.execute(
                    """
                    UPDATE
                        security_actor_permission_grants
                    SET
                        active = 0,
                        revoked_at = CURRENT_TIMESTAMP,
                        updated_at = CURRENT_TIMESTAMP
                    WHERE actor_id = ?
                      AND permission = ?
                      AND backend_id = ?
                    """,
                    (
                        self.actor_id,
                        permission,
                        backend_id,
                    ),
                )

            self.database.commit()
        except Exception:
            self.database.rollback()
            raise

    def restore(self) -> None:
        if not self.snapshot:
            return

        self.database.execute("BEGIN IMMEDIATE")

        try:
            for (
                permission,
                backend_id,
            ), row in self.snapshot.items():
                if row is None:
                    self.database.execute(
                        """
                        DELETE FROM
                            security_actor_permission_grants
                        WHERE actor_id = ?
                          AND permission = ?
                          AND backend_id = ?
                        """,
                        (
                            self.actor_id,
                            permission,
                            backend_id,
                        ),
                    )
                    continue

                self.database.execute(
                    """
                    INSERT INTO
                        security_actor_permission_grants
                        (
                            actor_id,
                            permission,
                            backend_id,
                            active,
                            revoked_at,
                            created_at,
                            updated_at
                        )
                    VALUES (?, ?, ?, ?, ?, ?, ?)
                    ON CONFLICT(
                        actor_id,
                        permission,
                        backend_id
                    )
                    DO UPDATE SET
                        active = excluded.active,
                        revoked_at = excluded.revoked_at,
                        created_at = excluded.created_at,
                        updated_at = excluded.updated_at
                    """,
                    (
                        self.actor_id,
                        permission,
                        backend_id,
                        *row,
                    ),
                )

            self.database.commit()
        except Exception:
            self.database.rollback()
            raise

        for (
            permission,
            backend_id,
        ), expected in self.snapshot.items():
            actual = self.database.execute(
                """
                SELECT
                    active,
                    revoked_at,
                    created_at,
                    updated_at
                FROM security_actor_permission_grants
                WHERE actor_id = ?
                  AND permission = ?
                  AND backend_id = ?
                """,
                (
                    self.actor_id,
                    permission,
                    backend_id,
                ),
            ).fetchone()

            require(
                actual == expected,
                "target_grant_restore_mismatch",
            )


class RuntimeAcceptance:
    def __init__(
        self,
        arguments: argparse.Namespace,
        manifest: dict[str, Any],
    ) -> None:
        self.arguments = arguments
        self.manifest = manifest

        self.database = sqlite3.connect(
            arguments.database,
            timeout=10,
            isolation_level=None,
        )
        self.database.execute(
            "PRAGMA busy_timeout=10000"
        )

        self.service_pid = 0
        self.start_pid = 0
        self.authorization = ""
        self.cookie = ""
        self.csrf_token = ""
        self.session_id = ""
        self.actor_id = ""
        self.grant_state: GrantState | None = None

        self.sequence = 0
        self.tests_passed = 0
        self.request_prefix = (
            f"phase62-{manifest['id']}-"
            f"{uuid.uuid4().hex[:10]}-"
        )

        self.base_url = urlsplit(
            arguments.base_url
        )

        require(
            self.base_url.scheme in (
                "http",
                "https",
            )
            and bool(self.base_url.hostname),
            "invalid_base_url",
        )

    def request(
        self,
        method: str,
        path: str,
        *,
        basic: bool = False,
        browser: bool = False,
        csrf: bool = False,
        body: dict[str, Any] | None = None,
    ) -> tuple[int, dict[str, str], str]:
        self.sequence += 1

        headers = {
            "Accept": "application/json",
            "Connection": "close",
            "X-Request-ID": (
                f"{self.request_prefix}"
                f"{self.sequence:03d}"
            ),
        }

        if basic:
            headers["Authorization"] = (
                self.authorization
            )

        if browser:
            require(
                bool(self.cookie),
                "browser_cookie_missing",
            )
            headers["Cookie"] = (
                "vdr_suite_session="
                + self.cookie
            )

        if csrf:
            require(
                bool(self.csrf_token),
                "csrf_token_missing",
            )
            headers["X-CSRF-Token"] = (
                self.csrf_token
            )

        encoded_body = None

        if body is not None:
            encoded_body = json.dumps(
                body,
                separators=(",", ":"),
            )
            headers["Content-Type"] = (
                "application/json"
            )

        port = self.base_url.port or (
            443
            if self.base_url.scheme == "https"
            else 80
        )

        connection_class = (
            http.client.HTTPSConnection
            if self.base_url.scheme == "https"
            else http.client.HTTPConnection
        )

        connection = connection_class(
            self.base_url.hostname,
            port,
            timeout=self.arguments.timeout,
        )

        base_path = self.base_url.path.rstrip("/")

        try:
            connection.request(
                method,
                base_path + path,
                body=encoded_body,
                headers=headers,
            )

            response = connection.getresponse()

            response_body = response.read().decode(
                "utf-8",
                errors="replace",
            )

            response_headers = {
                key.lower(): value
                for key, value
                in response.getheaders()
            }

            return (
                response.status,
                response_headers,
                response_body,
            )
        finally:
            connection.close()

    def expect(
        self,
        name: str,
        response: tuple[
            int,
            dict[str, str],
            str,
        ],
        expected_status: int,
        expected_fragment: str = "",
        expected_json: dict[
            str,
            Any,
        ] | None = None,
    ) -> None:
        actual_status, _, body = response

        require(
            actual_status == expected_status,
            f"{name}:expected_status="
            f"{expected_status}:actual="
            f"{actual_status}",
        )

        if expected_fragment:
            require(
                expected_fragment in body,
                f"{name}:missing="
                f"{expected_fragment}",
            )

        if expected_json is not None:
            require(
                response_matches(
                    body,
                    expected_json,
                ),
                f"{name}:unexpected_json",
            )

        self.tests_passed += 1

    def preflight(self) -> None:
        require(
            run_command(
                "git",
                "branch",
                "--show-current",
            )
            == self.arguments.expected_branch,
            "unexpected_branch",
        )

        require(
            run_command(
                "git",
                "rev-parse",
                "HEAD",
            )
            == self.arguments.expected_head,
            "unexpected_head",
        )

        require(
            run_command(
                "git",
                "status",
                "--porcelain",
                "--untracked-files=all",
            )
            == "",
            "worktree_not_clean",
        )

        require(
            run_command(
                "systemctl",
                "show",
                self.arguments.service,
                "--property=ActiveState",
                "--value",
            )
            == "active",
            "service_not_active",
        )

        require(
            run_command(
                "systemctl",
                "show",
                self.arguments.service,
                "--property=SubState",
                "--value",
            )
            == "running",
            "service_not_running",
        )

        self.service_pid = int(
            run_command(
                "systemctl",
                "show",
                self.arguments.service,
                "--property=MainPID",
                "--value",
            )
        )

        require(
            self.service_pid > 0,
            "invalid_service_pid",
        )

        self.start_pid = self.service_pid

        require(
            sha256(
                Path(self.arguments.daemon)
            )
            == self.arguments.expected_daemon_sha256,
            "installed_daemon_hash_mismatch",
        )

        require(
            sha256(
                Path(
                    f"/proc/{self.service_pid}/exe"
                )
            )
            == self.arguments.expected_daemon_sha256,
            "running_daemon_hash_mismatch",
        )

        require(
            sha256(
                Path(self.arguments.loader)
            )
            == self.arguments.expected_loader_sha256,
            "installed_loader_hash_mismatch",
        )

        verify_backup(
            Path(self.arguments.backup_dir)
        )

        require(
            self.database.execute(
                "PRAGMA quick_check"
            ).fetchone()[0]
            == "ok",
            "database_quick_check_failed",
        )

        require(
            not list(
                self.database.execute(
                    "PRAGMA foreign_key_check"
                )
            ),
            "database_foreign_key_violation",
        )

        environment: dict[str, str] = {}

        with open(
            f"/proc/{self.service_pid}/environ",
            "rb",
        ) as handle:
            for item in handle.read().split(
                b"\0"
            ):
                if b"=" not in item:
                    continue

                key, value = item.split(
                    b"=",
                    1,
                )

                environment[
                    key.decode(
                        "utf-8",
                        errors="strict",
                    )
                ] = value.decode(
                    "utf-8",
                    errors="strict",
                )

        self.authorization = environment.get(
            "VDR_SUITE_BASIC_AUTH",
            "",
        )

        require(
            self.authorization.startswith(
                "Basic "
            ),
            "basic_authorization_unavailable",
        )

    def resource_snapshot(self) -> str:
        status, _, body = self.request(
            "GET",
            self.manifest["snapshot"]["path"],
            basic=True,
        )

        self.expect(
            "resource_snapshot",
            (status, {}, body),
            200,
        )

        try:
            payload = json.loads(body)
        except json.JSONDecodeError as error:
            raise AcceptanceError(
                "resource_snapshot_invalid_json"
            ) from error

        return json.dumps(
            payload,
            sort_keys=True,
            separators=(",", ":"),
        )

    def issue_browser_session(self) -> None:
        active_before = {
            row[0]
            for row in self.database.execute(
                """
                SELECT session_id
                FROM security_sessions
                WHERE active = 1
                """
            )
        }

        status, headers, body = self.request(
            "POST",
            "/api/security/browser-sessions",
            basic=True,
        )

        require(
            status == 200,
            f"browser_login_status={status}",
        )

        try:
            payload = json.loads(body)
        except json.JSONDecodeError as error:
            raise AcceptanceError(
                "browser_login_invalid_json"
            ) from error

        csrf_token = payload.get(
            "csrfToken",
            "",
        )

        require(
            isinstance(csrf_token, str)
            and bool(csrf_token),
            "browser_login_missing_csrf",
        )

        cookie_match = re.search(
            r"(?:^|;\s*)"
            r"vdr_suite_session=([^;]+)",
            headers.get("set-cookie", ""),
        )

        require(
            cookie_match is not None,
            "browser_login_missing_cookie",
        )

        self.csrf_token = csrf_token
        self.cookie = cookie_match.group(1)

        active_after = list(
            self.database.execute(
                """
                SELECT
                    session_id,
                    actor_id
                FROM security_sessions
                WHERE active = 1
                """
            )
        )

        created = [
            row
            for row in active_after
            if row[0] not in active_before
        ]

        require(
            len(created) == 1,
            "browser_session_not_unique",
        )

        self.session_id = str(created[0][0])
        self.actor_id = str(created[0][1])

        self.grant_state = GrantState(
            self.database,
            self.actor_id,
            target_grants(self.manifest),
        )

        self.grant_state.capture_and_neutralize()
        self.tests_passed += 1

    def verify_accountability(
        self,
    ) -> dict[str, int]:
        rows = list(
            self.database.execute(
                """
                SELECT
                    event_type,
                    actor_id,
                    session_id,
                    authentication_state,
                    permission,
                    backend_id,
                    request_id,
                    correlation_id,
                    action,
                    decision,
                    reason_code,
                    outcome
                FROM accountability_events
                WHERE request_id LIKE ?
                ORDER BY rowid
                """,
                (
                    self.request_prefix + "%",
                ),
            )
        )

        actual = summarize_accountability(
            rows,
            permission=self.manifest["permission"],
            action=self.manifest["action"],
            backend_id=self.manifest["backendId"],
            route_count=len(self.manifest["routes"]),
        )

        forbidden_values = (
            self.authorization,
            self.authorization.removeprefix(
                "Basic "
            ),
            self.cookie,
            self.csrf_token,
        )

        for row in rows:
            combined = "\n".join(
                ""
                if value is None
                else str(value)
                for value in row
            )

            require(
                all(
                    not secret
                    or secret not in combined
                    for secret in forbidden_values
                ),
                "secret_in_accountability",
            )

        self.tests_passed += 1
        return actual

    def logout(self) -> None:
        if not self.cookie or not self.csrf_token:
            return

        status, _, _ = self.request(
            "POST",
            "/api/security/browser-sessions/logout",
            browser=True,
            csrf=True,
        )

        require(
            status == 204,
            f"browser_logout_status={status}",
        )

        row = self.database.execute(
            """
            SELECT active
            FROM security_sessions
            WHERE session_id = ?
            """,
            (self.session_id,),
        ).fetchone()

        require(
            row is not None and row[0] == 0,
            "browser_session_still_active",
        )

        replay_status, _, replay_body = (
            self.request(
                "GET",
                "/api/vdr/status",
                browser=True,
            )
        )

        require(
            replay_status == 401
            and "credential_revoked"
            in replay_body,
            "revoked_cookie_replay_not_denied",
        )

        self.tests_passed += 2

    def run(self) -> dict[str, int]:
        self.preflight()

        resource_before = (
            self.resource_snapshot()
        )

        routes = self.manifest["routes"]
        safe_body = self.manifest["safeBody"]

        expected_validation = self.manifest[
            "expectedValidation"
        ]

        permission = self.manifest["permission"]
        backend_id = self.manifest["backendId"]

        alternate_backend_id = self.manifest[
            "alternateBackendId"
        ]

        for route in routes:
            self.expect(
                "unauthenticated",
                self.request(
                    "POST",
                    route,
                    body=safe_body,
                ),
                401,
            )

        for route in routes:
            self.expect(
                "legacy_basic_safe_validation",
                self.request(
                    "POST",
                    route,
                    basic=True,
                    body=safe_body,
                ),
                expected_validation["status"],
                expected_json=(
                    expected_validation["json"]
                ),
            )

        self.issue_browser_session()

        require(
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
            alternate_backend_id,
            True,
        )

        for route in routes:
            self.expect(
                "wrong_backend_scope",
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
            alternate_backend_id,
            False,
        )

        self.grant_state.set_active(
            permission,
            backend_id,
            True,
        )

        for route in routes:
            self.expect(
                "direct_permission",
                self.request(
                    "POST",
                    route,
                    browser=True,
                    csrf=True,
                    body=safe_body,
                ),
                expected_validation["status"],
                expected_json=(
                    expected_validation["json"]
                ),
            )

            self.expect(
                "query_string",
                self.request(
                    "POST",
                    route
                    + self.manifest[
                        "querySuffix"
                    ],
                    browser=True,
                    csrf=True,
                    body=safe_body,
                ),
                expected_validation["status"],
                expected_json=(
                    expected_validation["json"]
                ),
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
            backend_id,
            False,
        )

        self.grant_state.set_active(
            "role.admin",
            "*",
            True,
        )

        for route in routes:
            self.expect(
                "wildcard_admin",
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
            "*",
            False,
        )

        self.grant_state.set_active(
            "role.admin",
            backend_id,
            True,
        )

        for route in routes:
            self.expect(
                "admin_role",
                self.request(
                    "POST",
                    route,
                    browser=True,
                    csrf=True,
                    body=safe_body,
                ),
                expected_validation["status"],
                expected_json=(
                    expected_validation["json"]
                ),
            )

        self.grant_state.set_active(
            "role.read-only",
            backend_id,
            True,
        )

        for route in routes:
            self.expect(
                "read_only_precedence",
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

        resource_after = (
            self.resource_snapshot()
        )

        require(
            resource_after == resource_before,
            "resource_state_changed",
        )

        self.tests_passed += 1
        return counts

    def cleanup(self) -> list[str]:
        errors: list[str] = []

        try:
            if self.grant_state is not None:
                self.grant_state.restore()
        except Exception as error:
            errors.append(
                "grant_restore:"
                + type(error).__name__
            )

        try:
            self.logout()
        except Exception as error:
            errors.append(
                "session_cleanup:"
                + type(error).__name__
            )

        return errors

    def final_verify(self) -> None:
        require(
            run_command(
                "systemctl",
                "show",
                self.arguments.service,
                "--property=MainPID",
                "--value",
            )
            == str(self.start_pid),
            "service_restarted",
        )

        require(
            run_command(
                "systemctl",
                "show",
                self.arguments.service,
                "--property=ActiveState",
                "--value",
            )
            == "active",
            "service_not_active_after_run",
        )

        require(
            run_command(
                "systemctl",
                "show",
                self.arguments.service,
                "--property=SubState",
                "--value",
            )
            == "running",
            "service_not_running_after_run",
        )

        require(
            sha256(
                Path(self.arguments.daemon)
            )
            == self.arguments.expected_daemon_sha256,
            "installed_daemon_changed",
        )

        require(
            sha256(
                Path(
                    f"/proc/{self.start_pid}/exe"
                )
            )
            == self.arguments.expected_daemon_sha256,
            "running_daemon_changed",
        )

        require(
            sha256(
                Path(self.arguments.loader)
            )
            == self.arguments.expected_loader_sha256,
            "installed_loader_changed",
        )

        require(
            self.database.execute(
                "PRAGMA quick_check"
            ).fetchone()[0]
            == "ok",
            "database_quick_check_failed_after_run",
        )

        require(
            not list(
                self.database.execute(
                    "PRAGMA foreign_key_check"
                )
            ),
            "foreign_key_violation_after_run",
        )

        require(
            run_command(
                "git",
                "status",
                "--porcelain",
                "--untracked-files=all",
            )
            == "",
            "worktree_changed",
        )

        verify_backup(
            Path(self.arguments.backup_dir)
        )


def self_test(
    manifest: dict[str, Any],
) -> None:
    errors = (
        validate_manifest(manifest)
        + validate_repository_routes(manifest)
    )

    require(
        not errors,
        ";".join(errors),
    )

    require(
        response_matches(
            json.dumps(
                manifest[
                    "expectedValidation"
                ]["json"]
            ),
            manifest[
                "expectedValidation"
            ]["json"],
        ),
        "response_match_self_test_failed",
    )

    synthetic_rows: list[tuple[Any, ...]] = []

    def add_accountability_rows(
        count: int,
        reason_code: str,
        outcome: str,
    ) -> None:
        for index in range(count):
            synthetic_rows.append(
                (
                    "authorization.allowed"
                    if outcome == "dispatch_authorized"
                    else "authorization.denied",
                    "actor",
                    "session",
                    "authenticated",
                    manifest["permission"],
                    manifest["backendId"],
                    f"self-test-{reason_code}-{index}",
                    "",
                    manifest["action"],
                    "allowed"
                    if outcome == "dispatch_authorized"
                    else "denied",
                    reason_code,
                    outcome,
                )
            )

    route_count = len(manifest["routes"])

    add_accountability_rows(
        route_count,
        "csrf_validation_failed",
        "dispatch_denied",
    )
    add_accountability_rows(
        route_count,
        "permission_denied",
        "dispatch_denied",
    )
    add_accountability_rows(
        route_count * 2,
        "backend_scope_denied",
        "dispatch_denied",
    )
    add_accountability_rows(
        route_count,
        "role_read_only",
        "dispatch_denied",
    )
    add_accountability_rows(
        route_count * 4,
        "permission_granted",
        "dispatch_authorized",
    )

    synthetic_rows.append(
        (
            "authorization.allowed",
            "actor",
            "session",
            "authenticated",
            "unrelated.permission",
            manifest["backendId"],
            "self-test-unrelated",
            "",
            "unrelated.action",
            "allowed",
            "permission_granted",
            "dispatch_authorized",
        )
    )

    summary = summarize_accountability(
        synthetic_rows,
        permission=manifest["permission"],
        action=manifest["action"],
        backend_id=manifest["backendId"],
        route_count=route_count,
    )

    require(
        summary == {
            "csrf": route_count,
            "permission": route_count,
            "scope": route_count * 2,
            "read_only": route_count,
            "authorized": route_count * 4,
        },
        "accountability_summary_self_test_failed",
    )

    count_mismatch_rows = list(
        synthetic_rows
    )

    for index, row in enumerate(
        count_mismatch_rows
    ):
        if row[4] == manifest["permission"]:
            del count_mismatch_rows[index]
            break
    else:
        raise AcceptanceError(
            "accountability_negative_count_fixture_missing"
        )

    try:
        summarize_accountability(
            count_mismatch_rows,
            permission=manifest["permission"],
            action=manifest["action"],
            backend_id=manifest["backendId"],
            route_count=route_count,
        )
    except AcceptanceError as error:
        require(
            "accountability_count_mismatch"
            in str(error),
            "accountability_count_failure_reason_mismatch",
        )
    else:
        raise AcceptanceError(
            "accountability_count_mismatch_not_detected"
        )

    contract_mismatch_rows = list(
        synthetic_rows
    )

    for index, row in enumerate(
        contract_mismatch_rows
    ):
        if row[4] == manifest["permission"]:
            changed = list(row)
            changed[5] = "unexpected-backend"
            contract_mismatch_rows[index] = tuple(
                changed
            )
            break
    else:
        raise AcceptanceError(
            "accountability_negative_contract_fixture_missing"
        )

    try:
        summarize_accountability(
            contract_mismatch_rows,
            permission=manifest["permission"],
            action=manifest["action"],
            backend_id=manifest["backendId"],
            route_count=route_count,
        )
    except AcceptanceError as error:
        require(
            "accountability_contract_mismatch"
            in str(error),
            "accountability_contract_failure_reason_mismatch",
        )
    else:
        raise AcceptanceError(
            "accountability_contract_mismatch_not_detected"
        )

    database = sqlite3.connect(
        ":memory:",
        isolation_level=None,
    )

    database.execute(
        """
        CREATE TABLE
            security_actor_permission_grants
            (
                actor_id TEXT NOT NULL,
                permission TEXT NOT NULL,
                backend_id TEXT NOT NULL,
                active INTEGER NOT NULL
                    DEFAULT 1,
                revoked_at TEXT NOT NULL
                    DEFAULT '',
                created_at TEXT NOT NULL
                    DEFAULT CURRENT_TIMESTAMP,
                updated_at TEXT NOT NULL
                    DEFAULT CURRENT_TIMESTAMP,
                PRIMARY KEY(
                    actor_id,
                    permission,
                    backend_id
                )
            )
        """
    )

    database.execute(
        """
        INSERT INTO
            security_actor_permission_grants
            (
                actor_id,
                permission,
                backend_id
            )
        VALUES (
            'actor',
            'remote.control',
            'default'
        )
        """
    )

    grant_state = GrantState(
        database,
        "actor",
        target_grants(manifest),
    )

    grant_state.capture_and_neutralize()

    grant_state.set_active(
        manifest["permission"],
        manifest["backendId"],
        True,
    )

    grant_state.restore()

    unrelated_grant = database.execute(
        """
        SELECT active
        FROM security_actor_permission_grants
        WHERE actor_id = 'actor'
          AND permission = 'remote.control'
          AND backend_id = 'default'
        """
    ).fetchone()

    require(
        unrelated_grant == (1,),
        "unrelated_grant_changed",
    )

    database.close()

    with tempfile.TemporaryDirectory() as directory:
        backup = Path(directory)
        sample = backup / "sample"

        sample.write_text(
            "ok\n",
            encoding="utf-8",
        )

        (backup / "SHA256SUMS").write_text(
            f"{sha256(sample)}  sample\n",
            encoding="utf-8",
        )

        verify_backup(backup)


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description=(
            "Validate or run mutation-safe "
            "Phase 62 runtime acceptance."
        )
    )

    parser.add_argument(
        "--manifest",
        default=str(DEFAULT_MANIFEST),
    )

    mode = parser.add_mutually_exclusive_group(
        required=True
    )

    mode.add_argument(
        "--validate-only",
        action="store_true",
    )

    mode.add_argument(
        "--self-test",
        action="store_true",
    )

    mode.add_argument(
        "--run",
        action="store_true",
    )

    parser.add_argument(
        "--base-url",
        default="http://127.0.0.1:18080",
    )

    parser.add_argument(
        "--database",
        default=(
            "/var/lib/vdr-suite/"
            "vdr-suite.db"
        ),
    )

    parser.add_argument(
        "--service",
        default="vdr-suite-daemon.service",
    )

    parser.add_argument(
        "--daemon",
        default="/usr/sbin/vdr-suite-daemon",
    )

    parser.add_argument(
        "--loader",
        default=(
            "/usr/share/vdr-suite/"
            "web/frontend/platform/"
            "deferred-runtime-loader.js"
        ),
    )

    parser.add_argument(
        "--backup-dir",
        default="",
    )

    parser.add_argument(
        "--expected-branch",
        default="",
    )

    parser.add_argument(
        "--expected-head",
        default="",
    )

    parser.add_argument(
        "--expected-daemon-sha256",
        default="",
    )

    parser.add_argument(
        "--expected-loader-sha256",
        default="",
    )

    parser.add_argument(
        "--timeout",
        type=int,
        default=10,
    )

    parser.add_argument(
        "--report-json",
        default="",
    )

    return parser.parse_args()


def main() -> int:
    arguments = parse_arguments()

    manifest = load_manifest(
        Path(arguments.manifest)
    )

    errors = (
        validate_manifest(manifest)
        + validate_repository_routes(manifest)
    )

    if errors:
        for error in errors:
            print(f"validation_error={error}")

        return 1

    if arguments.validate_only:
        print(
            "phase62_runtime_acceptance_"
            "manifest=valid"
        )
        return 0

    if arguments.self_test:
        self_test(manifest)
        print(
            "phase62_runtime_acceptance_"
            "self_test=passed"
        )
        return 0

    for argument_name in (
        "backup_dir",
        "expected_branch",
        "expected_head",
        "expected_daemon_sha256",
        "expected_loader_sha256",
    ):
        require(
            bool(
                getattr(
                    arguments,
                    argument_name,
                )
            ),
            "missing_run_argument:"
            f"{argument_name}",
        )

    acceptance = RuntimeAcceptance(
        arguments,
        manifest,
    )

    counts: dict[str, int] = {}
    error_message = ""

    try:
        counts = acceptance.run()
    except Exception as error:
        error_message = (
            f"{type(error).__name__}:"
            f"{error}"
        )

    cleanup_errors = acceptance.cleanup()

    if cleanup_errors:
        error_message = (
            error_message + ";"
            if error_message
            else ""
        ) + ",".join(cleanup_errors)

    try:
        acceptance.final_verify()
    except Exception as error:
        error_message = (
            error_message + ";"
            if error_message
            else ""
        ) + (
            "final_verify:"
            f"{type(error).__name__}:"
            f"{error}"
        )

    report = {
        "schemaVersion": 1,
        "manifestId": manifest["id"],
        "passed": not error_message,
        "testsPassed": (
            acceptance.tests_passed
        ),
        "httpRequests": acceptance.sequence,
        "accountability": counts,
        "error": error_message,
    }

    if arguments.report_json:
        report_path = Path(
            arguments.report_json
        )

        report_path.parent.mkdir(
            parents=True,
            exist_ok=True,
        )

        report_path.write_text(
            json.dumps(
                report,
                indent=2,
                sort_keys=True,
            )
            + "\n",
            encoding="utf-8",
        )

    acceptance.database.close()

    if error_message:
        print(
            "phase62_runtime_acceptance_error="
            f"{error_message}"
        )
        print(
            "runtime_http_requests="
            f"{acceptance.sequence}"
        )
        return 1

    print(f"slice={manifest['id']}")
    print(
        "tests_passed="
        f"{acceptance.tests_passed}"
    )
    print("tests_failed=0")
    print(
        "runtime_http_requests="
        f"{acceptance.sequence}"
    )

    for key, value in sorted(
        counts.items()
    ):
        print(
            f"accountability_{key}={value}"
        )

    print("resource_state_unchanged=yes")
    print("target_grants_restored=yes")
    print("unrelated_grants_untouched=yes")
    print("browser_session_revoked=yes")
    print("revoked_cookie_replay_denied=yes")
    print("accountability_secret_free=yes")
    print("database_integrity=yes")
    print("service_pid_unchanged=yes")
    print(
        "phase62_runtime_acceptance=passed"
    )

    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except AcceptanceError as error:
        print(
            "phase62_runtime_acceptance_error="
            f"{error}"
        )
        raise SystemExit(1)
