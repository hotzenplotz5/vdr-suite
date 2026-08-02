#!/usr/bin/env python3
from __future__ import annotations

import importlib.util
import shlex
import sys
import tempfile
import time
from pathlib import Path
from types import ModuleType

from idle_expiry_audit_contract import adapt_accountability_rows
from idle_expiry_systemd_override import (
    install_runtime_override,
    remove_runtime_override,
    runtime_override_path,
)

DEFAULT_SERVICE = "vdr-suite-daemon.service"
SYSTEMD_RUNTIME_ROOT = Path("/run/systemd/system")
IDLE_ENVIRONMENT_KEY = "VDR_SUITE_BROWSER_SESSION_IDLE_TIMEOUT_SECONDS"
IDLE_ENVIRONMENT_VALUE = f"{IDLE_ENVIRONMENT_KEY}=300"


def implementation_path() -> Path:
    return Path(__file__).with_name("idle-expiry-runner.py")


def load_implementation(path: Path) -> ModuleType:
    specification = importlib.util.spec_from_file_location(
        "phase62_idle_expiry_runner_implementation",
        path,
    )
    if specification is None or specification.loader is None:
        raise RuntimeError("idle_expiry_runner_load_failed")

    module = importlib.util.module_from_spec(specification)
    sys.modules[specification.name] = module
    specification.loader.exec_module(module)
    return module


def argument_value(
    arguments: list[str],
    name: str,
    default: str = "",
) -> str:
    for index, argument in enumerate(arguments):
        if argument.startswith(name + "="):
            return argument.split("=", 1)[1]
        if argument == name and index + 1 < len(arguments):
            return arguments[index + 1]
    return default


def service_argument(arguments: list[str]) -> str:
    return argument_value(arguments, "--service", DEFAULT_SERVICE) or DEFAULT_SERVICE


def install_audit_contract(implementation: ModuleType) -> None:
    original_reader = implementation.accountability_rows

    def validated_reader(database, request_ids):
        return adapt_accountability_rows(
            original_reader,
            implementation.require,
            database,
            request_ids,
        )

    implementation.accountability_rows = validated_reader


def systemd_unit_environment(
    implementation: ModuleType,
    service: str,
) -> set[str]:
    raw = implementation.run(
        Path.cwd(),
        "systemctl",
        "show",
        "--property=Environment",
        "--value",
        service,
    )
    try:
        return set(shlex.split(raw))
    except ValueError as error:
        raise implementation.AcceptanceError(
            "systemd_environment_parse_failed"
        ) from error


def install_environment_validation_contract(
    implementation: ModuleType,
    service: str,
    *,
    environment_reader=None,
) -> None:
    original_require = implementation.require

    def read_environment() -> set[str]:
        if environment_reader is not None:
            return set(environment_reader())
        return systemd_unit_environment(implementation, service)

    def validated_require(condition: bool, message: str) -> None:
        if message == "idle_configuration_not_applied":
            original_require(
                IDLE_ENVIRONMENT_VALUE in read_environment(),
                "idle_runtime_override_not_loaded",
            )
            return

        if message == "idle_configuration_not_restored":
            original_require(
                IDLE_ENVIRONMENT_VALUE not in read_environment(),
                "idle_runtime_override_not_removed",
            )
            return

        original_require(condition, message)

    implementation.require = validated_require


def running_daemon_observation(
    implementation: ModuleType,
    service: str,
) -> tuple[str, int, str]:
    state = implementation.run(
        Path.cwd(),
        "systemctl",
        "is-active",
        service,
        check=False,
    )
    try:
        pid = implementation.service_pid(Path.cwd(), service)
    except Exception:
        return state or "unknown", 0, "unreadable"

    if pid <= 0:
        return state or "unknown", pid, "missing"

    try:
        executable_hash = implementation.sha256(Path(f"/proc/{pid}/exe"))
    except OSError:
        executable_hash = "unreadable"
    return state or "unknown", pid, executable_hash


