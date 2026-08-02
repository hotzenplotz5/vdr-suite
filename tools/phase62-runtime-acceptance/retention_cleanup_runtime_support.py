#!/usr/bin/env python3
from __future__ import annotations

import argparse
import hashlib
import http.client
import json
import os
import shutil
import sqlite3
import stat
import subprocess
import sys
import tempfile
import time
from contextlib import closing
from datetime import datetime, timedelta, timezone
from pathlib import Path

from retention_cleanup_systemd_override import (
    DATABASE_KEY,
    IDLE_KEY,
    RETENTION_KEY,
    SECURITY_DATABASE_KEY,
    remove_runtime_override,
    runtime_override_path,
    write_runtime_override,
)

SYSTEMD_RUNTIME_ROOT = Path("/run/systemd/system")
DEFAULT_SERVICE = "vdr-suite-daemon.service"
RETENTION_SECONDS = 86400
IDLE_SECONDS = 300
BATCH_SIZE = 256


class AcceptanceError(RuntimeError):
    pass


def require(condition: bool, message: str) -> None:
    if not condition:
        raise AcceptanceError(message)


def run(root: Path, *arguments: str, check: bool = True) -> str:
    completed = subprocess.run(
        arguments,
        cwd=root,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        check=False,
    )
    if check and completed.returncode != 0:
        command = arguments[0] if arguments else "unknown"
        raise AcceptanceError(f"command_failed:{command}")
    return completed.stdout.strip()


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def atomic_copy(source: Path, destination: Path, mode: int) -> None:
    temporary = destination.with_name(destination.name + ".slice2w-new")
    shutil.copyfile(source, temporary)
    os.chmod(temporary, mode)
    os.chown(temporary, 0, 0)
    os.replace(temporary, destination)


def restore_exact(source: Path, destination: Path) -> None:
    temporary = destination.with_name(destination.name + ".slice2w-restore")
    shutil.copyfile(source, temporary)
    os.chmod(temporary, stat.S_IMODE(source.stat().st_mode))
    os.chown(temporary, 0, 0)
    os.replace(temporary, destination)


def database_connection(path: Path) -> sqlite3.Connection:
    database = sqlite3.connect(
        str(path),
        timeout=10,
        isolation_level=None,
    )
    database.execute("PRAGMA busy_timeout=10000")
    database.execute("PRAGMA foreign_keys=ON")
    return database


def verify_database(database: sqlite3.Connection) -> tuple[str, int]:
    quick = str(database.execute("PRAGMA quick_check").fetchone()[0])
    foreign_keys = database.execute("PRAGMA foreign_key_check").fetchall()
    require(quick == "ok", "sqlite_quick_check_failed")
    require(not foreign_keys, "sqlite_foreign_key_check_failed")
    return quick, len(foreign_keys)


def backup_database(source_path: Path, destination: Path) -> None:
    source = sqlite3.connect(f"file:{source_path}?mode=ro", uri=True)
    target = sqlite3.connect(str(destination))
    try:
        source.backup(target)
    finally:
        target.close()
        source.close()


def clone_database(source: Path, destination: Path) -> None:
    require(not destination.exists(), "scenario_database_already_exists")
    shutil.copy2(source, destination)
    os.chmod(destination, 0o600)


def parse_env_file(path: Path) -> dict[str, str]:
    values: dict[str, str] = {}
    for raw in path.read_text(encoding="utf-8").splitlines():
        line = raw.strip()
        if not line or line.startswith("#") or "=" not in line:
            continue
        key, value = line.split("=", 1)
        value = value.strip()
        if (
            len(value) >= 2
            and value[0] == value[-1]
            and value[0] in ("'", '"')
        ):
            value = value[1:-1]
        values[key.strip()] = value
    return values


def service_pid(root: Path, service: str) -> int:
    return int(
        run(
            root,
            "systemctl",
            "show",
            "-p",
            "MainPID",
            "--value",
            service,
        )
        or "0"
    )


def wait_service(root: Path, service: str, timeout: float = 20.0) -> int:
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        if (
            run(
                root,
                "systemctl",
                "is-active",
                service,
                check=False,
            )
            == "active"
        ):
            pid = service_pid(root, service)
            if pid > 0:
                return pid
        time.sleep(0.25)
    raise AcceptanceError("service_did_not_become_active")


