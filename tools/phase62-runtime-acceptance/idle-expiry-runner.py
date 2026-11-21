#!/usr/bin/env python3
from __future__ import annotations

import argparse
import hashlib
import http.client
import json
import os
import secrets
import shutil
import sqlite3
import stat
import subprocess
import sys
import time
import warnings
from contextlib import closing
from datetime import datetime, timedelta, timezone
from pathlib import Path

warnings.filterwarnings("ignore", category=DeprecationWarning)
import crypt  # noqa: E402


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


def service_pid(root: Path, service: str) -> int:
    value = run(
        root,
        "systemctl",
        "show",
        "-p",
        "MainPID",
        "--value",
        service,
    )
    return int(value or "0")


def wait_service(root: Path, service: str, timeout: float = 20.0) -> int:
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        if run(
            root,
            "systemctl",
            "is-active",
            service,
            check=False,
        ) == "active":
            pid = service_pid(root, service)
            if pid > 0:
                return pid
        time.sleep(0.25)
    raise AcceptanceError("service_did_not_become_active")


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


def write_idle_config(path: Path, seconds: int) -> None:
    original = path.read_text(encoding="utf-8")
    kept = [
        line
        for line in original.splitlines()
        if not line.lstrip().startswith(
            "VDR_SUITE_BROWSER_SESSION_IDLE_TIMEOUT_SECONDS="
        )
    ]
    content = "\n".join(kept)
    if content:
        content += "\n"
    content += (
        "VDR_SUITE_BROWSER_SESSION_IDLE_TIMEOUT_SECONDS="
        f"{seconds}\n"
    )

    metadata = path.stat()
    temporary = path.with_name(path.name + ".slice2v-new")
    temporary.write_text(content, encoding="utf-8")
    os.chmod(temporary, stat.S_IMODE(metadata.st_mode))
    os.chown(temporary, metadata.st_uid, metadata.st_gid)
    os.replace(temporary, path)


def atomic_copy(source: Path, destination: Path, mode: int) -> None:
    temporary = destination.with_name(destination.name + ".slice2v-new")
    shutil.copyfile(source, temporary)
    os.chmod(temporary, mode)
    os.chown(temporary, 0, 0)
    os.replace(temporary, destination)


def restore_exact(source: Path, destination: Path) -> None:
    temporary = destination.with_name(destination.name + ".slice2v-restore")
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


def request(
    port: int,
    method: str,
    path: str,
    request_id: str,
    *,
    cookie: str = "",
    csrf: str = "",
    body: dict[str, object] | None = None,
) -> tuple[int, dict[str, str], str]:
    headers = {
        "Accept": "application/json",
        "Connection": "close",
        "X-Request-ID": request_id,
    }
    if cookie:
        headers["Cookie"] = "vdr_suite_session=" + cookie
    if csrf:
        headers["X-CSRF-Token"] = csrf

    payload: str | None = None
    if body is not None:
        payload = json.dumps(body, separators=(",", ":"))
        headers["Content-Type"] = "application/json"

    connection = http.client.HTTPConnection(
        "127.0.0.1",
        port,
        timeout=10,
    )
    try:
        connection.request(method, path, body=payload, headers=headers)
        response = connection.getresponse()
        response_body = response.read().decode("utf-8", errors="replace")
        response_headers = {
            key.lower(): value
            for key, value in response.getheaders()
        }
        return response.status, response_headers, response_body
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


def wait_http(port: int, timeout: float = 20.0) -> None:
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        try:
            status, _, _ = request(
                port,
                "GET",
                "/api/backends",
                "phase62-s2v-readiness",
            )
            if 100 <= status <= 599:
                return
        except OSError:
            pass
        time.sleep(0.25)
    raise AcceptanceError("http_endpoint_did_not_become_ready")


def secret_hash(secret: str) -> str:
    hashed = crypt.crypt(
        secret,
        "$6$" + secrets.token_hex(8) + "$",
    )
    require(bool(hashed) and hashed.startswith("$6$"), "secret_hash_failed")
    return hashed


def future_timestamp(seconds: int) -> str:
    return (
        datetime.now(timezone.utc) + timedelta(seconds=seconds)
    ).strftime("%Y-%m-%d %H:%M:%S")


