#!/usr/bin/env python3
from __future__ import annotations

import os
import re
import sys
import tempfile
from pathlib import Path

RETENTION_KEY = "VDR_SUITE_BROWSER_SESSION_RETENTION_SECONDS"
IDLE_KEY = "VDR_SUITE_BROWSER_SESSION_IDLE_TIMEOUT_SECONDS"
DATABASE_KEY = "VDR_SUITE_DATABASE_PATH"
SECURITY_DATABASE_KEY = "VDR_SUITE_SECURITY_DATABASE_PATH"
DROP_IN_NAME = "phase62-retention-runtime-acceptance.conf"
SERVICE_PATTERN = re.compile(r"[A-Za-z0-9_.@:-]+")


def require(condition: bool, message: str) -> None:
    if not condition:
        raise RuntimeError(message)


def runtime_override_path(runtime_root: Path, service: str) -> Path:
    require(bool(SERVICE_PATTERN.fullmatch(service)), "invalid_systemd_service_name")
    return runtime_root / f"{service}.d" / DROP_IN_NAME


def _environment_line(key: str, value: str) -> str:
    require(
        "\x00" not in value and "\r" not in value and "\n" not in value,
        "invalid_systemd_environment_value",
    )
    escaped = value.replace("\\", "\\\\").replace('"', '\\"')
    return f'Environment="{key}={escaped}"\n'


def render_runtime_override(
    database_path: Path,
    retention_seconds: int,
    idle_seconds: int,
) -> str:
    require(database_path.is_absolute(), "database_path_must_be_absolute")
    require(isinstance(retention_seconds, int), "retention_must_be_integer")
    require(
        retention_seconds == 0 or 86400 <= retention_seconds <= 31536000,
        "retention_out_of_runtime_range",
    )
    require(isinstance(idle_seconds, int), "idle_timeout_must_be_integer")
    require(
        idle_seconds == 0 or 300 <= idle_seconds <= 86400,
        "idle_timeout_out_of_runtime_range",
    )
    database = str(database_path)
    return (
        "[Service]\n"
        + _environment_line(DATABASE_KEY, database)
        + _environment_line(SECURITY_DATABASE_KEY, database)
        + _environment_line(RETENTION_KEY, str(retention_seconds))
        + _environment_line(IDLE_KEY, str(idle_seconds))
    )


def write_runtime_override(
    path: Path,
    database_path: Path,
    retention_seconds: int,
    idle_seconds: int,
) -> None:
    require(not path.is_symlink(), "retention_runtime_override_is_symlink")
    require(not path.is_dir(), "retention_runtime_override_is_directory")
    path.parent.mkdir(mode=0o755, parents=True, exist_ok=True)

    temporary = path.with_name(path.name + ".new")
    require(
        not temporary.exists() and not temporary.is_symlink(),
        "retention_runtime_override_temporary_exists",
    )

    flags = os.O_WRONLY | os.O_CREAT | os.O_EXCL
    if hasattr(os, "O_NOFOLLOW"):
        flags |= os.O_NOFOLLOW

    descriptor = os.open(temporary, flags, 0o644)
    try:
        with os.fdopen(descriptor, "w", encoding="utf-8") as handle:
            descriptor = -1
            handle.write(
                render_runtime_override(
                    database_path,
                    retention_seconds,
                    idle_seconds,
                )
            )
            handle.flush()
            os.fsync(handle.fileno())
        os.replace(temporary, path)
    finally:
        if descriptor >= 0:
            os.close(descriptor)
        if temporary.exists() or temporary.is_symlink():
            temporary.unlink()


def remove_runtime_override(path: Path) -> None:
    if path.exists() or path.is_symlink():
        require(not path.is_dir(), "retention_runtime_override_is_directory")
        path.unlink()
    try:
        path.parent.rmdir()
    except OSError:
        pass


def self_test() -> int:
    with tempfile.TemporaryDirectory(
        prefix="phase62-retention-systemd-"
    ) as directory:
        runtime_root = Path(directory) / "run/systemd/system"
        database = Path(directory) / "acceptance.sqlite"
        path = runtime_override_path(
            runtime_root,
            "vdr-suite-daemon.service",
        )
        write_runtime_override(path, database.resolve(), 0, 300)
        expected = (
            "[Service]\n"
            f'Environment="{DATABASE_KEY}={database.resolve()}"\n'
            f'Environment="{SECURITY_DATABASE_KEY}={database.resolve()}"\n'
            f'Environment="{RETENTION_KEY}=0"\n'
            f'Environment="{IDLE_KEY}=300"\n'
        )
        require(
            path.read_text(encoding="utf-8") == expected,
            "self_test_override_content_mismatch",
        )
        write_runtime_override(path, database.resolve(), 86400, 300)
        require(
            f'Environment="{RETENTION_KEY}=86400"'
            in path.read_text(encoding="utf-8"),
            "self_test_override_replace_failed",
        )
        remove_runtime_override(path)
        require(not path.exists(), "self_test_override_not_removed")

    print("PHASE_62_RETENTION_SYSTEMD_OVERRIDE_SELF_TEST=PASS")
    return 0


if __name__ == "__main__":
    if sys.argv[1:] != ["--self-test"]:
        raise SystemExit(
            "usage: retention_cleanup_systemd_override.py --self-test"
        )
    raise SystemExit(self_test())
