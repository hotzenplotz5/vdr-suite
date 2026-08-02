#!/usr/bin/env python3
from __future__ import annotations

import os
import re
import sys
import time
from pathlib import Path

import retention_cleanup_runtime_support as support

_EXEC_START_PATTERN = re.compile(r"(?:^|[ {;])path=([^ ;}]+)")
_DELETED_SUFFIX = " (deleted)"


def _parse_exec_start_path(value: str) -> Path:
    match = _EXEC_START_PATTERN.search(value)
    support.require(match is not None, "systemd_exec_start_path_missing")
    path = Path(match.group(1))
    support.require(path.is_absolute(), "systemd_exec_start_path_not_absolute")
    return path


def _service_exec_start_path(root: Path, service: str) -> Path:
    return _parse_exec_start_path(
        support.run(
            root,
            "systemctl",
            "show",
            "-p",
            "ExecStart",
            "--value",
            service,
        )
    )


def _normalized_executable_link(value: str) -> Path:
    return Path(value.removesuffix(_DELETED_SUFFIX))


def wait_service(root: Path, service: str, timeout: float = 20.0) -> int:
    expected_exec = _service_exec_start_path(root, service).resolve()
    deadline = time.monotonic() + timeout
    last_state = "unknown"
    last_pid = 0
    last_link = "missing"

    while time.monotonic() < deadline:
        last_state = support.run(
            root,
            "systemctl",
            "is-active",
            service,
            check=False,
        )
        last_pid = support.service_pid(root, service)
        if last_state == "active" and last_pid > 0:
            executable = Path(f"/proc/{last_pid}/exe")
            try:
                last_link = os.readlink(executable)
                running_exec = _normalized_executable_link(last_link).resolve()
            except OSError:
                running_exec = Path()
            if running_exec == expected_exec:
                return last_pid
        time.sleep(0.1)

    raise support.AcceptanceError(
        "service_did_not_reach_exec_start:"
        f"state={last_state}:pid={last_pid}:exe={last_link}"
    )


def stop_service(root: Path, service: str) -> None:
    support.run(root, "systemctl", "stop", service)
    deadline = time.monotonic() + 20.0
    last_state = "unknown"
    last_pid = -1

    while time.monotonic() < deadline:
        last_state = support.run(
            root,
            "systemctl",
            "is-active",
            service,
            check=False,
        )
        last_pid = support.service_pid(root, service)
        if (
            last_state in ("inactive", "failed", "unknown")
            and last_pid == 0
        ):
            return
        time.sleep(0.1)

    raise support.AcceptanceError(
        f"service_did_not_stop:state={last_state}:pid={last_pid}"
    )


def verify_runtime_process(
    root: Path,
    service: str,
    expected_daemon_sha256: str,
    database_path: Path,
    retention_seconds: int,
    idle_seconds: int,
) -> int:
    pid = wait_service(root, service)
    executable = Path(f"/proc/{pid}/exe")
    executable_link = os.readlink(executable)
    running_sha256 = support.sha256(executable)
    installed_path = _service_exec_start_path(root, service)
    installed_sha256 = (
        support.sha256(installed_path)
        if installed_path.is_file()
        else "missing"
    )
    support.require(
        running_sha256 == expected_daemon_sha256,
        "running_new_daemon_mismatch:"
        f"pid={pid}:exe={executable_link}:"
        f"running_sha256={running_sha256}:"
        f"installed_sha256={installed_sha256}:"
        f"expected_sha256={expected_daemon_sha256}",
    )

    environment = support.process_environment(pid)
    expected = {
        f"{support.DATABASE_KEY}={database_path}".encode(),
        f"{support.SECURITY_DATABASE_KEY}={database_path}".encode(),
        f"{support.RETENTION_KEY}={retention_seconds}".encode(),
        f"{support.IDLE_KEY}={idle_seconds}".encode(),
    }
    support.require(
        expected.issubset(environment),
        f"runtime_override_not_applied:pid={pid}",
    )
    return pid


def self_test() -> int:
    parsed = _parse_exec_start_path(
        "{ path=/usr/sbin/vdr-suite-daemon ; "
        "argv[]=/usr/sbin/vdr-suite-daemon ; ignore_errors=no ; }"
    )
    support.require(
        parsed == Path("/usr/sbin/vdr-suite-daemon"),
        "process_self_test_exec_start_parse_failed",
    )
    support.require(
        _normalized_executable_link(
            "/usr/sbin/vdr-suite-daemon (deleted)"
        )
        == Path("/usr/sbin/vdr-suite-daemon"),
        "process_self_test_deleted_suffix_failed",
    )
    print("PHASE_62_RETENTION_RUNTIME_PROCESS_SELF_TEST=PASS")
    return 0


if __name__ == "__main__":
    if sys.argv[1:] != ["--self-test"]:
        raise SystemExit("usage: retention_cleanup_runtime_process.py --self-test")
    raise SystemExit(self_test())