def provision_identity(
    database: sqlite3.Connection,
    actor_id: str,
    device_id: str,
    issuer_credential_id: str,
) -> None:
    database.execute("BEGIN IMMEDIATE")
    try:
        database.execute(
            """
            INSERT INTO security_actors
                (actor_id, actor_type, display_name, active, revoked_at)
            VALUES (?, 'user', ?, 1, '')
            """,
            (actor_id, "Phase 62 Slice 2V runtime acceptance"),
        )
        database.execute(
            """
            INSERT INTO security_devices
                (device_id, actor_id, display_name, active, revoked_at)
            VALUES (?, ?, ?, 1, '')
            """,
            (device_id, actor_id, "Phase 62 Slice 2V runtime device"),
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
            (issuer_credential_id, actor_id),
        )
        database.execute("COMMIT")
    except Exception:
        database.execute("ROLLBACK")
        raise


def provision_browser_session(
    database: sqlite3.Connection,
    actor_id: str,
    device_id: str,
    issuer_credential_id: str,
    label: str,
) -> dict[str, str]:
    suffix = secrets.token_hex(8)
    session_id = f"{label}-session-{suffix}"
    credential_id = f"{label}-credential-{suffix}"
    token_id = f"{label}-token-{suffix}"
    session_secret = secrets.token_urlsafe(48)
    csrf_secret = secrets.token_urlsafe(48)
    expires_at = future_timestamp(3600)

    database.execute("BEGIN IMMEDIATE")
    try:
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
            (session_id, actor_id, device_id, expires_at),
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
            VALUES (?, ?, 'browser-session', 1, ?, '')
            """,
            (credential_id, actor_id, expires_at),
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
            VALUES (?, ?, ?, ?, ?, ?, ?, ?, 1, ?, CURRENT_TIMESTAMP, '')
            """,
            (
                token_id,
                session_id,
                actor_id,
                device_id,
                credential_id,
                issuer_credential_id,
                secret_hash(session_secret),
                secret_hash(csrf_secret),
                expires_at,
            ),
        )
        database.execute("COMMIT")
    except Exception:
        database.execute("ROLLBACK")
        raise

    return {
        "token_id": token_id,
        "cookie": token_id + "." + session_secret,
        "csrf": csrf_secret,
    }


def browser_row(
    database: sqlite3.Connection,
    token_id: str,
) -> tuple[str, str, int, str]:
    row = database.execute(
        """
        SELECT last_seen_at, expires_at, active, revoked_at
        FROM security_browser_session_credentials
        WHERE token_id = ?
        """,
        (token_id,),
    ).fetchone()
    require(row is not None, "browser_session_row_missing")
    return str(row[0]), str(row[1]), int(row[2]), str(row[3])


def revoke_test_identity(
    database: sqlite3.Connection,
    actor_id: str,
) -> None:
    if not actor_id:
        return
    database.execute("BEGIN IMMEDIATE")
    try:
        for statement in (
            """
            UPDATE security_browser_session_credentials
            SET active = 0,
                revoked_at = CASE
                    WHEN revoked_at = '' THEN CURRENT_TIMESTAMP
                    ELSE revoked_at
                END,
                updated_at = CURRENT_TIMESTAMP
            WHERE actor_id = ?
            """,
            """
            UPDATE security_sessions
            SET active = 0,
                revoked_at = CASE
                    WHEN revoked_at = '' THEN CURRENT_TIMESTAMP
                    ELSE revoked_at
                END,
                updated_at = CURRENT_TIMESTAMP
            WHERE actor_id = ?
            """,
            """
            UPDATE security_credentials
            SET active = 0,
                revoked_at = CASE
                    WHEN revoked_at = '' THEN CURRENT_TIMESTAMP
                    ELSE revoked_at
                END,
                updated_at = CURRENT_TIMESTAMP
            WHERE actor_id = ?
            """,
            """
            UPDATE security_devices
            SET active = 0,
                revoked_at = CASE
                    WHEN revoked_at = '' THEN CURRENT_TIMESTAMP
                    ELSE revoked_at
                END,
                updated_at = CURRENT_TIMESTAMP
            WHERE actor_id = ?
            """,
            """
            UPDATE security_actors
            SET active = 0,
                revoked_at = CASE
                    WHEN revoked_at = '' THEN CURRENT_TIMESTAMP
                    ELSE revoked_at
                END,
                updated_at = CURRENT_TIMESTAMP
            WHERE actor_id = ?
            """,
        ):
            database.execute(statement, (actor_id,))
        database.execute("COMMIT")
    except Exception:
        database.execute("ROLLBACK")
        raise


