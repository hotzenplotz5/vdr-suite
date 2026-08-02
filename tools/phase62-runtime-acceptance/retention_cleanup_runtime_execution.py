#!/usr/bin/env python3
from __future__ import annotations

from retention_cleanup_runtime_support import *  # noqa: F403
from retention_cleanup_runtime_scenarios import *  # noqa: F403


def write_report(path: Path, values: list[tuple[str, object]]) -> None:
    path.write_text(
        "".join(f"{key}={value}\n" for key, value in values),
        encoding="utf-8",
    )
    os.chmod(path, 0o600)


def execute_acceptance(
    arguments: argparse.Namespace,
    root: Path,
    daemon: Path,
    built_daemon: Path,
    configuration: Path,
    production_database: Path,
    loader: Path,
    override: Path,
    port: int,
) -> int:
    timestamp_text = datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%SZ")
    evidence = Path(arguments.backup_root) / (
        "vdr-suite-phase62-slice2w-"
        f"{timestamp_text}-{arguments.expected_head[:12]}"
    )
    evidence.mkdir(mode=0o700, parents=False, exist_ok=False)

    backup_daemon = evidence / "vdr-suite-daemon.before"
    backup_config = evidence / "vdr-suite-daemon.default.before"
    backup_loader = evidence / "deferred-runtime-loader.js.before"
    backup_production_database = evidence / "production.before.sqlite"
    schema_database = evidence / "schema-initialization.sqlite"
    schema_template = evidence / "schema-template.sqlite"
    disabled_database = evidence / "disabled-policy.sqlite"
    rollback_database = evidence / "rollback-policy.sqlite"
    enabled_database = evidence / "enabled-policy.sqlite"
    bounded_database = evidence / "bounded-policy.sqlite"
    report_path = evidence / "runtime-acceptance-report.txt"

    binary_replaced = False
    success = False
    failure_reason = ""
    scenario_values: dict[str, object] = {}
    production_sha_before = ""

    def install_override(
        database_path: Path,
        retention_seconds: int,
        idle_seconds: int,
    ) -> None:
        write_runtime_override(
            override,
            database_path,
            retention_seconds,
            idle_seconds,
        )
        run(root, "systemctl", "daemon-reload")

    def start_scenario(
        database_path: Path,
        retention_seconds: int,
        *,
        expect_unavailable: bool,
    ) -> tuple[int, int]:
        install_override(
            database_path,
            retention_seconds,
            IDLE_SECONDS,
        )
        run(root, "systemctl", "start", arguments.service)
        pid = verify_runtime_process(
            root,
            arguments.service,
            arguments.expected_new_daemon_sha256,
            database_path,
            retention_seconds,
            IDLE_SECONDS,
        )
        status = wait_http(
            port,
            expect_unavailable=expect_unavailable,
        )
        return pid, status

    try:
        stop_service(root, arguments.service)
        production_sha_before = sha256(production_database)

        shutil.copy2(daemon, backup_daemon)
        shutil.copy2(configuration, backup_config)
        shutil.copy2(loader, backup_loader)
        backup_database(production_database, backup_production_database)
        with closing(database_connection(backup_production_database)) as database:
            verify_database(database)

        checksum_files = (
            backup_daemon,
            backup_config,
            backup_loader,
            backup_production_database,
        )
        checksum_path = evidence / "SHA256SUMS"
        checksum_path.write_text(
            "".join(
                f"{sha256(path)}  {path.name}\n"
                for path in checksum_files
            ),
            encoding="utf-8",
        )
        os.chmod(checksum_path, 0o600)

        atomic_copy(built_daemon, daemon, 0o755)
        binary_replaced = True

        schema_database.touch(mode=0o600, exist_ok=False)
        _, schema_status = start_scenario(
            schema_database,
            0,
            expect_unavailable=False,
        )
        require(schema_status != 503, "schema_runtime_unavailable")
        stop_service(root, arguments.service)
        with closing(database_connection(schema_database)) as database:
            required_security_schema(database)
            schema_quick, schema_foreign_keys = verify_database(database)
        backup_database(schema_database, schema_template)

        clone_database(schema_template, disabled_database)
        disabled_state = prepare_disabled_database(disabled_database)
        _, disabled_status = start_scenario(
            disabled_database,
            0,
            expect_unavailable=False,
        )
        require(disabled_status != 503, "disabled_runtime_unavailable")
        stop_service(root, arguments.service)
        scenario_values.update(
            verify_disabled_database(disabled_database, disabled_state)
        )

        clone_database(schema_template, rollback_database)
        rollback_state = prepare_rollback_database(rollback_database)
        _, rollback_status = start_scenario(
            rollback_database,
            RETENTION_SECONDS,
            expect_unavailable=True,
        )
        require(rollback_status == 503, "rollback_runtime_not_fail_closed")
        stop_service(root, arguments.service)
        scenario_values.update(
            verify_rollback_database(rollback_database, rollback_state)
        )

        clone_database(schema_template, enabled_database)
        enabled_state = prepare_enabled_database(enabled_database)
        _, enabled_status = start_scenario(
            enabled_database,
            RETENTION_SECONDS,
            expect_unavailable=False,
        )
        require(enabled_status != 503, "enabled_runtime_unavailable")
        stop_service(root, arguments.service)
        scenario_values.update(
            verify_enabled_database(enabled_database, enabled_state)
        )

        clone_database(schema_template, bounded_database)
        bounded_state = prepare_bounded_database(bounded_database)
        _, bounded_status = start_scenario(
            bounded_database,
            RETENTION_SECONDS,
            expect_unavailable=False,
        )
        require(bounded_status != 503, "bounded_runtime_unavailable")
        stop_service(root, arguments.service)
        scenario_values.update(
            verify_bounded_database(bounded_database, bounded_state)
        )

        require(
            sha256(production_database) == production_sha_before,
            "production_database_changed_during_isolated_acceptance",
        )
        require(
            sha256(configuration) == arguments.expected_config_sha256,
            "configuration_changed_during_acceptance",
        )
        require(
            sha256(loader) == arguments.expected_loader_sha256,
            "loader_changed_during_acceptance",
        )

        remove_runtime_override(override)
        run(root, "systemctl", "daemon-reload")
        run(root, "systemctl", "start", arguments.service)
        final_pid = wait_service(root, arguments.service)
        require(
            sha256(daemon) == arguments.expected_new_daemon_sha256,
            "final_daemon_mismatch",
        )
        require(
            sha256(Path(f"/proc/{final_pid}/exe"))
            == arguments.expected_new_daemon_sha256,
            "final_running_daemon_mismatch",
        )
        final_environment = process_environment(final_pid)
        scenario_paths = {
            str(path).encode()
            for path in (
                schema_database,
                disabled_database,
                rollback_database,
                enabled_database,
                bounded_database,
            )
        }
        for value in final_environment:
            require(
                not any(path in value for path in scenario_paths),
                "final_runtime_uses_acceptance_database",
            )
        require(
            f"{RETENTION_KEY}={RETENTION_SECONDS}".encode()
            not in final_environment,
            "retention_runtime_override_not_removed",
        )
        require(
            f"{IDLE_KEY}={IDLE_SECONDS}".encode()
            not in final_environment,
            "idle_runtime_override_not_removed",
        )
        require(
            sha256(configuration) == arguments.expected_config_sha256,
            "final_configuration_mismatch",
        )
        require(
            sha256(loader) == arguments.expected_loader_sha256,
            "final_loader_mismatch",
        )
        require(
            run(root, "git", "status", "--porcelain") == "",
            "worktree_changed",
        )
        with closing(database_connection(production_database)) as database:
            final_quick, final_foreign_keys = verify_database(database)

        report_values: list[tuple[str, object]] = [
            ("PHASE_62_SLICE_2W_RUNTIME_ACCEPTANCE", "PASS"),
            ("head", arguments.expected_head),
            ("source_ci_run", arguments.source_ci_run),
            ("source_ci_run_id", arguments.source_ci_run_id),
            ("daemon_sha256", arguments.expected_new_daemon_sha256),
            ("loader_sha256", arguments.expected_loader_sha256),
            ("configuration_sha256", arguments.expected_config_sha256),
            ("retention_seconds", RETENTION_SECONDS),
            ("idle_timeout_seconds", IDLE_SECONDS),
            ("fixed_batch_size", BATCH_SIZE),
            ("schema_http_status", schema_status),
            ("schema_sqlite_quick_check", schema_quick),
            ("schema_foreign_key_violations", schema_foreign_keys),
            ("disabled_http_status", disabled_status),
            ("rollback_http_status", rollback_status),
            ("rollback_error_code", "security_runtime_unavailable"),
            ("enabled_http_status", enabled_status),
            ("bounded_http_status", bounded_status),
            ("production_database_sha256_before", production_sha_before),
            (
                "production_database_sha256_after",
                sha256(production_database),
            ),
            ("production_domain_mutations", 0),
            ("accountability_secret_free", "yes"),
            (
                "final_service_state",
                run(root, "systemctl", "is-active", arguments.service),
            ),
            ("final_service_pid", final_pid),
            ("final_sqlite_quick_check", final_quick),
            ("final_sqlite_foreign_key_violations", final_foreign_keys),
            ("evidence_directory", evidence),
        ]
        report_values.extend(sorted(scenario_values.items()))
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
            else f"{error.__class__.__name__}:{error}"
        )

    finally:
        if not success:
            try:
                stop_service(root, arguments.service)
            except Exception:
                pass
            try:
                remove_runtime_override(override)
                run(root, "systemctl", "daemon-reload", check=False)
            except Exception:
                pass
            try:
                if binary_replaced and backup_daemon.is_file():
                    atomic_copy(backup_daemon, daemon, 0o755)
            except Exception:
                pass
            try:
                if backup_config.is_file():
                    restore_exact(backup_config, configuration)
            except Exception:
                pass
            try:
                run(
                    root,
                    "systemctl",
                    "start",
                    arguments.service,
                    check=False,
                )
                wait_service(root, arguments.service)
            except Exception:
                pass
            try:
                write_report(
                    report_path,
                    [
                        ("PHASE_62_SLICE_2W_RUNTIME_ACCEPTANCE", "FAIL"),
                        ("head", arguments.expected_head),
                        ("failure_reason", failure_reason or "unknown"),
                        ("evidence_directory", evidence),
                    ],
                )
            except Exception:
                pass

    if not success:
        print("PHASE_62_SLICE_2W_RUNTIME_ACCEPTANCE=FAIL")
        print(f"FAILURE_REASON={failure_reason or 'unknown'}")
        print(f"EVIDENCE={evidence}")
        return 1

    print("PHASE_62_SLICE_2W_RUNTIME_ACCEPTANCE=PASS")
    print(f"HEAD={arguments.expected_head}")
    print(f"DAEMON_SHA256={arguments.expected_new_daemon_sha256}")
    print(f"LOADER_SHA256={arguments.expected_loader_sha256}")
    print(f"CONFIGURATION_SHA256={arguments.expected_config_sha256}")
    print(f"RUNTIME_REPORT_SHA256={sha256(report_path)}")
    print(f"EVIDENCE={evidence}")
    print(f"FINAL_SERVICE_PID={service_pid(root, arguments.service)}")
    return 0