def stop_service(root: Path, service: str) -> None:
    run(root, "systemctl", "stop", service)
    deadline = time.monotonic() + 20.0
    while time.monotonic() < deadline:
        state = run(
            root,
            "systemctl",
            "is-active",
            service,
            check=False,
        )
        if state in ("inactive", "failed", "unknown"):
            return
        time.sleep(0.25)
    raise AcceptanceError("service_did_not_stop")


def request(port: int) -> tuple[int, str]:
    connection = http.client.HTTPConnection(
        "127.0.0.1",
        port,
        timeout=5,
    )
    try:
        connection.request(
            "GET",
            "/api/backends",
            headers={
                "Accept": "application/json",
                "Connection": "close",
                "X-Request-ID": "phase62-s2w-runtime-readiness",
            },
        )
        response = connection.getresponse()
        body = response.read().decode("utf-8", errors="replace")
        return response.status, body
    finally:
        connection.close()


def error_code(body: str) -> str:
    try:
        decoded = json.loads(body)
    except json.JSONDecodeError:
        return ""
    if not isinstance(decoded, dict):
        return ""
    error = decoded.get("error")
    if not isinstance(error, dict):
        return ""
    code = error.get("code")
    return code if isinstance(code, str) else ""


def wait_http(
    port: int,
    *,
    expect_unavailable: bool,
    timeout: float = 20.0,
) -> int:
    deadline = time.monotonic() + timeout
    last_status = 0
    last_code = ""
    while time.monotonic() < deadline:
        try:
            last_status, body = request(port)
            last_code = error_code(body)
            unavailable = (
                last_status == 503
                and last_code == "security_runtime_unavailable"
            )
            if unavailable == expect_unavailable:
                return last_status
        except OSError:
            pass
        time.sleep(0.25)
    raise AcceptanceError(
        "http_runtime_state_mismatch:"
        f"status={last_status}:code={last_code or 'none'}"
    )


def process_environment(pid: int) -> set[bytes]:
    return set(Path(f"/proc/{pid}/environ").read_bytes().split(b"\0"))


def verify_runtime_process(
    root: Path,
    service: str,
    expected_daemon_sha256: str,
    database_path: Path,
    retention_seconds: int,
    idle_seconds: int,
) -> int:
    pid = wait_service(root, service)
    require(
        sha256(Path(f"/proc/{pid}/exe")) == expected_daemon_sha256,
        "running_new_daemon_mismatch",
    )
    environment = process_environment(pid)
    expected = {
        f"{DATABASE_KEY}={database_path}".encode(),
        f"{SECURITY_DATABASE_KEY}={database_path}".encode(),
        f"{RETENTION_KEY}={retention_seconds}".encode(),
        f"{IDLE_KEY}={idle_seconds}".encode(),
    }
    require(expected.issubset(environment), "runtime_override_not_applied")
    return pid


def timestamp(offset_seconds: int) -> str:
    return (
        datetime.now(timezone.utc) + timedelta(seconds=offset_seconds)
    ).strftime("%Y-%m-%d %H:%M:%S")


def required_security_schema(database: sqlite3.Connection) -> None:
    required_tables = {
        "security_actors",
        "security_devices",
        "security_sessions",
        "security_credentials",
        "security_browser_session_credentials",
        "security_actor_permission_grants",
        "accountability_events",
    }
    actual = {
        str(row[0])
        for row in database.execute(
            "SELECT name FROM sqlite_master WHERE type = 'table'"
        )
    }
    require(required_tables.issubset(actual), "security_schema_missing")
    browser_columns = {
        str(row[1])
        for row in database.execute(
            "PRAGMA table_info(security_browser_session_credentials)"
        )
    }
    require("last_seen_at" in browser_columns, "last_seen_schema_missing")


def provision_identity(
    database: sqlite3.Connection,
    prefix: str,
) -> dict[str, str]:
    actor_id = prefix + "-actor"
    device_id = prefix + "-device"
    issuer_id = prefix + "-issuer"
    database.execute("BEGIN IMMEDIATE")
    try:
        database.execute(
            """
            INSERT INTO security_actors
                (actor_id, actor_type, display_name, active, revoked_at)
            VALUES (?, 'user', ?, 1, '')
            """,
            (actor_id, "Phase 62 Slice 2W runtime actor"),
        )
        database.execute(
            """
            INSERT INTO security_devices
                (device_id, actor_id, display_name, active, revoked_at)
            VALUES (?, ?, ?, 1, '')
            """,
            (device_id, actor_id, "Phase 62 Slice 2W runtime device"),
        )
        database.execute(
            """
            INSERT INTO security_credentials
                (
                    credential_id,
                    actor_id,
                    credential_type,
                    active,
                    expires_at,
                    revoked_at
                )
            VALUES (?, ?, 'runtime-acceptance-issuer', 1, '', '')
            """,
            (issuer_id, actor_id),
        )
        database.execute(
            """
            INSERT INTO security_actor_permission_grants
                (actor_id, permission, backend_id, active, revoked_at)
            VALUES (?, 'recordings.view', 'default', 1, '')
            """,
            (actor_id,),
        )
        database.execute("COMMIT")
    except Exception:
        database.execute("ROLLBACK")
        raise
    return {
        "actor_id": actor_id,
        "device_id": device_id,
        "issuer_id": issuer_id,
    }