def active_test_rows(
    database: sqlite3.Connection,
    actor_id: str,
) -> int:
    queries = (
        "SELECT COUNT(*) FROM security_actors "
        "WHERE actor_id = ? AND (active <> 0 OR revoked_at = '')",
        "SELECT COUNT(*) FROM security_devices "
        "WHERE actor_id = ? AND (active <> 0 OR revoked_at = '')",
        "SELECT COUNT(*) FROM security_sessions "
        "WHERE actor_id = ? AND (active <> 0 OR revoked_at = '')",
        "SELECT COUNT(*) FROM security_credentials "
        "WHERE actor_id = ? AND (active <> 0 OR revoked_at = '')",
        "SELECT COUNT(*) FROM security_browser_session_credentials "
        "WHERE actor_id = ? AND (active <> 0 OR revoked_at = '')",
    )
    return sum(
        int(database.execute(query, (actor_id,)).fetchone()[0])
        for query in queries
    )


def accountability_rows(
    database: sqlite3.Connection,
    request_ids: list[str],
) -> list[tuple[str, ...]]:
    placeholders = ",".join("?" for _ in request_ids)
    return database.execute(
        f"""
        SELECT
            event_type,
            actor_id,
            device_id,
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
        WHERE request_id IN ({placeholders})
        ORDER BY recorded_at, event_id
        """,
        request_ids,
    ).fetchall()


def write_report(path: Path, values: list[tuple[str, object]]) -> None:
    path.write_text(
        "".join(f"{key}={value}\n" for key, value in values),
        encoding="utf-8",
    )
    os.chmod(path, 0o600)


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Guarded Phase 62 Slice 2V real-runtime acceptance"
    )
    parser.add_argument("--run", action="store_true")
    parser.add_argument("--repository", default="/home/yavdr/vdr-suite-phase62")
    parser.add_argument("--expected-branch", required=True)
    parser.add_argument("--expected-remote-ref", required=True)
    parser.add_argument("--expected-head", required=True)
    parser.add_argument("--expected-old-daemon-sha256", required=True)
    parser.add_argument("--expected-new-daemon-sha256", required=True)
    parser.add_argument("--expected-config-sha256", required=True)
    parser.add_argument("--expected-loader-sha256", required=True)
    parser.add_argument("--expected-service-pid", required=True, type=int)
    parser.add_argument("--source-ci-run", required=True, type=int)
    parser.add_argument("--source-ci-run-id", required=True, type=int)
    parser.add_argument("--backup-root", default="/var/backups")
    parser.add_argument("--service", default="vdr-suite-daemon.service")
    parser.add_argument("--daemon", default="/usr/sbin/vdr-suite-daemon")
    parser.add_argument("--built-daemon", default=".build/vdr-suite-daemon")
    parser.add_argument("--configuration", default="/etc/default/vdr-suite-daemon")
    parser.add_argument("--database", default="/var/lib/vdr-suite/vdr-suite.db")
    parser.add_argument(
        "--loader",
        default=(
            "/usr/share/vdr-suite/web/frontend/platform/"
            "deferred-runtime-loader.js"
        ),
    )
    return parser.parse_args()


