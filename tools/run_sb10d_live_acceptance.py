#!/usr/bin/env python3

import argparse
import base64
import hashlib
import json
import os
import shlex
import shutil
import signal
import socket
import subprocess
import sys
import threading
import time
import urllib.error
import urllib.request
from pathlib import Path
from typing import Any

ROOT = Path(__file__).resolve().parents[1]
PLUGIN_DIR = ROOT / "vdr-plugin-suite-bridge"
BUILD_DIR = ROOT / ".build"
DAEMON_BINARY = BUILD_DIR / "vdr-suite-daemon"
LIVE_BINARY = BUILD_DIR / "test_suite_bridge_embedded_agent_runtime_live"
SETUP_PATH = Path("/var/lib/vdr/setup.conf")
VDR_SERVICE = "vdr.service"
DAEMON_SERVICE = "vdr-suite-daemon.service"
PLUGIN_CONFIG = Path("/etc/vdr/conf.avail/suitebridge.conf")


class AcceptanceError(RuntimeError):
    pass


class NoConnectionProbe:
    def __init__(self) -> None:
        self._server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self._server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self._server.bind(("127.0.0.1", 0))
        self._server.listen(8)
        self._server.settimeout(0.1)
        self._port = int(self._server.getsockname()[1])
        self._connections = 0
        self._lock = threading.Lock()
        self._stop = threading.Event()
        self._thread = threading.Thread(
            target=self._run,
            name="sb10d-disabled-connection-probe",
        )

    @property
    def port(self) -> int:
        return self._port

    def start(self) -> None:
        self._thread.start()

    def stop(self) -> None:
        self._stop.set()
        self._thread.join(timeout=2)
        self._server.close()
        if self._thread.is_alive():
            raise AcceptanceError("disabled connection probe did not stop")

    def connection_count(self) -> int:
        with self._lock:
            return self._connections

    def _run(self) -> None:
        while not self._stop.is_set():
            try:
                connection, _ = self._server.accept()
            except socket.timeout:
                continue
            except OSError:
                if self._stop.is_set():
                    return
                raise

            with connection:
                with self._lock:
                    self._connections += 1


def command_text(command: list[str]) -> str:
    return shlex.join(command)


def run(
    command: list[str],
    *,
    cwd: Path = ROOT,
    env: dict[str, str] | None = None,
    check: bool = True,
    capture: bool = True,
    timeout: int = 300,
) -> subprocess.CompletedProcess[str]:
    print(f"+ {command_text(command)}", flush=True)
    completed = subprocess.run(
        command,
        cwd=cwd,
        env=env,
        text=True,
        stdout=subprocess.PIPE if capture else None,
        stderr=subprocess.STDOUT if capture else None,
        timeout=timeout,
        check=False,
    )

    if capture and completed.stdout:
        print(completed.stdout.rstrip(), flush=True)

    if check and completed.returncode != 0:
        raise AcceptanceError(
            f"command failed with exit code {completed.returncode}: "
            f"{command_text(command)}"
        )

    return completed


def require_command(name: str) -> None:
    if shutil.which(name) is None:
        raise AcceptanceError(f"required command is unavailable: {name}")


def service_active(name: str) -> bool:
    completed = subprocess.run(
        ["systemctl", "is-active", "--quiet", name],
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
        check=False,
    )
    return completed.returncode == 0


def wait_service(name: str, active: bool, timeout_seconds: int = 30) -> None:
    deadline = time.monotonic() + timeout_seconds

    while time.monotonic() < deadline:
        if service_active(name) == active:
            return
        time.sleep(0.25)

    expected = "active" if active else "inactive"
    raise AcceptanceError(f"service {name} did not become {expected}")


def port_open(host: str, port: int, timeout: float = 0.2) -> bool:
    try:
        with socket.create_connection((host, port), timeout=timeout):
            return True
    except OSError:
        return False


def wait_port(host: str, port: int, open_state: bool, timeout_seconds: int = 30) -> None:
    deadline = time.monotonic() + timeout_seconds

    while time.monotonic() < deadline:
        if port_open(host, port) == open_state:
            return
        time.sleep(0.25)

    expected = "open" if open_state else "closed"
    raise AcceptanceError(f"port {host}:{port} did not become {expected}")