def create_lifecycle(
    database: sqlite3.Connection,
    identity: dict[str, str],
    label: str,
    *,
    verifier_expires_at: str,
    last_seen_at: str,
    revoked_at: str = "",
    credential_type: str = "browser-session",
    session_id: str = "",
    credential_id: str = "",
    create_canonical: bool = True,
) -> dict[str, str]:
    session_id = session_id or label + "-session"
    credential_id = credential_id or label + "-credential"
    token_id = label + "-token"
    canonical_expiry = timestamp(7200)

    database.execute("BEGIN IMMEDIATE")
    try:
        if create_canonical:
            database.execute(
                """
                INSERT INTO security_sessions
                    (
                        session_id,
                        actor_id,
                        device_id,
                        active,
                        expires_at,
                        revoked_at
                    )
                VALUES (?, ?, ?, 1, ?, '')
                """,
                (
                    session_id,
                    identity["actor_id"],
                    identity["device_id"],
                    canonical_expiry,
                ),
            )
            database.execute(
                """
                INSERT INTO security_credentials
                    (
                        credential_id,
                        actor_id,
                        credential_type,
                        active,
                        expires_at,
                        revoked_at
                    )
                VALUES (?, ?, ?, 1, ?, '')
                """,
                (
                    credential_id,
                    identity["actor_id"],
                    credential_type,
                    canonical_expiry,
                ),
            )
        database.execute(
            """
            INSERT INTO security_browser_session_credentials
                (
                    token_id,
                    session_id,
                    actor_id,
                    device_id,
                    credential_id,
                    issued_from_credential_id,
                    session_secret_hash,
                    csrf_secret_hash,
                    active,
                    expires_at,
                    last_seen_at,
                    revoked_at
                )
            VALUES (?, ?, ?, ?, ?, ?, ?, ?, 1, ?, ?, ?)
            """,
            (
                token_id,
                session_id,
                identity["actor_id"],
                identity["device_id"],
                credential_id,
                identity["issuer_id"],
                "$6$phase62-s2w-runtime-session",
                "$6$phase62-s2w-runtime-csrf",
                verifier_expires_at,
                last_seen_at,
                revoked_at,
            ),
        )
        database.execute("COMMIT")
    except Exception:
        database.execute("ROLLBACK")
        raise
    return {
        "token_id": token_id,
        "session_id": session_id,
        "credential_id": credential_id,
    }


def revoke_issuer(
    database: sqlite3.Connection,
    issuer_id: str,
) -> None:
    database.execute(
        """
        UPDATE security_credentials
        SET active = 0,
            revoked_at = ?,
            updated_at = CURRENT_TIMESTAMP
        WHERE credential_id = ?
        """,
        (timestamp(-90000), issuer_id),
    )


def verifier_exists(database: sqlite3.Connection, token_id: str) -> bool:
    return (
        database.execute(
            """
            SELECT 1
            FROM security_browser_session_credentials
            WHERE token_id = ?
            """,
            (token_id,),
        ).fetchone()
        is not None
    )


def session_exists(database: sqlite3.Connection, session_id: str) -> bool:
    return (
        database.execute(
            "SELECT 1 FROM security_sessions WHERE session_id = ?",
            (session_id,),
        ).fetchone()
        is not None
    )


def credential_type(
    database: sqlite3.Connection,
    credential_id: str,
) -> str:
    row = database.execute(
        """
        SELECT credential_type
        FROM security_credentials
        WHERE credential_id = ?
        """,
        (credential_id,),
    ).fetchone()
    return "" if row is None else str(row[0])


