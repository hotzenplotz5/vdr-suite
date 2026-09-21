#!/usr/bin/env python3
from __future__ import annotations

from retention_cleanup_runtime_execution import execute_acceptance
from retention_cleanup_runtime_support import *  # noqa: F403
from retention_cleanup_runtime_scenarios import *  # noqa: F403


def write_report(path: Path, values: list[tuple[str, object]]) -> None:
    path.write_text(
        "".join(f"{key}={value}\n" for key, value in values),
        encoding="utf-8",
    )
    os.chmod(path, 0o600)


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Guarded Phase 62 Slice 2W real-runtime acceptance"
    )
    parser.add_argument("--run", action="store_true")
    parser.add_argument("--self-test", action="store_true")
    parser.add_argument("--repository", default="/home/yavdr/vdr-suite-phase62")
    parser.add_argument("--expected-branch", default="")
    parser.add_argument("--expected-remote-ref", default="")
    parser.add_argument("--expected-head", default="")
    parser.add_argument("--expected-old-daemon-sha256", default="")
    parser.add_argument("--expected-new-daemon-sha256", default="")
    parser.add_argument("--expected-config-sha256", default="")
    parser.add_argument("--expected-loader-sha256", default="")
    parser.add_argument("--expected-service-pid", type=int, default=0)
    parser.add_argument("--source-ci-run", type=int, default=0)
    parser.add_argument("--source-ci-run-id", type=int, default=0)
    parser.add_argument("--backup-root", default="/var/backups")
    parser.add_argument("--service", default=DEFAULT_SERVICE)
    parser.add_argument("--daemon", default="/usr/sbin/vdr-suite-daemon")
    parser.add_argument("--built-daemon", default=".build/vdr-suite-daemon")
    parser.add_argument(
        "--configuration",
        default="/etc/default/vdr-suite-daemon",
    )
    parser.add_argument(
        "--database",
        default="/var/lib/vdr-suite/vdr-suite.db",
    )
    parser.add_argument(
        "--loader",
        default=(
            "/usr/share/vdr-suite/web/frontend/platform/"
            "deferred-runtime-loader.js"
        ),
    )
    return parser.parse_args()


def self_test() -> int:
    with tempfile.TemporaryDirectory(prefix="phase62-s2w-runner-") as directory:
        path = Path(directory) / "fixture.sqlite"
        with closing(database_connection(path)) as database:
            database.executescript(
                """
                CREATE TABLE security_actors (
                    actor_id TEXT PRIMARY KEY,
                    actor_type TEXT NOT NULL,
                    display_name TEXT NOT NULL,
                    active INTEGER NOT NULL,
                    revoked_at TEXT NOT NULL,
                    created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
                    updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP
                );
                CREATE TABLE security_devices (
                    device_id TEXT PRIMARY KEY,
                    actor_id TEXT NOT NULL,
                    display_name TEXT NOT NULL,
                    active INTEGER NOT NULL,
                    revoked_at TEXT NOT NULL,
                    created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
                    updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP
                );
                CREATE TABLE security_sessions (
                    session_id TEXT PRIMARY KEY,
                    actor_id TEXT NOT NULL,
                    device_id TEXT NOT NULL,
                    active INTEGER NOT NULL,
                    expires_at TEXT NOT NULL,
                    revoked_at TEXT NOT NULL,
                    created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
                    updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP
                );
                CREATE TABLE security_credentials (
                    credential_id TEXT PRIMARY KEY,
                    actor_id TEXT NOT NULL,
                    credential_type TEXT NOT NULL,
                    active INTEGER NOT NULL,
                    expires_at TEXT NOT NULL,
                    revoked_at TEXT NOT NULL,
                    created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
                    updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP
                );
                CREATE TABLE security_browser_session_credentials (
                    token_id TEXT PRIMARY KEY,
                    session_id TEXT NOT NULL UNIQUE,
                    actor_id TEXT NOT NULL,
                    device_id TEXT NOT NULL,
                    credential_id TEXT NOT NULL UNIQUE,
                    issued_from_credential_id TEXT NOT NULL,
                    session_secret_hash TEXT NOT NULL,
                    csrf_secret_hash TEXT NOT NULL,
                    active INTEGER NOT NULL,
                    expires_at TEXT NOT NULL,
                    last_seen_at TEXT NOT NULL,
                    revoked_at TEXT NOT NULL,
                    created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
                    updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP
                );
                CREATE TABLE security_actor_permission_grants (
                    actor_id TEXT NOT NULL,
                    permission TEXT NOT NULL,
                    backend_id TEXT NOT NULL,
                    active INTEGER NOT NULL,
                    revoked_at TEXT NOT NULL,
                    created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
                    updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
                    PRIMARY KEY(actor_id, permission, backend_id)
                );
                CREATE TABLE accountability_events (
                    event_id TEXT PRIMARY KEY,
                    event_type TEXT NOT NULL,
                    classes TEXT NOT NULL,
                    actor_type TEXT NOT NULL,
                    session_id TEXT NOT NULL,
                    authentication_state TEXT NOT NULL,
                    action TEXT NOT NULL,
                    decision TEXT NOT NULL,
                    reason_code TEXT NOT NULL,
                    outcome TEXT NOT NULL,
                    request_id TEXT NOT NULL,
                    actor_id TEXT NOT NULL,
                    device_id TEXT NOT NULL,
                    recorded_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP
                );
                """
            )
            required_security_schema(database)
            identity = provision_identity(database, "phase62-s2w-selftest")
            lifecycle = create_lifecycle(
                database,
                identity,
                "phase62-s2w-selftest-lifecycle",
                verifier_expires_at="2020-01-01 00:00:00",
                last_seen_at=timestamp(0),
            )
            assert_lifecycle(
                database,
                lifecycle,
                verifier=True,
                session=True,
                credential=True,
            )
            assert_identity_preserved(
                database,
                identity,
                issuer_revoked=False,
            )
            verify_database(database)
            enabled_state = prepare_enabled_database(path)
            shared_active = enabled_state["shared_active"]
            require(
                isinstance(shared_active, dict)
                and not verifier_exists(
                    database,
                    shared_active["token_id"],
                ),
                "self_test_replacement_inserted_before_cleanup",
            )
            verify_database(database)
        report = Path(directory) / "report.txt"
        write_report(
            report,
            [("PHASE_62_SLICE_2W_RUNTIME_ACCEPTANCE", "SELF_TEST")],
        )
        require(
            report.stat().st_mode & 0o077 == 0,
            "self_test_report_mode_too_open",
        )
    print("PHASE_62_RETENTION_RUNTIME_RUNNER_SELF_TEST=PASS")
    return 0


