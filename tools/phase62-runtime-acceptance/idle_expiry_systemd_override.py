#!/usr/bin/env python3
from __future__ import annotations

import os
import re
import sys
import tempfile
from pathlib import Path

ENVIRONMENT_KEY = "VDR_SUITE_BROWSER_SESSION_IDLE_TIMEOUT_SECONDS"
DROP_IN_NAME = "phase62-idle-runtime-acceptance.conf"
SERVICE_PATTERN = re.compile(r"[A-Za-z0-9_.@:-]+")


def require(condition: bool, message: str) -> None:
    if not condition:
        raise RuntimeError(message)


def runtime_override_path(runtime_root: Path, service: str) -> Path:
    require(bool(SERVICE_PATTERN.fullmatch(service)), "invalid_systemd_service_name")
    return runtime_root / f"{service}.d" / DROP_IN_NAME


def render_runtime_override(seconds: int) -> str:
    require(isinstance(seconds, int), "idle_timeout_must_be_integer")
    require(300 <= seconds <= 86400, "idle_timeout_out_of_runtime_range")
    return (
        "[Service]\n"
        f"Environment={ENVIRONMENT_KEY}={seconds}\n"
    )


def install_runtime_override(path: Path, seconds: int) -> None:
    require(not path.exists() and not path.is_symlink(), "idle_runtime_override_exists")
    path.parent.mkdir(mode=0o755, parents=True, exist_ok=True)

    temporary = path.with_name(path.name + ".new")
    require(
        not temporary.exists() and not temporary.is_symlink(),
        "idle_runtime_override_temporary_exists",
    )

    flags = os.O_WRONLY | os.O_CREAT | os.O_EXCL
    if hasattr(os, "O_NOFOLLOW"):
        flags |= os.O_NOFOLLOW

    descriptor = os.open(temporary, flags, 0o644)
    try:
        with os.fdopen(descriptor, "w", encoding="utf-8") as handle:
            descriptor = -1
            handle.write(render_runtime_override(seconds))
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
        require(not path.is_dir(), "idle_runtime_override_is_directory")
        path.unlink()
    try:
        path.parent.rmdir()
    except OSError:
        pass


def self_test() -> int:
    with tempfile.TemporaryDirectory(prefix="phase62-idle-systemd-") as directory:
        runtime_root = Path(directory) / "run/systemd/system"
        path = runtime_override_path(runtime_root, "vdr-suite-daemon.service")
        install_runtime_override(path, 300)
        require(path.is_file(), "self_test_override_missing")
        require(
            path.read_text(encoding="utf-8")
            == (
                "[Service]\n"
                f"Environment={ENVIRONMENT_KEY}=300\n"
            ),
            "self_test_override_content_mismatch",
        )
        remove_runtime_override(path)
        require(not path.exists(), "self_test_override_not_removed")

    print("PHASE_62_IDLE_SYSTEMD_OVERRIDE_SELF_TEST=PASS")
    return 0


if __name__ == "__main__":
    if sys.argv[1:] != ["--self-test"]:
        raise SystemExit("usage: idle_expiry_systemd_override.py --self-test")
    raise SystemExit(self_test())
