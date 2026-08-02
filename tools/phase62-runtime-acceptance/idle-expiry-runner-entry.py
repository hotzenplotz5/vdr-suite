#!/usr/bin/env python3
from __future__ import annotations

import importlib.util
import sys
import tempfile
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


def service_argument(arguments: list[str]) -> str:
    for index, argument in enumerate(arguments):
        if argument.startswith("--service="):
            value = argument.split("=", 1)[1]
            return value or DEFAULT_SERVICE
        if argument == "--service" and index + 1 < len(arguments):
            return arguments[index + 1]
    return DEFAULT_SERVICE


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


def self_test_systemd_override_contract() -> None:
    with tempfile.TemporaryDirectory(prefix="phase62-idle-entry-") as directory:
        root = Path(directory)
        runtime_root = root / "run/systemd/system"
        configuration = root / "etc/default/vdr-suite-daemon"
        source = root / "configuration.before"
        reloads: list[bool] = []
        restores: list[tuple[Path, Path]] = []

        fake = ModuleType("phase62_idle_override_self_test")
        fake.AcceptanceError = RuntimeError

        def fake_require(condition: bool, message: str) -> None:
            if not condition:
                raise RuntimeError(message)

        fake.require = fake_require
        fake.write_idle_config = lambda path, seconds: None
        fake.restore_exact = lambda old, new: restores.append((old, new))
        fake.run = lambda *arguments, **keywords: ""

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
        "write_idle_config",
    ):
        if not hasattr(implementation, attribute):
            raise RuntimeError(
                f"idle_expiry_runner_attribute_missing:{attribute}"
            )
    install_audit_contract(implementation)
    if implementation.accountability_rows.__name__ != "validated_reader":
        raise RuntimeError("idle_expiry_audit_contract_not_installed")
    self_test_systemd_override_contract()
    print("PHASE_62_IDLE_RUNNER_LOADER_SELF_TEST=PASS")
    return 0


def main() -> int:
    implementation = load_implementation(implementation_path())
    install_audit_contract(implementation)
    cleanup_override = install_systemd_override_contract(
        implementation,
        service_argument(sys.argv[1:]),
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