def main() -> int:
    arguments = parse_arguments()
    if arguments.self_test:
        return self_test()

    require(arguments.run, "explicit_run_flag_required")
    require(os.geteuid() == 0, "root_required")
    for value, message in (
        (arguments.expected_branch, "expected_branch_missing"),
        (arguments.expected_remote_ref, "expected_remote_ref_missing"),
        (arguments.expected_head, "expected_head_missing"),
        (
            arguments.expected_old_daemon_sha256,
            "expected_old_daemon_sha256_missing",
        ),
        (
            arguments.expected_new_daemon_sha256,
            "expected_new_daemon_sha256_missing",
        ),
        (arguments.expected_config_sha256, "expected_config_sha256_missing"),
        (arguments.expected_loader_sha256, "expected_loader_sha256_missing"),
    ):
        require(bool(value), message)
    require(arguments.expected_service_pid > 0, "expected_service_pid_missing")
    require(arguments.source_ci_run > 0, "source_ci_run_missing")
    require(arguments.source_ci_run_id > 0, "source_ci_run_id_missing")

    root = Path(arguments.repository).resolve()
    daemon = Path(arguments.daemon)
    built_daemon = root / arguments.built_daemon
    configuration = Path(arguments.configuration)
    production_database = Path(arguments.database)
    loader = Path(arguments.loader)
    override = runtime_override_path(
        SYSTEMD_RUNTIME_ROOT,
        arguments.service,
    )

    require(Path.cwd().resolve() == root, "unexpected_working_directory")
    require(
        run(root, "git", "branch", "--show-current")
        == arguments.expected_branch,
        "unexpected_branch",
    )
    require(
        run(root, "git", "rev-parse", "HEAD") == arguments.expected_head,
        "unexpected_local_head",
    )
    require(
        run(root, "git", "rev-parse", arguments.expected_remote_ref)
        == arguments.expected_head,
        "unexpected_remote_ref",
    )
    require(
        run(root, "git", "status", "--porcelain") == "",
        "worktree_not_clean",
    )
    require(
        not override.exists() and not override.is_symlink(),
        "retention_runtime_override_already_present",
    )

    for path, message in (
        (built_daemon, "built_daemon_missing"),
        (daemon, "installed_daemon_missing"),
        (configuration, "daemon_configuration_missing"),
        (production_database, "production_database_missing"),
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
    require(
        initial_pid == arguments.expected_service_pid,
        "service_pid_changed",
    )
    require(
        sha256(Path(f"/proc/{initial_pid}/exe"))
        == arguments.expected_old_daemon_sha256,
        "running_daemon_fingerprint_changed",
    )

    with closing(database_connection(production_database)) as database:
        verify_database(database)

    configured = parse_env_file(configuration)
    port_text = configured.get("VDR_SUITE_HTTP_PORT", "18080")
    require(port_text.isdigit(), "invalid_http_port")
    port = int(port_text)
    require(1 <= port <= 65535, "invalid_http_port")

    return execute_acceptance(
        arguments,
        root,
        daemon,
        built_daemon,
        configuration,
        production_database,
        loader,
        override,
        port,
    )