def install_running_daemon_validation_contract(
    implementation: ModuleType,
    service: str,
    expected_sha256: str,
    *,
    observer=None,
    sleep_callback=time.sleep,
    max_attempts: int = 40,
    required_stable_matches: int = 4,
) -> None:
    original_require = implementation.require
    guarded_messages = {
        "running_new_daemon_mismatch",
        "final_running_daemon_mismatch",
    }

    def observe() -> tuple[str, int, str]:
        if observer is not None:
            return observer()
        return running_daemon_observation(implementation, service)

    def wait_for_expected_running_daemon(message: str) -> None:
        stable_pid = 0
        stable_matches = 0
        last_state = "unknown"
        last_pid = 0
        last_hash = "unreadable"

        for attempt in range(max_attempts):
            last_state, last_pid, last_hash = observe()
            if (
                last_state == "active"
                and last_pid > 0
                and last_hash == expected_sha256
            ):
                if last_pid == stable_pid:
                    stable_matches += 1
                else:
                    stable_pid = last_pid
                    stable_matches = 1
                if stable_matches >= required_stable_matches:
                    return
            else:
                stable_pid = 0
                stable_matches = 0

            if attempt + 1 < max_attempts:
                sleep_callback(0.25)

        raise implementation.AcceptanceError(
            f"{message}:state={last_state}:pid={last_pid}:"
            f"sha256={last_hash}"
        )

    def validated_require(condition: bool, message: str) -> None:
        if message in guarded_messages and not condition:
            wait_for_expected_running_daemon(message)
            return
        original_require(condition, message)

    implementation.require = validated_require


def install_systemd_override_contract(
    implementation: ModuleType,
    service: str,
    *,
    runtime_root: Path = SYSTEMD_RUNTIME_ROOT,
    reload_callback=None,
):
    override = runtime_override_path(runtime_root, service)
    original_restore = implementation.restore_exact
    configured_path: Path | None = None
    active = False

    def reload_systemd(check: bool) -> None:
        if reload_callback is not None:
            reload_callback(check)
            return
        implementation.run(
            Path.cwd(),
            "systemctl",
            "daemon-reload",
            check=check,
        )

    def helper_call(function, *arguments) -> None:
        try:
            function(*arguments)
        except RuntimeError as error:
            raise implementation.AcceptanceError(str(error)) from error

    def remove_override(check: bool) -> None:
        nonlocal active
        helper_call(remove_runtime_override, override)
        reload_systemd(check)
        active = False

    def write_idle_runtime_override(path: Path, seconds: int) -> None:
        nonlocal active, configured_path
        implementation.require(
            not active and not override.exists() and not override.is_symlink(),
            "idle_runtime_override_already_present",
        )
        configured_path = path.resolve()
        helper_call(install_runtime_override, override, seconds)
        try:
            reload_systemd(True)
        except Exception:
            try:
                helper_call(remove_runtime_override, override)
            except Exception:
                pass
            try:
                reload_systemd(False)
            except Exception:
                pass
            configured_path = None
            raise
        active = True

    def restore_with_runtime_override_removed(
        source: Path,
        destination: Path,
    ) -> None:
        nonlocal configured_path
        if (
            configured_path is not None
            and destination.resolve() == configured_path
        ):
            if active or override.exists() or override.is_symlink():
                remove_override(True)
            configured_path = None
        original_restore(source, destination)

    def cleanup() -> None:
        nonlocal active
        if active or override.exists() or override.is_symlink():
            try:
                remove_runtime_override(override)
            except Exception:
                pass
            try:
                reload_systemd(False)
            except Exception:
                pass
        active = False

    implementation.write_idle_config = write_idle_runtime_override
    implementation.restore_exact = restore_with_runtime_override_removed
    return cleanup


def make_fake_implementation() -> ModuleType:
    fake = ModuleType("phase62_idle_override_self_test")
    fake.AcceptanceError = RuntimeError

    def fake_require(condition: bool, message: str) -> None:
        if not condition:
            raise RuntimeError(message)

    fake.require = fake_require
    fake.run = lambda *arguments, **keywords: ""
    return fake


def self_test_environment_validation_contract() -> None:
    sequence = iter(
        (
            {IDLE_ENVIRONMENT_VALUE},
            set(),
        )
    )
    fake = make_fake_implementation()
    install_environment_validation_contract(
        fake,
        DEFAULT_SERVICE,
        environment_reader=lambda: next(sequence),
    )
    fake.require(False, "idle_configuration_not_applied")
    fake.require(False, "idle_configuration_not_restored")

    missing = make_fake_implementation()
    install_environment_validation_contract(
        missing,
        DEFAULT_SERVICE,
        environment_reader=set,
    )
    try:
        missing.require(False, "idle_configuration_not_applied")
    except RuntimeError as error:
        if str(error) != "idle_runtime_override_not_loaded":
            raise
    else:
        raise RuntimeError("loader_self_test_missing_override_accepted")

    retained = make_fake_implementation()
    install_environment_validation_contract(
        retained,
        DEFAULT_SERVICE,
        environment_reader=lambda: {IDLE_ENVIRONMENT_VALUE},
    )
    try:
        retained.require(False, "idle_configuration_not_restored")
    except RuntimeError as error:
        if str(error) != "idle_runtime_override_not_removed":
            raise
    else:
        raise RuntimeError("loader_self_test_retained_override_accepted")