def main() -> int:
    arguments = parse_arguments()
    require(arguments.run, "explicit_run_flag_required")
    require(os.geteuid() == 0, "root_required")

    root = Path(arguments.repository).resolve()
    daemon = Path(arguments.daemon)
    built_daemon = root / arguments.built_daemon
    configuration = Path(arguments.configuration)
    database_path = Path(arguments.database)
    loader = Path(arguments.loader)

    require(Path.cwd().resolve() == root, "unexpected_working_directory")
    require(
        run(root, "git", "branch", "--show-current")
        == arguments.expected_branch,
        "unexpected_branch",
    )
    require(
        run(root, "git", "rev-parse", "HEAD")
        == arguments.expected_head,
        "unexpected_local_head",
    )
    require(
        run(root, "git", "rev-parse", arguments.expected_remote_ref)
        == arguments.expected_head,
        "unexpected_remote_ref",
    )
    require(run(root, "git", "status", "--porcelain") == "", "worktree_not_clean")
    for path, message in (
        (built_daemon, "built_daemon_missing"),
        (daemon, "installed_daemon_missing"),
        (configuration, "daemon_configuration_missing"),
        (database_path, "database_missing"),
        (loader, "deferred_loader_missing_at_canonical_path"),
    ):
        require(path.is_file(), message)

    require(
        sha256(built_daemon) == arguments.expected_new_daemon_sha256,
        "built_daemon_fingerprint_changed",
    )
    require(
        sha256(daemon) == arguments.expected_old_daemon_sha256,
        "installed_daemon_fingerprint_changed",
    )
    require(
        sha256(configuration) == arguments.expected_config_sha256,
        "configuration_fingerprint_changed",
    )
    require(
        sha256(loader) == arguments.expected_loader_sha256,
        "loader_fingerprint_changed",
    )
    require(
        run(root, "systemctl", "is-active", arguments.service) == "active",
        "service_not_active",
    )
    initial_pid = service_pid(root, arguments.service)
    require(initial_pid == arguments.expected_service_pid, "service_pid_changed")
    require(
        sha256(Path(f"/proc/{initial_pid}/exe"))
        == arguments.expected_old_daemon_sha256,
        "running_daemon_fingerprint_changed",
    )

    with closing(database_connection(database_path)) as preflight_database:
        verify_database(preflight_database)

    database_sha_before = sha256(database_path)
    timestamp = datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%SZ")
    evidence = Path(arguments.backup_root) / (
        "vdr-suite-phase62-slice2v-"
        f"{timestamp}-{arguments.expected_head[:12]}"
    )
    evidence.mkdir(mode=0o700, parents=False, exist_ok=False)

    backup_daemon = evidence / "vdr-suite-daemon.before"
    backup_config = evidence / "vdr-suite-daemon.default.before"
    backup_loader = evidence / "deferred-runtime-loader.js.before"
    backup_database_path = evidence / "vdr-suite.before.sqlite"
    report_path = evidence / "runtime-acceptance-report.txt"

    binary_replaced = False
    actor_id = ""
    success = False
    failure_reason = ""

    try:
        run(root, "systemctl", "stop", arguments.service)
        require(
            run(
                root,
                "systemctl",
                "is-active",
                arguments.service,
                check=False,
            ) in ("inactive", "failed"),
            "service_did_not_stop",
        )

        shutil.copy2(daemon, backup_daemon)
        shutil.copy2(configuration, backup_config)
        shutil.copy2(loader, backup_loader)
        backup_database(database_path, backup_database_path)

        with closing(database_connection(backup_database_path)) as backup_db:
            verify_database(backup_db)

        backup_files = (
            backup_daemon,
            backup_config,
            backup_loader,
            backup_database_path,
        )
        checksum_path = evidence / "SHA256SUMS"
        checksum_path.write_text(
            "".join(f"{sha256(path)}  {path.name}\n" for path in backup_files),
            encoding="utf-8",
        )
        os.chmod(checksum_path, 0o600)

        atomic_copy(built_daemon, daemon, 0o755)
        binary_replaced = True
        write_idle_config(configuration, 300)

        run(root, "systemctl", "start", arguments.service)
        runtime_pid = wait_service(root, arguments.service)
        runtime_environment = Path(f"/proc/{runtime_pid}/environ").read_bytes().split(b"\0")
        require(
            b"VDR_SUITE_BROWSER_SESSION_IDLE_TIMEOUT_SECONDS=300"
            in runtime_environment,
            "idle_configuration_not_applied",
        )
        require(
            sha256(daemon) == arguments.expected_new_daemon_sha256,
            "installed_new_daemon_mismatch",
        )
        require(
            sha256(Path(f"/proc/{runtime_pid}/exe"))
            == arguments.expected_new_daemon_sha256,
            "running_new_daemon_mismatch",
        )
        require(
            sha256(loader) == arguments.expected_loader_sha256,
            "loader_changed_during_install",
        )

        configured = parse_env_file(configuration)
        port_text = configured.get("VDR_SUITE_HTTP_PORT", "18080")
        require(port_text.isdigit(), "invalid_http_port")
        port = int(port_text)
        require(1 <= port <= 65535, "invalid_http_port")
        wait_http(port)

        with closing(database_connection(database_path)) as database:
            columns = {
                str(row[1])
                for row in database.execute(
                    "PRAGMA table_info(security_browser_session_credentials)"
                )
            }
            require("last_seen_at" in columns, "last_seen_schema_migration_missing")
            verify_database(database)

            suffix = secrets.token_hex(8)
            actor_id = f"phase62-s2v-actor-{suffix}"
            device_id = f"phase62-s2v-device-{suffix}"
            issuer_id = f"phase62-s2v-issuer-{suffix}"
            provision_identity(database, actor_id, device_id, issuer_id)

            first = provision_browser_session(
                database,
                actor_id,
                device_id,
                issuer_id,
                "phase62-s2v-first",
            )
            absolute_expiry = browser_row(database, first["token_id"])[1]

            database.execute(
                """
                UPDATE security_browser_session_credentials
                SET last_seen_at = datetime(CURRENT_TIMESTAMP, '-61 seconds')
                WHERE token_id = ?
                """,
                (first["token_id"],),
            )
            before_touch = browser_row(database, first["token_id"])[0]

            prefix = f"phase62-s2v-{secrets.token_hex(6)}"
            active_get_id = prefix + "-active-get"
            status, _, _ = request(
                port,
                "GET",
                "/api/backends",
                active_get_id,
                cookie=first["cookie"],
            )
            require(status == 200, f"active_get_status_{status}")
            after_touch, expiry_after_touch, _, _ = browser_row(
                database,
                first["token_id"],
            )
            require(after_touch > before_touch, "last_seen_not_updated_when_due")
            require(
                expiry_after_touch == absolute_expiry,
                "absolute_expiry_changed_after_activity",
            )

            throttle_get_id = prefix + "-throttle-get"
            status, _, _ = request(
                port,
                "GET",
                "/api/backends",
                throttle_get_id,
                cookie=first["cookie"],
            )
            require(status == 200, f"throttle_get_status_{status}")
            after_throttle, expiry_after_throttle, _, _ = browser_row(
                database,
                first["token_id"],
            )
            require(after_throttle == after_touch, "last_seen_not_throttled")
            require(
                expiry_after_throttle == absolute_expiry,
                "absolute_expiry_changed_during_throttle",
            )

            database.execute(
                """
                UPDATE security_browser_session_credentials
                SET last_seen_at = datetime(CURRENT_TIMESTAMP, '-301 seconds')
                WHERE token_id = ?
                """,
                (first["token_id"],),
            )
            idle_timestamp = browser_row(database, first["token_id"])[0]

            idle_get_id = prefix + "-idle-get"
            status, _, body = request(
                port,
                "GET",
                "/api/backends",
                idle_get_id,
                cookie=first["cookie"],
            )
            require(status == 401, f"idle_get_status_{status}")
            require(error_code(body) == "session_expired", "idle_get_wrong_error")

            idle_mutation_id = prefix + "-idle-mutation"
            status, _, body = request(
                port,
                "POST",
                "/api/vdr/remote/actions",
                idle_mutation_id,
                cookie=first["cookie"],
                csrf=first["csrf"],
                body={},
            )
            require(status == 401, f"idle_mutation_status_{status}")
            require(
                error_code(body) == "session_expired",
                "idle_mutation_wrong_error",
            )

            after_idle, expiry_after_idle, _, _ = browser_row(
                database,
                first["token_id"],
            )
            require(after_idle == idle_timestamp, "idle_expiry_updated_last_seen")
            require(
                expiry_after_idle == absolute_expiry,
                "absolute_expiry_changed_after_idle_expiry",
            )

            idle_audit = accountability_rows(
                database,
                [idle_get_id, idle_mutation_id],
            )
            require(len(idle_audit) == 2, "idle_accountability_count_mismatch")
            require(
                all(
                    row[12] == "session_expired"
                    and row[13] == "dispatch_denied"
                    and row[10] == "http.access"
                    for row in idle_audit
                ),
                "idle_accountability_contract_mismatch",
            )

            replacement = provision_browser_session(
                database,
                actor_id,
                device_id,
                issuer_id,
                "phase62-s2v-replacement",
            )
            replacement_expiry = browser_row(database, replacement["token_id"])[1]

            logout_id = prefix + "-logout"
            status, headers, _ = request(
                port,
                "POST",
                "/api/security/browser-sessions/logout",
                logout_id,
                cookie=replacement["cookie"],
                csrf=replacement["csrf"],
            )
            require(status == 204, f"logout_status_{status}")
            require(
                "max-age=0" in headers.get("set-cookie", "").lower(),
                "logout_cookie_not_expired",
            )
            replacement_row = browser_row(database, replacement["token_id"])
            require(
                replacement_row[2] == 0 and replacement_row[3] != "",
                "replacement_browser_not_revoked",
            )
            require(
                replacement_row[1] == replacement_expiry,
                "replacement_absolute_expiry_changed",
            )

            replay_id = prefix + "-replay"
            status, _, body = request(
                port,
                "GET",
                "/api/backends",
                replay_id,
                cookie=replacement["cookie"],
            )
            require(status == 401, f"replay_status_{status}")
            require(error_code(body) == "credential_revoked", "replay_wrong_error")

            logout_audit = accountability_rows(database, [logout_id, replay_id])
            require(len(logout_audit) == 2, "logout_accountability_count_mismatch")
            require(
                any(
                    row[0] == "operation.succeeded"
                    and row[12] == "browser_session_revoked"
                    and row[13] == "succeeded"
                    for row in logout_audit
                ),
                "logout_success_accountability_missing",
            )
            require(
                any(
                    row[12] == "credential_revoked"
                    and row[13] == "dispatch_denied"
                    for row in logout_audit
                ),
                "replay_accountability_missing",
            )

            evidence_rows = accountability_rows(
                database,
                [
                    active_get_id,
                    throttle_get_id,
                    idle_get_id,
                    idle_mutation_id,
                    logout_id,
                    replay_id,
                ],
            )
            serialized = "\n".join(
                "\t".join("" if value is None else str(value) for value in row)
                for row in evidence_rows
            )
            for forbidden in (
                first["cookie"],
                first["csrf"],
                replacement["cookie"],
                replacement["csrf"],
                "Authorization",
                "X-CSRF-Token",
                "vdr_suite_session=",
            ):
                require(
                    forbidden not in serialized,
                    "accountability_contains_secret_material",
                )

            revoke_test_identity(database, actor_id)
            require(
                active_test_rows(database, actor_id) == 0,
                "test_lifecycle_not_fully_revoked",
            )
            quick, foreign_key_violations = verify_database(database)

        restore_exact(backup_config, configuration)
        require(
            sha256(configuration) == arguments.expected_config_sha256,
            "configuration_restore_mismatch",
        )
        run(root, "systemctl", "restart", arguments.service)
        final_pid = wait_service(root, arguments.service)
        final_environment = Path(f"/proc/{final_pid}/environ").read_bytes().split(b"\0")
        require(
            b"VDR_SUITE_BROWSER_SESSION_IDLE_TIMEOUT_SECONDS=300"
            not in final_environment,
            "idle_configuration_not_restored",
        )
        require(
            sha256(daemon) == arguments.expected_new_daemon_sha256,
            "final_daemon_mismatch",
        )
        require(
            sha256(Path(f"/proc/{final_pid}/exe"))
            == arguments.expected_new_daemon_sha256,
            "final_running_daemon_mismatch",
        )
        require(sha256(loader) == arguments.expected_loader_sha256, "final_loader_mismatch")
        require(run(root, "git", "status", "--porcelain") == "", "worktree_changed")

        with closing(database_connection(database_path)) as final_database:
            final_quick, final_foreign_keys = verify_database(final_database)
            require(
                active_test_rows(final_database, actor_id) == 0,
                "final_test_lifecycle_not_revoked",
            )

        report_values: list[tuple[str, object]] = [
            ("PHASE_62_SLICE_2V_RUNTIME_ACCEPTANCE", "PASS"),
            ("head", arguments.expected_head),
            ("source_ci_run", arguments.source_ci_run),
            ("source_ci_run_id", arguments.source_ci_run_id),
            ("daemon_sha256", arguments.expected_new_daemon_sha256),
            ("loader_sha256", arguments.expected_loader_sha256),
            ("idle_timeout_seconds", 300),
            ("last_seen_write_interval_seconds", 60),
            ("ordinary_get_before_idle", 200),
            ("ordinary_get_after_idle", 401),
            ("mutation_after_idle", 401),
            ("idle_error_code", "session_expired"),
            ("last_seen_write_count_in_interval", 1),
            ("absolute_expiry_unchanged", "yes"),
            ("domain_mutations", 0),
            ("logout_status", 204),
            ("replay_status", 401),
            ("replay_error_code", "credential_revoked"),
            ("test_lifecycle_active_rows", 0),
            ("sqlite_quick_check", quick),
            ("sqlite_foreign_key_violations", foreign_key_violations),
            ("accountability_secret_free", "yes"),
            ("configuration_restored_sha256", sha256(configuration)),
            ("database_sha256_before", database_sha_before),
            ("database_sha256_after", sha256(database_path)),
            (
                "final_service_state",
                run(root, "systemctl", "is-active", arguments.service),
            ),
            ("final_service_pid", final_pid),
            ("final_sqlite_quick_check", final_quick),
            ("final_sqlite_foreign_key_violations", final_foreign_keys),
            ("evidence_directory", evidence),
        ]
        write_report(report_path, report_values)
        report_sha = sha256(report_path)
        report_checksum = evidence / "runtime-acceptance-report.sha256"
        report_checksum.write_text(
            f"{report_sha}  {report_path.name}\n",
            encoding="utf-8",
        )
        os.chmod(report_checksum, 0o600)
        success = True

    except Exception as error:
        failure_reason = (
            str(error)
            if isinstance(error, AcceptanceError)
            else error.__class__.__name__
        )

    finally:
        if not success:
            try:
                if actor_id and database_path.is_file():
                    with closing(database_connection(database_path)) as cleanup_database:
                        revoke_test_identity(cleanup_database, actor_id)
            except Exception:
                pass
            try:
                if backup_config.is_file():
                    restore_exact(backup_config, configuration)
            except Exception:
                pass
            try:
                if binary_replaced and backup_daemon.is_file():
                    atomic_copy(backup_daemon, daemon, 0o755)
            except Exception:
                pass
            try:
                run(root, "systemctl", "restart", arguments.service, check=False)
                wait_service(root, arguments.service)
            except Exception:
                pass
            try:
                write_report(
                    report_path,
                    [
                        ("PHASE_62_SLICE_2V_RUNTIME_ACCEPTANCE", "FAIL"),
                        ("head", arguments.expected_head),
                        ("failure_reason", failure_reason or "unknown"),
                        ("evidence_directory", evidence),
                    ],
                )
            except Exception:
                pass

    if not success:
        print("PHASE_62_SLICE_2V_RUNTIME_ACCEPTANCE=FAIL")
        print(f"FAILURE_REASON={failure_reason or 'unknown'}")
        print(f"EVIDENCE={evidence}")
        return 1

    print("PHASE_62_SLICE_2V_RUNTIME_ACCEPTANCE=PASS")
    print(f"HEAD={arguments.expected_head}")
    print(f"DAEMON_SHA256={arguments.expected_new_daemon_sha256}")
    print(f"LOADER_SHA256={arguments.expected_loader_sha256}")
    print(f"CONFIG_RESTORED_SHA256={sha256(configuration)}")
    print(f"RUNTIME_REPORT_SHA256={sha256(report_path)}")
    print(f"EVIDENCE={evidence}")
    print(f"FINAL_SERVICE_PID={service_pid(root, arguments.service)}")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except AcceptanceError as error:
        print("PHASE_62_SLICE_2V_RUNTIME_ACCEPTANCE=FAIL")
        print(f"FAILURE_REASON={error}")
        raise SystemExit(1)
