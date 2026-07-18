#!/usr/bin/env python3

from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def replace_once(path: str, old: str, new: str) -> None:
    target = ROOT / path
    text = target.read_text(encoding="utf-8")
    count = text.count(old)
    if count != 1:
        raise RuntimeError(
            f"{path}: expected one occurrence, found {count}: {old!r}"
        )
    target.write_text(text.replace(old, new, 1), encoding="utf-8")


replace_once(
    "tools/run_sb10d_live_acceptance.py",
    '''import subprocess
import sys
import time
''',
    '''import subprocess
import sys
import threading
import time
''',
)

replace_once(
    "tools/run_sb10d_live_acceptance.py",
    '''class AcceptanceError(RuntimeError):
    pass


def command_text(command: list[str]) -> str:
''',
    '''class AcceptanceError(RuntimeError):
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
''',
)

replace_once(
    "tools/run_sb10d_live_acceptance.py",
    '''def run_safe_daemon_probes(
    base_url: str,
    report_path: Path,
    attempts: int = 1,
) -> None:
''',
    '''def run_disabled_daemon_connection_probe(
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
''',
)

replace_once(
    "tools/run_sb10d_live_acceptance.py",
    '''        if installed_object.exists() or PLUGIN_CONFIG.exists() or suitebridge_config_links():
            raise AcceptanceError(
                "Suite Bridge installation/configuration already exists; refusing overwrite"
            )
''',
    '''        existing_objects = sorted(
            library_directory.glob("libvdr-suitebridge.so*")
        )
        if existing_objects or PLUGIN_CONFIG.exists() or suitebridge_config_links():
            raise AcceptanceError(
                "Suite Bridge installation/configuration already exists; refusing overwrite"
            )
''',
)

replace_once(
    "tools/run_sb10d_live_acceptance.py",
    '''        wait_port("127.0.0.1", 18080, False, timeout_seconds=30)

        plugin_staged = True
''',
    '''        wait_port("127.0.0.1", 18080, False, timeout_seconds=30)

        disabled_probe = run_disabled_daemon_connection_probe(evidence)
        report["disabledDaemon"] = disabled_probe
        report["disabledDaemonNoSuiteBridgeConnection"] = (
            disabled_probe.get("connections") == 0
        )

        plugin_staged = True
''',
)

replace_once(
    "tools/run_sb10d_live_acceptance.py",
    '''        if result.get("mutations_enabled") is not False:
            raise AcceptanceError("live result enabled mutations")
        report["embeddedRuntimeResult"] = result
''',
    '''        if result.get("mutations_enabled") is not False:
            raise AcceptanceError("live result enabled mutations")
        if result.get("saw_degraded") is not True:
            raise AcceptanceError(
                "VDR restart did not expose a degraded observation state"
            )
        report["embeddedRuntimeResult"] = result
''',
)

replace_once(
    "tools/run_sb10d_live_acceptance.py",
    '''            if PLUGIN_CONFIG.exists() or suitebridge_config_links():
                cleanup_errors.append("Suite Bridge configuration remains installed")
            require_clean_worktree()
''',
    '''            installed_object_text = report.get("installedObject", "")
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
''',
)

replace_once(
    "tools/check_sb10d_live_acceptance_contract.py",
    '''        "original_daemon_active",
        "stateBefore",
''',
    '''        "original_daemon_active",
        "run_disabled_daemon_connection_probe(",
        "disabledDaemonNoSuiteBridgeConnection",
        "stateBefore",
''',
)

replace_once(
    "tools/check_sb10d_live_acceptance_contract.py",
    '''    for forbidden in (
        "shell=True",
''',
    '''    require(
        'result.get("saw_degraded") is not True' in runner,
        "live runner must require an observed degraded restart state",
    )
    require(
        'glob("libvdr-suitebridge.so*")' in runner,
        "live runner must reject and detect every Suite Bridge binary variant",
    )

    for forbidden in (
        "shell=True",
''',
)

replace_once(
    "docs/architecture/suite-bridge-embedded-agent-runtime.md",
    '''It refuses a pre-existing Suite Bridge installation, records only hashes for
channel, Timer, Recording and setup state, stops and later restores an active
`vdr-suite-daemon.service`, stages the plugin, starts the repository daemon with
SB.10d enabled, runs safe REST probes, restarts VDR, requires a changed plugin
epoch, verifies clean worker and daemon shutdown, removes the staged plugin and
configuration, restarts VDR, restores the original daemon-service state and
requires a clean worktree.
''',
    '''It refuses a pre-existing Suite Bridge installation, records only hashes for
channel, Timer, Recording and setup state, stops and later restores an active
`vdr-suite-daemon.service`, proves that the disabled daemon opens no Suite Bridge
transport connection, stages the plugin, starts the repository daemon with
SB.10d enabled, runs safe REST probes, restarts VDR, requires both a degraded
observation state and a changed plugin epoch, verifies clean worker and daemon
shutdown, removes every staged plugin binary and configuration, restarts VDR,
restores the original daemon-service state and requires a clean worktree.
''',
)

print("SB.10d live acceptance final review applied")