def self_test_running_daemon_validation_contract() -> None:
    expected = "phase62-daemon"
    successful_observations = iter(
        (
            ("active", 101, "other-daemon"),
            ("active", 202, expected),
            ("active", 202, expected),
            ("active", 202, expected),
            ("active", 202, expected),
        )
    )
    recovered = make_fake_implementation()
    install_running_daemon_validation_contract(
        recovered,
        DEFAULT_SERVICE,
        expected,
        observer=lambda: next(successful_observations),
        sleep_callback=lambda seconds: None,
        max_attempts=5,
        required_stable_matches=4,
    )
    recovered.require(False, "final_running_daemon_mismatch")

    persistent = make_fake_implementation()
    install_running_daemon_validation_contract(
        persistent,
        DEFAULT_SERVICE,
        expected,
        observer=lambda: ("active", 303, "wrong-daemon"),
        sleep_callback=lambda seconds: None,
        max_attempts=2,
        required_stable_matches=2,
    )
    try:
        persistent.require(False, "final_running_daemon_mismatch")
    except RuntimeError as error:
        if str(error) != (
            "final_running_daemon_mismatch:state=active:pid=303:"
            "sha256=wrong-daemon"
        ):
            raise
    else:
        raise RuntimeError("loader_self_test_persistent_mismatch_accepted")


def self_test_systemd_override_contract() -> None:
    with tempfile.TemporaryDirectory(prefix="phase62-idle-entry-") as directory:
        root = Path(directory)
        runtime_root = root / "run/systemd/system"
        configuration = root / "etc/default/vdr-suite-daemon"
        source = root / "configuration.before"
        reloads: list[bool] = []
        restores: list[tuple[Path, Path]] = []

        fake = make_fake_implementation()
        fake.write_idle_config = lambda path, seconds: None
        fake.restore_exact = lambda old, new: restores.append((old, new))

        cleanup = install_systemd_override_contract(
            fake,
            DEFAULT_SERVICE,
            runtime_root=runtime_root,
            reload_callback=lambda check: reloads.append(check),
        )
        override = runtime_override_path(runtime_root, DEFAULT_SERVICE)

        fake.write_idle_config(configuration, 300)
        if not override.is_file():
            raise RuntimeError("loader_self_test_override_missing")
        fake.restore_exact(source, configuration)
        if override.exists() or override.is_symlink():
            raise RuntimeError("loader_self_test_override_not_removed")
        if restores != [(source, configuration)]:
            raise RuntimeError("loader_self_test_restore_not_forwarded")
        if reloads != [True, True]:
            raise RuntimeError("loader_self_test_reload_contract_mismatch")
        cleanup()


def self_test_loader() -> int:
    implementation = load_implementation(implementation_path())
    for attribute in (
        "AcceptanceError",
        "accountability_rows",
        "main",
        "require",
        "restore_exact",
        "run",
        "service_pid",
        "sha256",
        "write_idle_config",
    ):
        if not hasattr(implementation, attribute):
            raise RuntimeError(
                f"idle_expiry_runner_attribute_missing:{attribute}"
            )
    install_audit_contract(implementation)
    if implementation.accountability_rows.__name__ != "validated_reader":
        raise RuntimeError("idle_expiry_audit_contract_not_installed")
    self_test_environment_validation_contract()
    self_test_running_daemon_validation_contract()
    self_test_systemd_override_contract()
    print("PHASE_62_IDLE_RUNNER_LOADER_SELF_TEST=PASS")
    return 0


def main() -> int:
    implementation = load_implementation(implementation_path())
    service = service_argument(sys.argv[1:])
    expected_new_daemon_sha256 = argument_value(
        sys.argv[1:],
        "--expected-new-daemon-sha256",
    )
    if not expected_new_daemon_sha256:
        print("PHASE_62_SLICE_2V_RUNTIME_ACCEPTANCE=FAIL")
        print("FAILURE_REASON=expected_new_daemon_sha256_missing")
        return 1

    install_audit_contract(implementation)
    install_environment_validation_contract(implementation, service)
    install_running_daemon_validation_contract(
        implementation,
        service,
        expected_new_daemon_sha256,
    )
    cleanup_override = install_systemd_override_contract(
        implementation,
        service,
    )

    try:
        try:
            return int(implementation.main())
        except implementation.AcceptanceError as error:
            print("PHASE_62_SLICE_2V_RUNTIME_ACCEPTANCE=FAIL")
            print(f"FAILURE_REASON={error}")
            return 1
    finally:
        cleanup_override()


if __name__ == "__main__":
    if sys.argv[1:] == ["--self-test-loader"]:
        raise SystemExit(self_test_loader())
    raise SystemExit(main())