def assert_lifecycle(
    database: sqlite3.Connection,
    lifecycle: dict[str, str],
    *,
    verifier: bool,
    session: bool,
    credential: bool,
) -> None:
    require(
        verifier_exists(database, lifecycle["token_id"]) == verifier,
        "unexpected_verifier_state:" + lifecycle["token_id"],
    )
    require(
        session_exists(database, lifecycle["session_id"]) == session,
        "unexpected_session_state:" + lifecycle["session_id"],
    )
    require(
        bool(credential_type(database, lifecycle["credential_id"]))
        == credential,
        "unexpected_credential_state:" + lifecycle["credential_id"],
    )


def cleanup_events(
    database: sqlite3.Connection,
    actor_id: str,
) -> list[tuple[object, ...]]:
    return database.execute(
        """
        SELECT
            event_type,
            classes,
            actor_type,
            session_id,
            authentication_state,
            action,
            decision,
            reason_code,
            outcome,
            request_id,
            event_id,
            actor_id,
            device_id
        FROM accountability_events
        WHERE actor_id = ?
          AND action = 'browser.session.cleanup'
        ORDER BY recorded_at, event_id
        """,
        (actor_id,),
    ).fetchall()


def assert_cleanup_events(
    events: list[tuple[object, ...]],
    expected_count: int,
) -> None:
    require(
        len(events) == expected_count,
        f"cleanup_accountability_count_mismatch:{len(events)}",
    )
    for row in events:
        require(
            str(row[0]) == "operation.succeeded",
            "cleanup_event_type_mismatch",
        )
        require(
            str(row[1]) == "security,lifecycle,maintenance",
            "cleanup_event_classes_mismatch",
        )
        require(str(row[2]) == "system", "cleanup_actor_type_mismatch")
        require(
            str(row[4]) == "system-maintenance",
            "cleanup_authentication_state_mismatch",
        )
        require(
            str(row[5]) == "browser.session.cleanup",
            "cleanup_action_mismatch",
        )
        require(str(row[6]) == "completed", "cleanup_decision_mismatch")
        require(
            str(row[7]) == "browser_session_retention_elapsed",
            "cleanup_reason_mismatch",
        )
        require(str(row[8]) == "deleted", "cleanup_outcome_mismatch")
        require(bool(str(row[9])), "cleanup_request_id_missing")
        require(
            str(row[9]) == str(row[10]),
            "cleanup_request_event_id_mismatch",
        )

    serialized = "\n".join(
        "\t".join("" if value is None else str(value) for value in row)
        for row in events
    )
    for forbidden in (
        "$6$",
        "phase62-s2w-runtime-session",
        "phase62-s2w-runtime-csrf",
        "Authorization",
        "Cookie",
        "vdr_suite_session=",
        "X-CSRF-Token",
    ):
        require(
            forbidden not in serialized,
            "cleanup_accountability_contains_secret",
        )


def assert_identity_preserved(
    database: sqlite3.Connection,
    identity: dict[str, str],
    *,
    issuer_revoked: bool,
) -> None:
    actor = database.execute(
        "SELECT active, revoked_at FROM security_actors WHERE actor_id = ?",
        (identity["actor_id"],),
    ).fetchone()
    device = database.execute(
        "SELECT active, revoked_at FROM security_devices WHERE device_id = ?",
        (identity["device_id"],),
    ).fetchone()
    issuer = database.execute(
        """
        SELECT credential_type, active, revoked_at
        FROM security_credentials
        WHERE credential_id = ?
        """,
        (identity["issuer_id"],),
    ).fetchone()
    grant = database.execute(
        """
        SELECT active, revoked_at
        FROM security_actor_permission_grants
        WHERE actor_id = ?
          AND permission = 'recordings.view'
          AND backend_id = 'default'
        """,
        (identity["actor_id"],),
    ).fetchone()
    require(
        actor is not None and int(actor[0]) == 1 and str(actor[1]) == "",
        "actor_not_preserved",
    )
    require(
        device is not None and int(device[0]) == 1 and str(device[1]) == "",
        "device_not_preserved",
    )
    require(issuer is not None, "issuer_not_preserved")
    require(
        str(issuer[0]) == "runtime-acceptance-issuer",
        "issuer_type_changed",
    )
    if issuer_revoked:
        require(
            int(issuer[1]) == 0 and str(issuer[2]) != "",
            "issuer_revocation_not_preserved",
        )
    else:
        require(
            int(issuer[1]) == 1 and str(issuer[2]) == "",
            "issuer_state_changed",
        )
    require(
        grant is not None and int(grant[0]) == 1 and str(grant[1]) == "",
        "grant_not_preserved",
    )