def request_json(
    url: str,
    *,
    username: str = "",
    password: str = "",
    timeout: int = 30,
) -> Any:
    headers = {"Accept": "application/json"}

    if username:
        token = base64.b64encode(
            f"{username}:{password}".encode("utf-8")
        ).decode("ascii")
        headers["Authorization"] = f"Basic {token}"

    request = urllib.request.Request(url, headers=headers, method="GET")

    try:
        with urllib.request.urlopen(request, timeout=timeout) as response:
            if response.status != 200:
                raise AcceptanceError(
                    f"expected HTTP 200 from {url}, got {response.status}"
                )
            payload = response.read().decode("utf-8", errors="strict")
    except urllib.error.HTTPError as error:
        raise AcceptanceError(
            f"HTTP request failed for {url}: {error.code}"
        ) from error
    except Exception as error:
        raise AcceptanceError(f"request failed for {url}: {error}") from error

    try:
        return json.loads(payload)
    except json.JSONDecodeError as error:
        raise AcceptanceError(f"invalid JSON from {url}: {error}") from error


def wait_json(
    url: str,
    *,
    username: str = "",
    password: str = "",
    timeout_seconds: int = 60,
) -> Any:
    deadline = time.monotonic() + timeout_seconds
    last_error = ""

    while time.monotonic() < deadline:
        try:
            return request_json(
                url,
                username=username,
                password=password,
                timeout=10,
            )
        except AcceptanceError as error:
            last_error = str(error)
            time.sleep(0.5)

    raise AcceptanceError(f"JSON endpoint did not recover: {url}: {last_error}")


def canonical_hash(value: Any) -> str:
    encoded = json.dumps(
        value,
        sort_keys=True,
        separators=(",", ":"),
        ensure_ascii=False,
    ).encode("utf-8")
    return hashlib.sha256(encoded).hexdigest()


def file_hash(path: Path) -> str:
    if not path.exists():
        return "missing"

    digest = hashlib.sha256()
    with path.open("rb") as handle:
        while True:
            block = handle.read(1024 * 1024)
            if not block:
                break
            digest.update(block)
    return digest.hexdigest()


def capture_vdr_state(
    base_url: str,
    username: str,
    password: str,
) -> dict[str, str]:
    endpoints = {
        "channels": "/channels.json",
        "timers": "/timers.json",
        "recordings": "/recordings.json",
    }
    state: dict[str, str] = {}

    for name, path in endpoints.items():
        payload = wait_json(
            base_url.rstrip("/") + path,
            username=username,
            password=password,
            timeout_seconds=60,
        )
        state[name] = canonical_hash(payload)

    state["setup"] = file_hash(SETUP_PATH)
    return state


def git_head() -> str:
    return run(["git", "rev-parse", "HEAD"]).stdout.strip()


def require_clean_worktree() -> None:
    status = run(["git", "status", "--porcelain"]).stdout.strip()
    if status:
        raise AcceptanceError("repository worktree is not clean")


def vdr_pids() -> list[int]:
    completed = subprocess.run(
        ["pidof", "vdr"],
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.DEVNULL,
        check=False,
    )
    if completed.returncode != 0 or not completed.stdout.strip():
        return []
    return [int(value) for value in completed.stdout.split()]


def plugin_mapped() -> bool:
    for pid in vdr_pids():
        maps = Path(f"/proc/{pid}/maps")
        try:
            text = maps.read_text(encoding="utf-8", errors="replace")
        except OSError:
            continue
        if "libvdr-suitebridge.so" in text:
            return True
    return False


def wait_plugin_mapped(expected: bool, timeout_seconds: int = 30) -> None:
    deadline = time.monotonic() + timeout_seconds
    while time.monotonic() < deadline:
        if plugin_mapped() == expected:
            return
        time.sleep(0.25)
    state = "mapped" if expected else "unmapped"
    raise AcceptanceError(f"Suite Bridge plugin did not become {state}")


def suitebridge_config_links() -> list[Path]:
    directory = Path("/etc/vdr/conf.d")
    if not directory.exists():
        return []
    return sorted(directory.glob("*suitebridge*.conf"))


def wait_file(path: Path, timeout_seconds: int) -> dict[str, Any]:
    deadline = time.monotonic() + timeout_seconds
    last_error = ""

    while time.monotonic() < deadline:
        if path.exists():
            try:
                return json.loads(path.read_text(encoding="utf-8"))
            except Exception as error:
                last_error = str(error)
        time.sleep(0.1)

    raise AcceptanceError(f"file did not become valid JSON: {path}: {last_error}")


def wait_log(path: Path, marker: str, timeout_seconds: int) -> None:
    deadline = time.monotonic() + timeout_seconds
    while time.monotonic() < deadline:
        if path.exists():
            text = path.read_text(encoding="utf-8", errors="replace")
            if marker in text:
                return
        time.sleep(0.1)
    raise AcceptanceError(f"log marker not observed: {marker}")


def stop_process(
    process: subprocess.Popen[str] | None,
    *,
    name: str,
    timeout_seconds: int = 20,
    require_clean: bool = False,
) -> bool:
    if process is None or process.poll() is not None:
        if process is None:
            return True
        return not require_clean or process.returncode == 0

    os.killpg(process.pid, signal.SIGTERM)
    try:
        process.wait(timeout=timeout_seconds)
    except subprocess.TimeoutExpired:
        os.killpg(process.pid, signal.SIGKILL)
        process.wait(timeout=5)
        return False

    return not require_clean or process.returncode == 0


def run_disabled_daemon_connection_probe(
    evidence: Path,
) -> dict[str, Any]:
    probe = NoConnectionProbe()
    process: subprocess.Popen[str] | None = None
    log_handle = None
    log_path = evidence / "daemon-disabled.log"
    database_path = evidence / "vdr-suite-disabled.db"

    environment = os.environ.copy()
    environment.update(
        {
            "VDR_SUITE_DATABASE_PATH": str(database_path),
            "VDR_SUITE_FRONTEND_ROOT": str(ROOT / "web/frontend"),
            "VDR_SUITE_SUITE_BRIDGE_ENABLED": "false",
            "VDR_SUITE_SUITE_BRIDGE_BACKEND_ID": "default",
            "VDR_SUITE_SUITE_BRIDGE_HOST": "127.0.0.1",
            "VDR_SUITE_SUITE_BRIDGE_PORT": str(probe.port),
        }
    )

    try:
        probe.start()
        log_handle = log_path.open("w", encoding="utf-8")
        process = subprocess.Popen(
            [str(DAEMON_BINARY)],
            cwd=ROOT,
            env=environment,
            text=True,
            stdout=log_handle,
            stderr=subprocess.STDOUT,
            start_new_session=True,
        )
        wait_port("127.0.0.1", 18080, True, timeout_seconds=30)
        time.sleep(1.0)

        if process.poll() is not None:
            raise AcceptanceError("disabled test daemon exited during startup")

        if not stop_process(
            process,
            name="disabled test daemon",
            timeout_seconds=20,
            require_clean=True,
        ):
            raise AcceptanceError("disabled test daemon did not stop cleanly")
        process = None
        wait_port("127.0.0.1", 18080, False, timeout_seconds=30)

        log_handle.flush()
        log_text = log_path.read_text(encoding="utf-8", errors="replace")
        if "Suite Bridge embedded Agent runtime started" in log_text:
            raise AcceptanceError(
                "disabled daemon reported a Suite Bridge runtime start"
            )

        connections = probe.connection_count()
        if connections != 0:
            raise AcceptanceError(
                "disabled daemon opened a Suite Bridge transport connection"
            )

        return {
            "configured": False,
            "connections": connections,
            "shutdown": "clean",
        }
    finally:
        stop_process(
            process,
            name="disabled test daemon",
            timeout_seconds=20,
        )
        if log_handle is not None:
            log_handle.close()
        probe.stop()


def run_safe_daemon_probes(
    base_url: str,
    report_path: Path,
    attempts: int = 1,
) -> None:
    last_output = ""

    for attempt in range(attempts):
        completed = subprocess.run(
            [
                "python3",
                "tools/real-vdr-acceptance/runner.py",
                "--max-risk",
                "safe",
                "--base-url",
                base_url,
                "--report-json",
                str(report_path),
            ],
            cwd=ROOT,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            check=False,
        )
        last_output = completed.stdout or ""
        print(last_output.rstrip(), flush=True)

        if completed.returncode == 0:
            return

        if attempt + 1 < attempts:
            time.sleep(2)

    raise AcceptanceError(
        "safe daemon probes failed"
        + (f": {last_output.strip()}" if last_output.strip() else "")
    )


def write_json(path: Path, value: Any) -> None:
    path.write_text(
        json.dumps(value, indent=2, sort_keys=True) + "\n",
        encoding="utf-8",
    )


def main() -> int:
    parser = argparse.ArgumentParser(
        description=(
            "Run controlled SB.10d live VDR, daemon, reconnect, shutdown "
            "and rollback acceptance."
        )
    )
    parser.add_argument("--svdrp-host", default="127.0.0.1")
    parser.add_argument("--svdrp-port", type=int, default=6419)
    parser.add_argument("--restfulapi-base-url", default="http://127.0.0.1:8002")
    parser.add_argument("--restfulapi-user", default="")
    parser.add_argument("--restfulapi-password", default="")
    parser.add_argument("--daemon-base-url", default="http://127.0.0.1:18080")
    parser.add_argument("--timeout-seconds", type=int, default=90)
    parser.add_argument(
        "--evidence-dir",
        default="",
        help="Optional evidence directory. Defaults to /tmp with a timestamp.",
    )
    args = parser.parse_args()

    if os.geteuid() != 0:
        print("ERROR: SB.10d live acceptance must run as root", file=sys.stderr)
        return 1

    if args.svdrp_port <= 0 or args.svdrp_port > 65535:
        print("ERROR: invalid SVDRP port", file=sys.stderr)
        return 1

    for command in (
        "g++",
        "git",
        "install",
        "make",
        "pidof",
        "pkg-config",
        "systemctl",
        "vdr",
        "vdrctl",
    ):
        require_command(command)

    timestamp = time.strftime("%Y%m%d-%H%M%S")
    evidence = Path(args.evidence_dir) if args.evidence_dir else Path(
        f"/tmp/vdr-suite-sb10d-live-{timestamp}"
    )
    evidence.mkdir(parents=True, exist_ok=False)

    ready_file = evidence / "embedded-runtime-ready.json"
    result_file = evidence / "embedded-runtime-result.json"
    live_log_path = evidence / "embedded-runtime.log"
    daemon_log_path = evidence / "daemon.log"
    pre_report_path = evidence / "daemon-safe-before-restart.json"
    post_report_path = evidence / "daemon-safe-after-restart.json"
    final_report_path = evidence / "acceptance-result.json"
    test_database = evidence / "vdr-suite-live.db"

    live_process: subprocess.Popen[str] | None = None
    daemon_process: subprocess.Popen[str] | None = None
    live_log = None
    daemon_log = None
    plugin_staged = False
    original_daemon_active = False
    initial_state: dict[str, str] | None = None
    final_state: dict[str, str] | None = None
    primary_error = ""
    cleanup_errors: list[str] = []
    report: dict[str, Any] = {
        "status": "failed",
        "evidenceDirectory": str(evidence),
    }

    try:
        require_clean_worktree()
        report["repositoryHead"] = git_head()

        if not service_active(VDR_SERVICE):
            raise AcceptanceError("VDR service is not active before acceptance")

        if plugin_mapped():
            raise AcceptanceError("Suite Bridge plugin is already mapped before acceptance")

        api_version = run(
            ["pkg-config", "--variable=apiversion", "vdr"]
        ).stdout.strip()
        library_directory = Path(
            run(["pkg-config", "--variable=libdir", "vdr"]).stdout.strip()
        )
        if not api_version or not library_directory.is_absolute():
            raise AcceptanceError("failed to resolve VDR plugin installation path")

        installed_object = (
            library_directory / f"libvdr-suitebridge.so.{api_version}"
        )
        report["vdrApiVersion"] = api_version
        report["vdrVersion"] = run(["vdr", "--version"]).stdout.splitlines()[0]
        report["installedObject"] = str(installed_object)

        existing_objects = sorted(
            library_directory.glob("libvdr-suitebridge.so*")
        )
        if existing_objects or PLUGIN_CONFIG.exists() or suitebridge_config_links():
            raise AcceptanceError(
                "Suite Bridge installation/configuration already exists; refusing overwrite"
            )

        initial_state = capture_vdr_state(
            args.restfulapi_base_url,
            args.restfulapi_user,
            args.restfulapi_password,
        )
        report["stateBefore"] = initial_state

        run(["make", "test-suite-bridge-embedded-runtime"])
        run(["make", "daemon"], timeout=900)
        run(["make", "build-suite-bridge-embedded-runtime-live"], timeout=300)
        run(["make", "clean"], cwd=PLUGIN_DIR)
        run(["make", "check"], cwd=PLUGIN_DIR, timeout=600)

        built_plugin = PLUGIN_DIR / "libvdr-suitebridge.so"
        if not built_plugin.is_file() or built_plugin.stat().st_size == 0:
            raise AcceptanceError("Suite Bridge shared object was not built")
        report["pluginSha256"] = file_hash(built_plugin)

        original_daemon_active = service_active(DAEMON_SERVICE)
        report["originalDaemonServiceActive"] = original_daemon_active

        if original_daemon_active:
            run(["systemctl", "stop", DAEMON_SERVICE])
            wait_service(DAEMON_SERVICE, False)

        wait_port("127.0.0.1", 18080, False, timeout_seconds=30)

        disabled_probe = run_disabled_daemon_connection_probe(evidence)
        report["disabledDaemon"] = disabled_probe
        report["disabledDaemonNoSuiteBridgeConnection"] = (
            disabled_probe.get("connections") == 0
        )

        plugin_staged = True
        run(
            [
                "install",
                "-m",
                "0755",
                str(built_plugin),
                str(installed_object),
            ]
        )
        PLUGIN_CONFIG.write_text("[suitebridge]\n", encoding="utf-8")
        run(["vdrctl", "enable", "suitebridge"])
        if not suitebridge_config_links():
            raise AcceptanceError("vdrctl did not create a Suite Bridge configuration link")

        run(["systemctl", "restart", VDR_SERVICE])
        wait_service(VDR_SERVICE, True)
        wait_plugin_mapped(True)
        wait_json(
            args.restfulapi_base_url.rstrip("/") + "/channels.json",
            username=args.restfulapi_user,
            password=args.restfulapi_password,
            timeout_seconds=60,
        )

        daemon_environment = os.environ.copy()
        daemon_environment.update(
            {
                "VDR_SUITE_DATABASE_PATH": str(test_database),
                "VDR_SUITE_FRONTEND_ROOT": str(ROOT / "web/frontend"),
                "VDR_SUITE_SUITE_BRIDGE_ENABLED": "true",
                "VDR_SUITE_SUITE_BRIDGE_BACKEND_ID": "default",
                "VDR_SUITE_SUITE_BRIDGE_HOST": args.svdrp_host,
                "VDR_SUITE_SUITE_BRIDGE_PORT": str(args.svdrp_port),
                "VDR_SUITE_SUITE_BRIDGE_CONNECT_TIMEOUT_MS": "1000",
                "VDR_SUITE_SUITE_BRIDGE_IO_TIMEOUT_MS": "1000",
                "VDR_SUITE_SUITE_BRIDGE_OPERATION_TIMEOUT_MS": "3000",
                "VDR_SUITE_SUITE_BRIDGE_POLL_INTERVAL_MS": "250",
                "VDR_SUITE_SUITE_BRIDGE_STALE_AFTER_MS": "750",
                "VDR_SUITE_SUITE_BRIDGE_OFFLINE_AFTER_MS": "3000",
                "VDR_SUITE_SUITE_BRIDGE_RECONNECT_INITIAL_MS": "250",
                "VDR_SUITE_SUITE_BRIDGE_RECONNECT_MAXIMUM_MS": "1000",
            }
        )
        daemon_log = daemon_log_path.open("w", encoding="utf-8")
        daemon_process = subprocess.Popen(
            [str(DAEMON_BINARY)],
            cwd=ROOT,
            env=daemon_environment,
            text=True,
            stdout=daemon_log,
            stderr=subprocess.STDOUT,
            start_new_session=True,
        )
        wait_log(
            daemon_log_path,
            "Suite Bridge embedded Agent runtime started: backend=default",
            30,
        )
        wait_port("127.0.0.1", 18080, True, timeout_seconds=30)
        if daemon_process.poll() is not None:
            raise AcceptanceError("test daemon exited during startup")

        run_safe_daemon_probes(args.daemon_base_url, pre_report_path)

        live_environment = os.environ.copy()
        live_environment.update(
            {
                "VDR_SUITE_SUITE_BRIDGE_BACKEND_ID": "default",
                "VDR_SUITE_SUITE_BRIDGE_HOST": args.svdrp_host,
                "VDR_SUITE_SUITE_BRIDGE_PORT": str(args.svdrp_port),
                "VDR_SUITE_SB10D_TIMEOUT_SECONDS": str(args.timeout_seconds),
                "VDR_SUITE_SB10D_EXPECT_EPOCH_CHANGE": "true",
                "VDR_SUITE_SB10D_READY_FILE": str(ready_file),
                "VDR_SUITE_SB10D_RESULT_FILE": str(result_file),
            }
        )
        live_log = live_log_path.open("w", encoding="utf-8")
        live_process = subprocess.Popen(
            [str(LIVE_BINARY)],
            cwd=ROOT,
            env=live_environment,
            text=True,
            stdout=live_log,
            stderr=subprocess.STDOUT,
            start_new_session=True,
        )

        ready = wait_file(ready_file, 30)
        if ready.get("status") != "ready" or not ready.get("initial_epoch"):
            raise AcceptanceError("embedded runtime did not publish a valid ready state")
        report["embeddedRuntimeReady"] = ready

        run(["systemctl", "restart", VDR_SERVICE])
        wait_service(VDR_SERVICE, True)
        wait_plugin_mapped(True)

        try:
            live_return_code = live_process.wait(timeout=args.timeout_seconds)
        except subprocess.TimeoutExpired as error:
            raise AcceptanceError(
                "embedded runtime did not recover after VDR restart"
            ) from error

        if live_return_code != 0:
            live_output = live_log_path.read_text(
                encoding="utf-8",
                errors="replace",
            )
            raise AcceptanceError(
                "embedded runtime live probe failed: " + live_output.strip()
            )

        result = wait_file(result_file, 5)
        if result.get("status") != "passed":
            raise AcceptanceError("embedded runtime result is not passed")
        if result.get("initial_epoch") == result.get("final_epoch"):
            raise AcceptanceError("VDR restart did not replace the plugin epoch")
        if result.get("mutations_enabled") is not False:
            raise AcceptanceError("live result enabled mutations")
        if result.get("saw_degraded") is not True:
            raise AcceptanceError(
                "VDR restart did not expose a degraded observation state"
            )
        report["embeddedRuntimeResult"] = result

        if daemon_process.poll() is not None:
            raise AcceptanceError("test daemon exited during VDR restart")

        wait_json(
            args.daemon_base_url.rstrip("/") + "/api/vdr/status",
            timeout_seconds=60,
        )
        run_safe_daemon_probes(
            args.daemon_base_url,
            post_report_path,
            attempts=15,
        )

        if not stop_process(
            daemon_process,
            name="test daemon",
            timeout_seconds=20,
            require_clean=True,
        ):
            raise AcceptanceError("test daemon did not stop cleanly")
        daemon_process = None
        wait_port("127.0.0.1", 18080, False, timeout_seconds=30)
        report["daemonShutdown"] = "clean"
        report["safeDaemonProbesBeforeRestart"] = True
        report["safeDaemonProbesAfterRestart"] = True

    except Exception as error:
        primary_error = str(error)
        report["error"] = primary_error
    finally:
        if daemon_log is not None:
            daemon_log.flush()
        if live_log is not None:
            live_log.flush()

        if not stop_process(
            live_process,
            name="embedded runtime probe",
            timeout_seconds=10,
        ):
            cleanup_errors.append("embedded runtime probe required forced stop")

        if not stop_process(
            daemon_process,
            name="test daemon",
            timeout_seconds=20,
        ):
            cleanup_errors.append("test daemon required forced stop")

        if live_log is not None:
            live_log.close()
        if daemon_log is not None:
            daemon_log.close()

        if plugin_staged:
            try:
                run(["vdrctl", "disable", "suitebridge"], check=False)
                for link in suitebridge_config_links():
                    link.unlink(missing_ok=True)
                PLUGIN_CONFIG.unlink(missing_ok=True)

                api_version = report.get("vdrApiVersion", "")
                installed_object_text = report.get("installedObject", "")
                if api_version and installed_object_text:
                    Path(installed_object_text).unlink(missing_ok=True)

                run(["systemctl", "restart", VDR_SERVICE])
                wait_service(VDR_SERVICE, True)
                wait_plugin_mapped(False)
                wait_json(
                    args.restfulapi_base_url.rstrip("/") + "/channels.json",
                    username=args.restfulapi_user,
                    password=args.restfulapi_password,
                    timeout_seconds=60,
                )
            except Exception as error:
                cleanup_errors.append(f"plugin rollback failed: {error}")

        if original_daemon_active:
            try:
                run(["systemctl", "start", DAEMON_SERVICE])
                wait_service(DAEMON_SERVICE, True)
            except Exception as error:
                cleanup_errors.append(f"daemon service restore failed: {error}")

        try:
            run(["make", "clean"], cwd=PLUGIN_DIR, check=False)
            run(["make", "clean"], check=False)
        except Exception as error:
            cleanup_errors.append(f"build cleanup failed: {error}")

        try:
            if initial_state is not None:
                final_state = capture_vdr_state(
                    args.restfulapi_base_url,
                    args.restfulapi_user,
                    args.restfulapi_password,
                )
                report["stateAfter"] = final_state
                report["vdrStateUnchanged"] = final_state == initial_state
                if final_state != initial_state:
                    changed = sorted(
                        key
                        for key in initial_state
                        if initial_state.get(key) != final_state.get(key)
                    )
                    cleanup_errors.append(
                        "VDR state changed: " + ", ".join(changed)
                    )
        except Exception as error:
            cleanup_errors.append(f"final VDR state capture failed: {error}")

        try:
            if plugin_mapped():
                cleanup_errors.append("Suite Bridge plugin remains mapped")
            installed_object_text = report.get("installedObject", "")
            remaining_objects = []
            if installed_object_text:
                remaining_objects = sorted(
                    Path(installed_object_text).parent.glob(
                        "libvdr-suitebridge.so*"
                    )
                )
            if remaining_objects:
                cleanup_errors.append("Suite Bridge binary remains installed")
            if PLUGIN_CONFIG.exists() or suitebridge_config_links():
                cleanup_errors.append("Suite Bridge configuration remains installed")
            require_clean_worktree()
            report["worktreeClean"] = True
        except Exception as error:
            cleanup_errors.append(f"final repository check failed: {error}")

        report["cleanupErrors"] = cleanup_errors
        report["rollbackComplete"] = not cleanup_errors

        if not primary_error and not cleanup_errors:
            report["status"] = "passed"
        else:
            report["status"] = "failed"

        write_json(final_report_path, report)

    print(json.dumps(report, indent=2, sort_keys=True), flush=True)
    print(f"SB.10d evidence: {evidence}", flush=True)

    if report["status"] != "passed":
        return 1

    print("SB.10d CONTROLLED LIVE ACCEPTANCE PASSED", flush=True)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
